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

#include "ParallelDataCollector.hpp"
#include "core/DCParallelDataSet.hpp"
#include "core/DCAttribute.hpp"
#include "core/DCParallelGroup.hpp"

using namespace DCollector;

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
    H5Pget_cache(fileAccProperties, &metaCacheElements, &rawCacheElements, &rawCacheSize, &policy);
    rawCacheSize = 64 * 1024 * 1024;
    H5Pset_cache(fileAccProperties, metaCacheElements, rawCacheElements, rawCacheSize, policy);

    //H5Pset_chunk_cache(0, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, H5D_CHUNK_CACHE_NBYTES_DEFAULT, H5D_CHUNK_CACHE_W0_DEFAULT);

#if defined SDC_DEBUG_OUTPUT
    std::cerr << "Raw Data Cache = " << rawCacheSize / 1024 << " KiB" << std::endl;
#endif
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
{
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

    dirent *dp;

    DIR *dirp = opendir(dir_path.c_str());
    while ((dp = readdir(dirp)) != NULL)
    {
        // size matches and starts with name.c_str()
        if (strstr(dp->d_name, name.c_str()) == dp->d_name)
        {
            std::string fname;
            fname.assign(dp->d_name);
            // end with correct file extension
            if (fname.find(".h5") != fname.size() - 3)
                continue;

            // extract id from filename
            int32_t id = atoi(
                    fname.substr(name.size(), fname.size() - 3 - name.size()).c_str());
            ids.insert(id);
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
    MPI_Comm_rank(comm, &(options.mpiRank));

    options.enableCompression = false;
    options.mpiComm = comm;
    options.mpiInfo = info;
    options.mpiSize = topology.getDimSize();
    options.mpiTopology.set(topology);
    options.maxID = -1;

    if (H5open() < 0)
        throw DCException(getExceptionString("ParallelDataCollector",
            "failed to initialize/open HDF5 library"));

    // surpress automatic output of HDF5 exception messages
    if (H5Eset_auto2(H5E_DEFAULT, NULL, NULL) < 0)
        throw DCException(getExceptionString("ParallelDataCollector",
            "failed to disable error printing"));

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
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "opening parallel data collector..." << std::endl;
#endif

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
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "closing parallel data collector..." << std::endl;
#endif   

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

    DCParallelGroup::getEntriesInternal(group.getHandle(), group_id_name.str(), &param);

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
        std::cerr << e.what() << std::endl;
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
    if (attrName == NULL || dataName == NULL || data == NULL)
        throw DCException(getExceptionString("readAttribute", "a parameter was null"));

    if (fileStatus == FST_CLOSED)
        throw DCException(getExceptionString("readAttribute", "this access is not permitted"));

    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(dataName, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.open(handles.get(id), group_path);

    hid_t dataset_id = -1;
    dataset_id = H5Dopen(group.getHandle(), dset_name.c_str(), H5P_DEFAULT);
    if (dataset_id < 0)
    {
        throw DCException(getExceptionString("readAttribute",
                "dataset not found", dset_name.c_str()));
    }

    try
    {
        DCAttribute::readAttribute(attrName, dataset_id, data);
    } catch (DCException)
    {
        H5Dclose(dataset_id);
        throw;
    }
    H5Dclose(dataset_id);
}

void ParallelDataCollector::writeAttribute(int32_t id,
        const CollectionType& type,
        const char *dataName,
        const char *attrName,
        const void* data)
throw (DCException)
{
    if (attrName == NULL || dataName == NULL || data == NULL)
        throw DCException(getExceptionString("writeAttribute", "a parameter was null"));

    if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
        throw DCException(getExceptionString("writeAttribute", "this access is not permitted"));

    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(dataName, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.open(handles.get(id), group_path);

    hid_t dataset_id = H5Dopen(group.getHandle(), dset_name.c_str(), H5P_DEFAULT);
    if (dataset_id < 0)
    {
        throw DCException(getExceptionString("writeAttribute",
                "dataset not found", dset_name.c_str()));
    }

    try
    {
        DCAttribute::writeAttribute(attrName, type.getDataType(), dataset_id, data);
    } catch (DCException)
    {
        H5Dclose(dataset_id);
        throw;
    }
    H5Dclose(dataset_id);
}

void ParallelDataCollector::read(int32_t id,
        const CollectionType& type,
        const char* name,
        Dimensions &sizeRead,
        void* data)
throw (DCException)
{
    this->read(id, type, name, Dimensions(0, 0, 0), sizeRead, Dimensions(0, 0, 0), data);
}

void ParallelDataCollector::read(int32_t id,
        const CollectionType& /*type*/,
        const char* name,
        Dimensions dstBuffer,
        Dimensions &sizeRead,
        Dimensions dstOffset,
        void* data)
throw (DCException)
{
    if (fileStatus != FST_READING && fileStatus != FST_WRITING)
        throw DCException(getExceptionString("read", "this access is not permitted"));

    uint32_t rank = 0;
    readDataSet(handles.get(id), id, name, false, dstBuffer, dstOffset,
            Dimensions(0, 0, 0), Dimensions(0, 0, 0), sizeRead, rank, data);
}

void ParallelDataCollector::read(int32_t id,
        const Dimensions localSize,
        const Dimensions globalOffset,
        const CollectionType& type,
        const char* name,
        Dimensions &sizeRead,
        void* data) throw (DCException)
{
    this->read(id, localSize, globalOffset, type, name, localSize,
            sizeRead, Dimensions(0, 0, 0), data);
}

void ParallelDataCollector::read(int32_t id,
        const Dimensions localSize,
        const Dimensions globalOffset,
        const CollectionType& /*type*/,
        const char* name,
        const Dimensions dstBuffer,
        Dimensions &sizeRead,
        const Dimensions dstOffset,
        void* data) throw (DCException)
{
    if (fileStatus != FST_READING && fileStatus != FST_WRITING)
        throw DCException(getExceptionString("read", "this access is not permitted"));

    uint32_t rank = 0;
    readDataSet(handles.get(id), id, name, true, dstBuffer, dstOffset,
            localSize, globalOffset, sizeRead, rank, data);
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcData, const char* name, const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset;
    gatherMPIWrites(rank, srcData, globalSize, globalOffset);

    write(id, globalSize, globalOffset,
            type, rank, srcData, Dimensions(1, 1, 1),
            srcData, Dimensions(0, 0, 0), name, data);
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcBuffer, const Dimensions srcData, const Dimensions srcOffset,
        const char* name, const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset;
    gatherMPIWrites(rank, srcData, globalSize, globalOffset);

    write(id, globalSize, globalOffset,
            type, rank, srcBuffer, Dimensions(1, 1, 1),
            srcData, srcOffset, name, data);
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcBuffer, const Dimensions srcStride, const Dimensions srcData,
        const Dimensions srcOffset, const char* name, const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset;
    gatherMPIWrites(rank, srcData, globalSize, globalOffset);

    write(id, globalSize, globalOffset,
            type, rank, srcBuffer, srcStride, srcData,
            srcOffset, name, data);
}

void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type, uint32_t rank, const Dimensions srcData,
        const char* name, const void* data)
{
    write(id, globalSize, globalOffset, type, rank, srcData, Dimensions(1, 1, 1), srcData,
            Dimensions(0, 0, 0), name, data);
}

void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type, uint32_t rank, const Dimensions srcBuffer,
        const Dimensions srcData, const Dimensions srcOffset, const char* name,
        const void* data)
{
    write(id, globalSize, globalOffset, type, rank, srcBuffer, Dimensions(1, 1, 1),
            srcData, srcOffset, name, data);
}

void ParallelDataCollector::write(int32_t id, const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type, uint32_t rank, const Dimensions srcBuffer,
        const Dimensions srcStride, const Dimensions srcData,
        const Dimensions srcOffset, const char* name, const void* data)
{
    if (name == NULL || data == NULL)
        throw DCException(getExceptionString("write", "a parameter was NULL"));

    if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
        throw DCException(getExceptionString("write", "this access is not permitted"));

    if (rank < 1 || rank > 3)
        throw DCException(getExceptionString("write", "maximum dimension is invalid"));

    // create group for this id/iteration
    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.openCreate(handles.get(id), group_path);

    // write data to the group
    try
    {
        writeDataSet(group.getHandle(), globalSize, globalOffset, type, rank,
                srcBuffer, srcStride, srcData, srcOffset, dset_name.c_str(), data);
    } catch (DCException)
    {
        throw;
    }
}

void ParallelDataCollector::reserve(int32_t id,
        const Dimensions size,
        Dimensions *globalSize,
        Dimensions *globalOffset,
        uint32_t rank,
        const CollectionType& type,
        const char* name) throw (DCException)
{
    if (name == NULL)
        throw DCException(getExceptionString("reserve", "a parameter was NULL"));

    if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
        throw DCException(getExceptionString("write", "this access is not permitted"));

    if (rank < 1 || rank > 3)
        throw DCException(getExceptionString("write", "maximum dimension is invalid"));

    Dimensions global_size, global_offset;
    gatherMPIWrites(rank, size, global_size, global_offset);

    // create group for this id/iteration
    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.openCreate(handles.get(id), group_path);

    DCParallelDataSet dataset(dset_name.c_str());
    // create the empty dataset
    dataset.create(type, group.getHandle(), global_size, rank, this->options.enableCompression);
    dataset.close();

    if (globalSize)
        globalSize->set(global_size);

    if (globalOffset)
        globalOffset->set(global_offset);
}

void ParallelDataCollector::append(int32_t id,
        const Dimensions size,
        const CollectionType& /*type*/,
        uint32_t rank,
        const Dimensions globalOffset,
        const char *name,
        const void *data)
{
    if (name == NULL || data == NULL)
        throw DCException(getExceptionString("append", "a parameter was NULL"));

    if (fileStatus == FST_CLOSED || fileStatus == FST_READING)
        throw DCException(getExceptionString("append", "this access is not permitted"));

    if (rank < 1 || rank > 3)
        throw DCException(getExceptionString("append", "maximum dimension is invalid"));

    // create group for this id/iteration
    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.openCreate(handles.get(id), group_path);

    // write data to the dataset
    try
    {
        DCParallelDataSet dataset(dset_name.c_str());
        dataset.setWriteIndependent();

        if (!dataset.open(group.getHandle()))
        {
            throw DCException(getExceptionString("append",
                    "Cannot open dataset (missing reserve?)", dset_name.c_str()));
        } else
            dataset.write(size, Dimensions(1, 1, 1), Dimensions(0, 0, 0), size, globalOffset, data);

        dataset.close();
    } catch (DCException)
    {
        throw;
    }
}

void ParallelDataCollector::remove(int32_t id)
throw (DCException)
{
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "removing group " << id << std::endl;
#endif

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
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "removing dataset " << name << " from group " << id << std::endl;
#endif

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
    try
    {
        DCParallelDataSet src_dataset(src_dset_name.c_str());
        src_dataset.open(src_group.getHandle());

        DCParallelDataSet dst_dataset(dst_dset_name.c_str());
        // create the actual reference as a new dataset
        // identical src and dst groups
        dst_dataset.createReference(src_group.getHandle(), src_group.getHandle(), src_dataset);

        dst_dataset.close();
        src_dataset.close();

    } catch (DCException e)
    {
        throw e;
    }
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

    try
    {
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

    } catch (DCException attr_error)
    {
        throw DCException(getExceptionString("writeHeader",
                "Failed to write header attribute.",
                attr_error.what()));
    }
}

void ParallelDataCollector::openCreate(const char *filename,
        FileCreationAttr& /*attr*/)
throw (DCException)
{
    this->fileStatus = FST_CREATING;

    // filters are currently not supported by parallel HDF5
    //this->options.enableCompression = attr.enableCompression;

#if defined SDC_DEBUG_OUTPUT
    if (attr.enableCompression)
        std::cerr << "compression is ON" << std::endl;
    else
        std::cerr << "compression is OFF" << std::endl;
#endif
    
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

void ParallelDataCollector::readDataSet(H5Handle h5File,
        int32_t id,
        const char* name,
        bool parallelRead,
        const Dimensions dstBuffer,
        const Dimensions dstOffset,
        const Dimensions srcSize,
        const Dimensions srcOffset,
        Dimensions &sizeRead,
        uint32_t& srcRank,
        void* dst)
throw (DCException)
{
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "# ParallelDataCollector::readInternal #" << std::endl;
#endif

    if (h5File < 0 || name == NULL)
        throw DCException(getExceptionString("readInternal", "invalid parameters"));

    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.open(h5File, group_path);

    Dimensions src_size(srcSize);
    Dimensions src_offset(srcOffset);

    try
    {
        DCParallelDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());
        if (!parallelRead && (src_size.getDimSize() == 0))
        {
            dataset.read(dstBuffer, dstOffset, src_size, src_offset, sizeRead, srcRank, NULL);
            src_size.set(sizeRead);
            src_offset.set(0, 0, 0);
        }

        dataset.read(dstBuffer, dstOffset, src_size, src_offset, sizeRead, srcRank, dst);
        dataset.close();
    } catch (DCException e)
    {
        throw e;
    }
}

void ParallelDataCollector::writeDataSet(H5Handle group, const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& datatype, uint32_t rank,
        const Dimensions srcBuffer, const Dimensions srcStride,
        const Dimensions srcData, const Dimensions srcOffset,
        const char* name, const void* data) throw (DCException)
{
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "# ParallelDataCollector::writeDataSet #" << std::endl;
#endif
    DCParallelDataSet dataset(name);
    // always create dataset but write data only if all dimensions > 0
    dataset.create(datatype, group, globalSize, rank, this->options.enableCompression);
    if (srcData.getDimSize() > 0)
        dataset.write(srcBuffer, srcStride, srcOffset, srcData, globalOffset, data);
    dataset.close();
}

void ParallelDataCollector::gatherMPIWrites(int rank, const Dimensions localSize,
        Dimensions &globalSize, Dimensions &globalOffset)
throw (DCException)
{
    uint64_t write_sizes[options.mpiSize * 3];
    uint64_t local_write_size[3] = {localSize[0], localSize[1], localSize[2]};

    globalSize.set(1, 1, 1);
    globalOffset.set(0, 0, 0);

    if (MPI_Allgather(local_write_size, 3, MPI_INTEGER8,
            write_sizes, 3, MPI_INTEGER8, options.mpiComm) != MPI_SUCCESS)
        throw DCException(getExceptionString("gatherMPIWrites",
            "MPI_Allgather failed", NULL));

    Dimensions tmp_mpi_topology(options.mpiTopology);
    Dimensions tmp_mpi_pos(options.mpiPos);
    if (rank == 1)
    {
        tmp_mpi_topology.set(options.mpiTopology.getDimSize(), 1, 1);
        tmp_mpi_pos.set(options.mpiRank, 0, 0);
    }

    if ((rank == 2) && (tmp_mpi_topology[2] > 1))
    {
        throw DCException(getExceptionString("gatherMPIWrites",
                "cannot auto-detect global size/offset for 2D data when writing with 3D topology", NULL));
    }

    for (int i = 0; i < rank; ++i)
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

size_t ParallelDataCollector::getRank(H5Handle h5File,
        int32_t id, const char* name)
{
#if defined SDC_DEBUG_OUTPUT
    std::cerr << "# ParallelDataCollector::getRank #" << std::endl;
#endif

    if (h5File < 0 || name == NULL)
        throw DCException(getExceptionString("getRank", "invalid parameters"));

    std::string group_path, dset_name;
    DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

    DCParallelGroup group;
    group.open(h5File, group_path);

    size_t rank = 0;

    try
    {
        DCParallelDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());

        rank = dataset.getRank();

        dataset.close();
    } catch (DCException e)
    {
        throw e;
    }

    return rank;
}
