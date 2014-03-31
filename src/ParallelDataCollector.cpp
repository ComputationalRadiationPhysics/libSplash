/**
 * Copyright 2013 Felix Schmitt
 *
 * This file is part of libSplash. 
 * 
 * libSplash is free software: you can redistribute it and/or modify 
 * it under the terms of of either the GNU General Public License or 
 * the GNU Lesser General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version. 
 * libSplash is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License and the GNU Lesser General Public License 
 * for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * and the GNU Lesser General Public License along with libSplash. 
 * If not, see <http://www.gnu.org/licenses/>. 
 */

#include <cassert>
#include <set>
#include <dirent.h>
#include <stdlib.h>
#include <cstring>

#include "splash/ParallelDataCollector.hpp"
#include "splash/core/DCParallelDataSet.hpp"
#include "splash/core/DCAttribute.hpp"
#include "splash/core/DCParallelGroup.hpp"
#include "splash/core/logging.hpp"

namespace splash
{

    /*******************************************************************************
     * PRIVATE FUNCTIONS
     *******************************************************************************/

    void ParallelDataCollector::setFileAccessParams(hid_t& fileAccProperties)
    {
        fileAccProperties = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_mpio(fileAccProperties, options.mpiComm, options.mpiInfo);

        int metaCacheElements = 0;
        size_t rawCacheElements = 0;
        size_t rawCacheSize = 0;
        double policy = 0.0;

        // set new cache size
        /*
         * Note from http://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetCache:
         * "Raw dataset chunk caching is not currently supported when using the MPI I/O
         * and MPI POSIX file drivers in read/write mode [..]. When using one of these
         * file drivers, all calls to H5Dread and H5Dwrite will access the disk directly,
         * and H5Pset_cache will have no effect on performance."
         */
        H5Pget_cache(fileAccProperties, &metaCacheElements, &rawCacheElements, &rawCacheSize, &policy);
        rawCacheSize = 256 * 1024 * 1024;
        H5Pset_cache(fileAccProperties, metaCacheElements, rawCacheElements, rawCacheSize, policy);

        log_msg(3, "Raw Data Cache (File) = %llu KiB", (long long unsigned) (rawCacheSize / 1024));
    }

    std::string ParallelDataCollector::getFullFilename(uint32_t id, std::string baseFilename)
    {
        std::stringstream serial_filename;
        serial_filename << baseFilename << "_" << id << ".h5";

        return serial_filename.str();
    }

    std::string ParallelDataCollector::getExceptionString(std::string func, std::string msg,
            const char *info)
    {
        std::stringstream full_msg;
        full_msg << "Exception for ParallelDataCollector::" << func <<
                ": " << msg;

        if (info != NULL)
            full_msg << " (" << info << ")";

        return full_msg.str();
    }

    void ParallelDataCollector::indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
    {
        mpiPos[2] = index / (mpiSize[0] * mpiSize[1]);
        mpiPos[1] = (index % (mpiSize[0] * mpiSize[1])) / mpiSize[0];
        mpiPos[0] = index % mpiSize[0];
    }

    void ParallelDataCollector::listFilesInDir(const std::string baseFilename, std::set<int32_t> &ids)
    throw (DCException)
    {
        log_msg(2, "listing files for %s", baseFilename.c_str());

        std::string dir_path, name;
        std::string::size_type pos = baseFilename.find_last_of('/');
        if (pos == std::string::npos)
        {
            dir_path.assign(".");
            name.assign(baseFilename);
        } else
        {
            dir_path.assign(baseFilename.c_str(), baseFilename.c_str() + pos);
            name.assign(baseFilename.c_str() + pos + 1);
            name.append("_");
        }

        dirent *dp = NULL;
        DIR *dirp = NULL;

        dirp = opendir(dir_path.c_str());
        if (!dirp)
        {
            throw DCException(getExceptionString("listFilesInDir",
                    "Failed to open directory", dir_path.c_str()));
        }

        while ((dp = readdir(dirp)) != NULL)
        {
            // size matches and starts with name.c_str()
            if (strstr(dp->d_name, name.c_str()) == dp->d_name)
            {
                std::string fname;
                fname.assign(dp->d_name);
                // end with correct file extension
                if (fname.rfind(".h5") != fname.size() - 3)
                    continue;

                // extract id from filename (part between "/path/prefix_" and ".h5")
                char* endPtr = NULL;
                std::string idStr = fname.substr(fname.rfind("_") + 1,
                        fname.size() - 3 - name.size());

                int32_t id = strtol(idStr.c_str(), &endPtr, 10);
                if (endPtr && *endPtr == 0L) {
                    ids.insert(id);
                }
            }
        }
        (void) closedir(dirp);
    }

    /*******************************************************************************
     * PUBLIC FUNCTIONS
     *******************************************************************************/

    ParallelDataCollector::ParallelDataCollector(MPI_Comm comm, MPI_Info info,
            const Dimensions topology, uint32_t maxFileHandles) :
    handles(maxFileHandles, HandleMgr::FNS_ITERATIONS),
    fileStatus(FST_CLOSED)
    {
        parseEnvVars();

        if (MPI_Comm_dup(comm, &(options.mpiComm)) != MPI_SUCCESS)
            throw DCException(getExceptionString("ParallelDataCollector",
                "failed to duplicate MPI communicator"));

        MPI_Comm_rank(options.mpiComm, &(options.mpiRank));
        options.enableCompression = false;
        options.mpiInfo = info;
        options.mpiSize = topology.getScalarSize();
        options.mpiTopology.set(topology);
        options.maxID = -1;
        
        setLogMpiRank(options.mpiRank);

        if (H5open() < 0)
            throw DCException(getExceptionString("ParallelDataCollector",
                "failed to initialize/open HDF5 library"));

#ifndef SPLASH_VERBOSE_HDF5
        // surpress automatic output of HDF5 exception messages
        if (H5Eset_auto2(H5E_DEFAULT, NULL, NULL) < 0)
            throw DCException(getExceptionString("ParallelDataCollector",
                "failed to disable error printing"));
#endif

        // set some default file access parameters
        setFileAccessParams(fileAccProperties);

        handles.registerFileCreate(fileCreateCallback, &options);
        handles.registerFileOpen(fileOpenCallback, &options);

        indexToPos(options.mpiRank, options.mpiTopology, options.mpiPos);
    }

    ParallelDataCollector::~ParallelDataCollector()
    {
        H5Pclose(fileAccProperties);
    }

    void ParallelDataCollector::open(const char* filename, FileCreationAttr &attr)
    throw (DCException)
    {
        log_msg(1, "opening parallel data collector");

        if (filename == NULL)
            throw DCException(getExceptionString("open", "filename must not be null"));

        if (fileStatus != FST_CLOSED)
            throw DCException(getExceptionString("open", "this access is not permitted"));

        this->baseFilename.assign(filename);

        switch (attr.fileAccType)
        {
            case FAT_READ:
            case FAT_READ_MERGED:
                openRead(filename, attr);
                break;
            case FAT_WRITE:
                openWrite(filename, attr);
                break;
            case FAT_CREATE:
                openCreate(filename, attr);
                break;
        }
    }

    void ParallelDataCollector::close()
    {
        log_msg(1, "closing parallel data collector");

        // close opened hdf5 file handles
        handles.close();

        options.maxID = -1;

        fileStatus = FST_CLOSED;
    }

    int32_t ParallelDataCollector::getMaxID()
    {
        std::set<int32_t> ids;
        listFilesInDir(this->baseFilename, ids);

        if (ids.size() > 0)
            options.maxID = *(ids.rbegin());

        return options.maxID;
    }

    void ParallelDataCollector::getMPISize(Dimensions& mpiSize)
    {
        mpiSize.set(options.mpiTopology);
    }

    void ParallelDataCollector::getEntryIDs(int32_t *ids, size_t *count)
    throw (DCException)
    {
        std::set<int32_t> file_ids;
        listFilesInDir(this->baseFilename, file_ids);

        if (count != NULL)
            *count = file_ids.size();

        if (ids != NULL)
        {
            size_t ctr = 0;
            for (std::set<int32_t>::const_iterator iter = file_ids.begin();
                    iter != file_ids.end(); ++iter)
            {
                ids[ctr] = *iter;
                ++ctr;
            }
        }
    }

    void ParallelDataCollector::getEntriesForID(int32_t id, DCEntry *entries,
            size_t *count)
    throw (DCException)
    {
        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        // open data group for id
        DCParallelGroup group;
        group.open(handles.get(id), group_id_name.str());

        DCParallelGroup::VisitObjCBType param;
        param.count = 0;
        param.entries = entries;

        DCParallelGroup::getEntriesInternal(group.getHandle(), group_id_name.str(), "", &param);

        if (count)
            *count = param.count;
    }

    void ParallelDataCollector::readGlobalAttribute(int32_t id,
            const char* name,
            void *data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("readGlobalAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
            throw DCException(getExceptionString("readGlobalAttribute", "this access is not permitted"));

        DCParallelGroup group_custom;
        group_custom.open(handles.get(id), SDC_GROUP_CUSTOM);

        try
        {
            DCAttribute::readAttribute(name, group_custom.getHandle(), data);
        } catch (DCException e)
        {
            log_msg(0, "Exception: %s", e.what());
            throw DCException(getExceptionString("readGlobalAttribute", "failed to open attribute", name));
        }
    }

    void ParallelDataCollector::writeGlobalAttribute(int32_t id,
            const CollectionType& type,
            const char *name,
            const void* data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("writeGlobalAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("writeGlobalAttribute", "this access is not permitted"));

        DCParallelGroup group_custom;
        group_custom.open(handles.get(id), SDC_GROUP_CUSTOM);

        try
        {
            DCAttribute::writeAttribute(name, type.getDataType(), group_custom.getHandle(), data);
        } catch (DCException e)
        {
            log_msg(0, "Exception: %s", e.what());
            throw DCException(getExceptionString("writeGlobalAttribute", "failed to write attribute", name));
        }
    }

    void ParallelDataCollector::readAttribute(int32_t id,
            const char *dataName,
            const char *attrName,
            void* data,
            Dimensions* /*mpiPosition*/)
    throw (DCException)
    {
        // mpiPosition is ignored
        if (attrName == NULL || data == NULL)
            throw DCException(getExceptionString("readAttribute", "a parameter was null"));

        // dataName may be NULL, attribute is read from iteration group in that case
        if (dataName && strlen(dataName) == 0)
            throw DCException(getExceptionString("readAttribute", "empty dataset name"));

        if (strlen(attrName) == 0)
            throw DCException(getExceptionString("readAttribute", "empty attribute name"));

        if (fileStatus == FST_CLOSED)
            throw DCException(getExceptionString("readAttribute", "this access is not permitted"));

        std::string group_path, obj_name;
        std::string dataNameInternal = "";
        if (dataName)
            dataNameInternal.assign(dataName);
        DCDataSet::getFullDataPath(dataNameInternal, SDC_GROUP_DATA, id, group_path, obj_name);

        DCParallelGroup group;
        group.open(handles.get(id), group_path);

        if (dataName)
        {
            // read attribute from the dataset or group
            hid_t obj_id = H5Oopen(group.getHandle(), obj_name.c_str(), H5P_DEFAULT);
            if (obj_id < 0)
            {
                throw DCException(getExceptionString("readAttribute",
                        "dataset not found", obj_name.c_str()));
            }

            try
            {
                DCAttribute::readAttribute(attrName, obj_id, data);
            } catch (DCException)
            {
                H5Oclose(obj_id);
                throw;
            }
            H5Oclose(obj_id);
        } else
        {
            // read attribute from the iteration
            DCAttribute::readAttribute(attrName, group.getHandle(), data);
        }
    }

    void ParallelDataCollector::writeAttribute(int32_t id,
            const CollectionType& type,
            const char *dataName,
            const char *attrName,
            const void* data)
    throw (DCException)
    {
        if (attrName == NULL || data == NULL)
            throw DCException(getExceptionString("writeAttribute", "a parameter was null"));

        // dataName may be NULL, attribute is attached to iteration group in that case
        if (dataName && strlen(dataName) == 0)
            throw DCException(getExceptionString("writeAttribute", "empty dataset name"));

        if (strlen(attrName) == 0)
            throw DCException(getExceptionString("writeAttribute", "empty attribute name"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("writeAttribute", "this access is not permitted"));

        std::string group_path, obj_name;
        std::string dataNameInternal = "";
        if (dataName)
            dataNameInternal.assign(dataName);
        DCDataSet::getFullDataPath(dataNameInternal, SDC_GROUP_DATA, id, group_path, obj_name);

        DCParallelGroup group;
        if (dataName)
        {
            // attach attribute to the dataset or group
            group.open(handles.get(id), group_path);
            hid_t obj_id = H5Oopen(group.getHandle(), obj_name.c_str(), H5P_DEFAULT);
            if (obj_id < 0)
            {
                throw DCException(getExceptionString("writeAttribute",
                        "object not found", obj_name.c_str()));
            }

            try
            {
                DCAttribute::writeAttribute(attrName, type.getDataType(), obj_id, data);
            } catch (DCException)
            {
                H5Oclose(obj_id);
                throw;
            }
            H5Oclose(obj_id);
        } else
        {
            // attach attribute to the iteration
            group.openCreate(handles.get(id), group_path);
            DCAttribute::writeAttribute(attrName, type.getDataType(), group.getHandle(), data);
        }
    }

    void ParallelDataCollector::read(int32_t id,
            const char* name,
            Dimensions &sizeRead,
            void* buf)
    throw (DCException)
    {
        this->read(id, name, Dimensions(0, 0, 0), Dimensions(0, 0, 0), sizeRead, buf);
    }

    void ParallelDataCollector::read(int32_t id,
            const char* name,
            Dimensions dstBuffer,
            Dimensions dstOffset,
            Dimensions &sizeRead,
            void* buf)
    throw (DCException)
    {
        if (fileStatus != FST_READING && fileStatus != FST_WRITING)
            throw DCException(getExceptionString("read", "this access is not permitted"));

        uint32_t ndims = 0;
        readCompleteDataSet(handles.get(id), id, name, dstBuffer, dstOffset,
                Dimensions(0, 0, 0), sizeRead, ndims, buf);
    }

    void ParallelDataCollector::read(int32_t id,
            const Dimensions localSize,
            const Dimensions globalOffset,
            const char* name,
            Dimensions &sizeRead,
            void* buf) throw (DCException)
    {
        this->read(id, localSize, globalOffset, name, localSize,
                Dimensions(0, 0, 0), sizeRead, buf);
    }

    void ParallelDataCollector::read(int32_t id,
            const Dimensions localSize,
            const Dimensions globalOffset,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            Dimensions &sizeRead,
            void* buf) throw (DCException)
    {
        if (fileStatus != FST_READING && fileStatus != FST_WRITING)
            throw DCException(getExceptionString("read", "this access is not permitted"));

        uint32_t ndims = 0;
        readDataSet(handles.get(id), id, name, dstBuffer, dstOffset,
                localSize, globalOffset, sizeRead, ndims, buf);
    }

    void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t ndims,
            const Dimensions srcData, const char* name, const void* data)
    throw (DCException)
    {
        Dimensions globalSize, globalOffset;
        gatherMPIWrites(ndims, srcData, globalSize, globalOffset);

        write(id, globalSize, globalOffset,
                type, ndims, srcData, Dimensions(1, 1, 1),
                srcData, Dimensions(0, 0, 0), name, data);
    }

    void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t ndims,
            const Dimensions srcBuffer, const Dimensions srcData, const Dimensions srcOffset,
            const char* name, const void* data)
    throw (DCException)
    {
        Dimensions globalSize, globalOffset;
        gatherMPIWrites(ndims, srcData, globalSize, globalOffset);

        write(id, globalSize, globalOffset,
                type, ndims, srcBuffer, Dimensions(1, 1, 1),
                srcData, srcOffset, name, data);
    }

    void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t ndims,
            const Dimensions srcBuffer, const Dimensions srcStride, const Dimensions srcData,
            const Dimensions srcOffset, const char* name, const void* buf)
    throw (DCException)
    {
        Dimensions globalSize, globalOffset;
        gatherMPIWrites(ndims, srcData, globalSize, globalOffset);

        write(id, globalSize, globalOffset,
                type, ndims, srcBuffer, srcStride, srcData,
                srcOffset, name, buf);
    }

    void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
            const Dimensions globalOffset,
            const CollectionType& type, uint32_t ndims, const Dimensions srcData,
            const char* name, const void* buf)
    {
        write(id, globalSize, globalOffset, type, ndims, srcData, Dimensions(1, 1, 1), srcData,
                Dimensions(0, 0, 0), name, buf);
    }

    void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
            const Dimensions globalOffset,
            const CollectionType& type, uint32_t ndims, const Dimensions srcBuffer,
            const Dimensions srcData, const Dimensions srcOffset, const char* name,
            const void* buf)
    {
        write(id, globalSize, globalOffset, type, ndims, srcBuffer, Dimensions(1, 1, 1),
                srcData, srcOffset, name, buf);
    }

    void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
            const Dimensions globalOffset,
            const CollectionType& type, uint32_t ndims, const Dimensions srcBuffer,
            const Dimensions srcStride, const Dimensions srcData,
            const Dimensions srcOffset, const char* name, const void* buf)
    {
        if (name == NULL)
            throw DCException(getExceptionString("write", "parameter name is NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("write", "this access is not permitted"));

        if (ndims < 1 || ndims > 3)
            throw DCException(getExceptionString("write", "maximum dimension is invalid"));

        // create group for this id/iteration
        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.openCreate(handles.get(id), group_path);

        // write data to the group
        writeDataSet(group.getHandle(), globalSize, globalOffset, type, ndims,
                srcBuffer, srcStride, srcData, srcOffset, dset_name.c_str(), buf);
    }

    void ParallelDataCollector::reserve(int32_t id,
            const Dimensions globalSize,
            uint32_t ndims,
            const CollectionType& type,
            const char* name) throw (DCException)
    {
        if (name == NULL)
            throw DCException(getExceptionString("reserve", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("write", "this access is not permitted"));

        if (ndims < 1 || ndims > 3)
            throw DCException(getExceptionString("write", "maximum dimension is invalid"));

        reserveInternal(id, globalSize, ndims, type, name);
    }

    void ParallelDataCollector::reserve(int32_t id,
            const Dimensions size,
            Dimensions *globalSize,
            Dimensions *globalOffset,
            uint32_t ndims,
            const CollectionType& type,
            const char* name) throw (DCException)
    {
        if (name == NULL)
            throw DCException(getExceptionString("reserve", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("write", "this access is not permitted"));

        if (ndims < 1 || ndims > 3)
            throw DCException(getExceptionString("write", "maximum dimension is invalid"));

        Dimensions global_size, global_offset;
        gatherMPIWrites(ndims, size, global_size, global_offset);

        reserveInternal(id, global_size, ndims, type, name);

        if (globalSize)
            globalSize->set(global_size);

        if (globalOffset)
            globalOffset->set(global_offset);
    }

    void ParallelDataCollector::append(int32_t id,
            const Dimensions size,
            uint32_t ndims,
            const Dimensions globalOffset,
            const char *name,
            const void *buf)
    {
        if (name == NULL)
            throw DCException(getExceptionString("append", "parameter name is NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("append", "this access is not permitted"));

        if (ndims < 1 || ndims > 3)
            throw DCException(getExceptionString("append", "maximum dimension is invalid"));

        // create group for this id/iteration
        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.open(handles.get(id), group_path);

        // write data to the dataset
        DCParallelDataSet dataset(dset_name.c_str());
        dataset.setWriteIndependent();

        if (!dataset.open(group.getHandle()))
        {
            throw DCException(getExceptionString("append",
                    "Cannot open dataset (missing reserve?)", dset_name.c_str()));
        } else
            dataset.write(size, Dimensions(1, 1, 1), Dimensions(0, 0, 0), size, globalOffset, buf);

        dataset.close();
    }

    void ParallelDataCollector::remove(int32_t id)
    throw (DCException)
    {
        log_msg(1, "removing group %d", id);

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        DCParallelGroup::remove(handles.get(id), group_id_name.str());

        // update maxID
        getMaxID();
    }

    void ParallelDataCollector::remove(int32_t id, const char* name)
    throw (DCException)
    {
        log_msg(1, "removing dataset %s from group %d", name, id);

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        if (name == NULL)
            throw DCException(getExceptionString("remove", "a parameter was NULL"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.open(handles.get(id), group_path);

        if (H5Ldelete(group.getHandle(), dset_name.c_str(), H5P_LINK_ACCESS_DEFAULT) < 0)
        {
            throw DCException(getExceptionString("remove", "failed to remove dataset", name));
        }
    }

    void ParallelDataCollector::createReference(int32_t srcID,
            const char *srcName,
            int32_t dstID,
            const char *dstName)
    throw (DCException)
    {
        if (srcName == NULL || dstName == NULL)
            throw DCException(getExceptionString("createReference", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("createReference", "this access is not permitted"));

        if (srcID != dstID)
            throw DCException(getExceptionString("createReference",
                "source and destination ID must be identical", NULL));

        if (srcName == dstName)
            throw DCException(getExceptionString("createReference",
                "a reference must not be identical to the referenced data", srcName));

        // open source group
        std::string src_group_path, src_dset_name, dst_dset_name;
        DCDataSet::getFullDataPath(srcName, SDC_GROUP_DATA, srcID, src_group_path, src_dset_name);
        DCDataSet::getFullDataPath(dstName, SDC_GROUP_DATA, dstID, src_group_path, dst_dset_name);

        DCParallelGroup src_group;
        src_group.open(handles.get(srcID), src_group_path);

        // open source dataset
        DCParallelDataSet src_dataset(src_dset_name.c_str());
        src_dataset.open(src_group.getHandle());

        DCParallelDataSet dst_dataset(dst_dset_name.c_str());
        // create the actual reference as a new dataset
        // identical src and dst groups
        dst_dataset.createReference(src_group.getHandle(), src_group.getHandle(), src_dataset);

        dst_dataset.close();
        src_dataset.close();
    }

    void ParallelDataCollector::createReference(int32_t srcID,
            const char *srcName,
            int32_t dstID,
            const char *dstName,
            Dimensions /*count*/,
            Dimensions /*offset*/,
            Dimensions /*stride*/)
    throw (DCException)
    {
        if (srcName == NULL || dstName == NULL)
            throw DCException(getExceptionString("createReference", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
            throw DCException(getExceptionString("createReference", "this access is not permitted"));

        if (srcID != dstID)
            throw DCException(getExceptionString("createReference",
                "source and destination ID must be identical", NULL));

        if (srcName == dstName)
            throw DCException(getExceptionString("createReference",
                "a reference must not be identical to the referenced data", srcName));

        throw DCException(getExceptionString("createReference",
                "feature currently not supported by Parallel HDF5", NULL));
    }

    /*******************************************************************************
     * PROTECTED FUNCTIONS
     *******************************************************************************/

    void ParallelDataCollector::fileCreateCallback(H5Handle handle, uint32_t index, void *userData)
    throw (DCException)
    {
        Options *options = (Options*) userData;

        // the custom group holds user-specified attributes
        DCParallelGroup group;
        group.create(handle, SDC_GROUP_CUSTOM);
        group.close();

        // the data group holds the actual simulation data
        group.create(handle, SDC_GROUP_DATA);
        group.close();

        writeHeader(handle, index, options->enableCompression, options->mpiTopology);
    }

    void ParallelDataCollector::fileOpenCallback(H5Handle /*handle*/, uint32_t index, void *userData)
    throw (DCException)
    {
        Options *options = (Options*) userData;

        options->maxID = std::max(options->maxID, (int32_t) index);
    }

    void ParallelDataCollector::writeHeader(hid_t fHandle, uint32_t id,
            bool enableCompression, Dimensions mpiTopology)
    throw (DCException)
    {
        // create group for header information (position, grid size, ...)
        DCParallelGroup group;
        group.create(fHandle, SDC_GROUP_HEADER);

        ColTypeDim dim_t;

        int32_t index = id;
        bool compression = enableCompression;

        // create master specific header attributes
        DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, H5T_NATIVE_INT32,
                group.getHandle(), &index);

        DCAttribute::writeAttribute(SDC_ATTR_COMPRESSION, H5T_NATIVE_HBOOL,
                group.getHandle(), &compression);

        DCAttribute::writeAttribute(SDC_ATTR_MPI_SIZE, dim_t.getDataType(),
                group.getHandle(), mpiTopology.getPointer());
    }

    void ParallelDataCollector::openCreate(const char *filename,
            FileCreationAttr& /*attr*/)
    throw (DCException)
    {
        this->fileStatus = FST_CREATING;

        // filters are currently not supported by parallel HDF5
        //this->options.enableCompression = attr.enableCompression;

        log_msg(1, "compression = 0");

        options.maxID = -1;

        // open file
        handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_TRUNC);
    }

    void ParallelDataCollector::openRead(const char* filename, FileCreationAttr& /*attr*/)
    throw (DCException)
    {
        this->fileStatus = FST_READING;

        getMaxID();

        handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_RDONLY);
    }

    void ParallelDataCollector::openWrite(const char* filename, FileCreationAttr& /*attr*/)
    throw (DCException)
    {
        this->fileStatus = FST_WRITING;

        getMaxID();

        // filters are currently not supported by parallel HDF5
        //this->options.enableCompression = attr.enableCompression;

        handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_RDWR);
    }

    void ParallelDataCollector::readCompleteDataSet(H5Handle h5File,
            int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            const Dimensions srcOffset,
            Dimensions &sizeRead,
            uint32_t& srcRank,
            void* dst)
    throw (DCException)
    {
        log_msg(2, "readCompleteDataSet");

        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("readCompleteDataSet", "invalid parameters"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.open(h5File, group_path);

        DCParallelDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());
        const Dimensions src_size(dataset.getSize() - srcOffset);

        dataset.read(dstBuffer, dstOffset, src_size, srcOffset, sizeRead, srcRank, dst);
        dataset.close();
    }

    void ParallelDataCollector::readDataSet(H5Handle h5File,
            int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            const Dimensions srcSize,
            const Dimensions srcOffset,
            Dimensions &sizeRead,
            uint32_t& srcRank,
            void* dst)
    throw (DCException)
    {
        log_msg(2, "readDataSet");

        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("readDataSet", "invalid parameters"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.open(h5File, group_path);

        DCParallelDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());

        dataset.read(dstBuffer, dstOffset, srcSize, srcOffset, sizeRead, srcRank, dst);
        dataset.close();
    }

    void ParallelDataCollector::writeDataSet(H5Handle group, const Dimensions globalSize,
            const Dimensions globalOffset,
            const CollectionType& datatype, uint32_t ndims,
            const Dimensions srcBuffer, const Dimensions srcStride,
            const Dimensions srcData, const Dimensions srcOffset,
            const char* name, const void* data) throw (DCException)
    {
        log_msg(2, "writeDataSet");

        DCParallelDataSet dataset(name);
        // always create dataset but write data only if all dimensions > 0
        dataset.create(datatype, group, globalSize, ndims, this->options.enableCompression);
        dataset.write(srcBuffer, srcStride, srcOffset, srcData, globalOffset, data);
        dataset.close();
    }

    void ParallelDataCollector::gatherMPIWrites(int ndims, const Dimensions localSize,
            Dimensions &globalSize, Dimensions &globalOffset)
    throw (DCException)
    {
        uint64_t write_sizes[options.mpiSize * 3];
        uint64_t local_write_size[3] = {localSize[0], localSize[1], localSize[2]};

        globalSize.set(1, 1, 1);
        globalOffset.set(0, 0, 0);

        if (MPI_Allgather(local_write_size, 3, MPI_UNSIGNED_LONG_LONG,
                write_sizes, 3, MPI_UNSIGNED_LONG_LONG, options.mpiComm) != MPI_SUCCESS)
            throw DCException(getExceptionString("gatherMPIWrites",
                "MPI_Allgather failed", NULL));

        Dimensions tmp_mpi_topology(options.mpiTopology);
        Dimensions tmp_mpi_pos(options.mpiPos);
        if (ndims == 1)
        {
            tmp_mpi_topology.set(options.mpiTopology.getScalarSize(), 1, 1);
            tmp_mpi_pos.set(options.mpiRank, 0, 0);
        }

        if ((ndims == 2) && (tmp_mpi_topology[2] > 1))
        {
            throw DCException(getExceptionString("gatherMPIWrites",
                    "cannot auto-detect global size/offset for 2D data when writing with 3D topology", NULL));
        }

        for (int i = 0; i < ndims; ++i)
        {
            globalSize[i] = 0;
            size_t index;
            for (size_t dim = 0; dim < tmp_mpi_topology[i]; ++dim)
            {
                switch (i)
                {
                    case 0:
                        index = dim;
                        break;
                    case 1:
                        index = dim * tmp_mpi_topology[0];
                        break;
                    default:
                        index = dim * tmp_mpi_topology[0] * tmp_mpi_topology[1];
                }

                globalSize[i] += write_sizes[index * 3 + i];
                if (dim < tmp_mpi_pos[i])
                    globalOffset[i] += write_sizes[index * 3 + i];
            }
        }
    }

    size_t ParallelDataCollector::getNDims(H5Handle h5File,
            int32_t id, const char* name)
    {
        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("getNDims", "invalid parameters"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.open(h5File, group_path);

        size_t ndims = 0;

        DCParallelDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());

        ndims = dataset.getNDims();

        dataset.close();

        return ndims;
    }

    void ParallelDataCollector::reserveInternal(int32_t id,
            const Dimensions globalSize,
            uint32_t ndims,
            const CollectionType& type,
            const char* name)
    throw (DCException)
    {
        log_msg(2, "reserveInternal");

        // create group for this id/iteration
        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCParallelGroup group;
        group.openCreate(handles.get(id), group_path);

        DCParallelDataSet dataset(dset_name.c_str());
        // create the empty dataset
        dataset.create(type, group.getHandle(), globalSize, ndims, this->options.enableCompression);
        dataset.close();
    }

}
