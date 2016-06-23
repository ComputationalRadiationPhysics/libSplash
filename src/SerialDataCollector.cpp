/**
 * Copyright 2013-2016 Felix Schmitt, Axel Huebl, Alexander Grund
 *
 * This file is part of libSplash.
 *
 * libSplash is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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



#include <cstring>
#include <time.h>
#include <stdlib.h>
#include <cassert>
#include <sys/stat.h>

#include "splash/SerialDataCollector.hpp"
#include "splash/core/DCAttribute.hpp"
#include "splash/core/DCDataSet.hpp"
#include "splash/core/DCGroup.hpp"
#include "splash/core/SDCHelper.hpp"
#include "splash/core/logging.hpp"
#include "splash/core/H5IdWrapper.hpp"
#include "splash/basetypes/basetypes.hpp"

namespace splash
{

    /*******************************************************************************
     * PRIVATE FUNCTIONS
     *******************************************************************************/

    void SerialDataCollector::setFileAccessParams(hid_t& fileAccProperties)
    {
        fileAccProperties = H5P_FILE_ACCESS_DEFAULT;

        int metaCacheElements = 0;
        size_t rawCacheElements = 0;
        size_t rawCacheSize = 0;
        double policy = 0.0;

        // set new cache size
        H5Pget_cache(fileAccProperties, &metaCacheElements, &rawCacheElements, &rawCacheSize, &policy);
        rawCacheSize = 256 * 1024 * 1024;
        H5Pset_cache(fileAccProperties, metaCacheElements, rawCacheElements, rawCacheSize, policy);

        log_msg(3, "Raw Data Cache (File) = %llu KiB", (long long unsigned) (rawCacheSize / 1024));
    }

    bool SerialDataCollector::fileExists(std::string filename)
    {
        struct stat fileInfo;
        return (stat(filename.c_str(), &fileInfo) == 0);
    }

    std::string SerialDataCollector::getFullFilename(const Dimensions mpiPos, std::string baseFilename,
            bool isFullNameAllowed) const throw (DCException)
    {
        // Check for existing extension
        if (baseFilename.find(".h5") == baseFilename.length() - 3)
        {
            if (isFullNameAllowed)
                return baseFilename;
            else
                throw DCException("Full filename is not allowed!");
        }

        std::stringstream serial_filename;
        serial_filename << baseFilename << "_" << mpiPos[0] << "_" << mpiPos[1] <<
                "_" << mpiPos[2] << ".h5";

        return serial_filename.str();
    }

    std::string SerialDataCollector::getExceptionString(std::string func, std::string msg,
            const char *info)
    {
        std::stringstream full_msg;
        full_msg << "Exception for SerialDataCollector::" << func <<
                ": " << msg;

        if (info != NULL)
            full_msg << " (" << info << ")";

        return full_msg.str();
    }

    /*******************************************************************************
     * PUBLIC FUNCTIONS
     *******************************************************************************/

    SerialDataCollector::SerialDataCollector(uint32_t maxFileHandles) :
    handles(maxFileHandles, HandleMgr::FNS_FULLNAME),
    fileStatus(FST_CLOSED),
    maxID(-1),
    mpiTopology(1, 1, 1)
    {
#ifdef COL_TYPE_CPP
        throw DCException("Check your defines !");
#endif

        parseEnvVars();

        if (H5open() < 0)
            throw DCException(getExceptionString("SerialDataCollector",
                "failed to initialize/open HDF5 library"));

#ifndef SPLASH_VERBOSE_HDF5
        // Suppress automatic output of HDF5 exception messages
        if (H5Eset_auto2(H5E_DEFAULT, NULL, NULL) < 0)
            throw DCException(getExceptionString("SerialDataCollector",
                "failed to disable error printing"));
#endif

        // set some default file access parameters
        setFileAccessParams(fileAccProperties);
    }

    SerialDataCollector::~SerialDataCollector()
    {
        close();
    }

    void SerialDataCollector::open(const char* filename, FileCreationAttr &attr)
    throw (DCException)
    {
        log_msg(1, "opening serial data collector");

        if (filename == NULL)
            throw DCException(getExceptionString("open", "filename must not be null"));

        if (fileStatus != FST_CLOSED)
            throw DCException(getExceptionString("open", "this access is not permitted"));

        switch (attr.fileAccType)
        {
            case FAT_READ:
                openRead(filename, attr);
                break;
            case FAT_WRITE:
                openWrite(filename, attr);
                break;
            case FAT_CREATE:
                openCreate(filename, attr);
                break;
            case FAT_READ_MERGED:
                openMerge(filename);
                break;
        }
    }

    void SerialDataCollector::close()
    {
        if (fileStatus == FST_CLOSED)
            return;

        log_msg(1, "closing serial data collector");

        if ((fileStatus == FST_CREATING || fileStatus == FST_WRITING) &&
            maxID >= 0)
        {
            // write number of iterations
            try
            {
                DCGroup group;
                group.open(handles.get(0), SDC_GROUP_HEADER);
                ColTypeInt32 ctInt32;
                DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, ctInt32.getDataType(),
                        group.getHandle(), &maxID);
            } catch (const DCException& e)
            {
                log_msg(0, "Exception: %s", e.what());
                log_msg(1, "continuing...");
            }
        }

        maxID = -1;
        mpiTopology.set(1, 1, 1);

        // close opened hdf5 file handles
        handles.close();

        fileStatus = FST_CLOSED;
    }

    void SerialDataCollector::openCustomGroup(DCGroup& group,
                    Dimensions *mpiPosition) throw (DCException)
    {
        // Note: Factored out from readGlobalAttribute

        if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
            throw DCException(getExceptionString("openCustomGroup", "this access is not permitted"));

        std::stringstream group_custom_name;
        if (mpiPosition == NULL || fileStatus == FST_MERGING)
            group_custom_name << SDC_GROUP_CUSTOM;
        else
            group_custom_name << SDC_GROUP_CUSTOM << "_" <<
                (*mpiPosition)[0] << "_" << (*mpiPosition)[1] << "_" << (*mpiPosition)[2];
        const std::string custom_string = group_custom_name.str();


        uint32_t mpi_rank = 0;
        if (fileStatus == FST_MERGING)
        {
            Dimensions mpi_pos(0, 0, 0);
            if (mpiPosition != NULL)
                mpi_pos = *mpiPosition;

            mpi_rank = mpi_pos[2] * mpiTopology[0] * mpiTopology[1] + mpi_pos[1] * mpiTopology[0] + mpi_pos[0];
        }

        group.open(handles.get(mpi_rank), custom_string.c_str());
    }

    hid_t SerialDataCollector::openGroup(DCGroup& group, int32_t id, const char* dataName,
            Dimensions *mpiPosition) throw (DCException)
    {
        // Note: Factored out from readAttribute

        // dataName may be NULL, attribute is read to iteration group in that case
        if (dataName && strlen(dataName) == 0)
            throw DCException(getExceptionString("openGroup", "empty dataset name"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
            throw DCException(getExceptionString("openGroup", "this access is not permitted"));

        std::string group_path, obj_name;
        std::string dataNameInternal = "";
        if (dataName)
            dataNameInternal.assign(dataName);
        DCDataSet::getFullDataPath(dataNameInternal, SDC_GROUP_DATA, id, group_path, obj_name);

        Dimensions mpi_pos(0, 0, 0);
        if ((fileStatus == FST_MERGING) && (mpiPosition != NULL))
        {
            mpi_pos.set(*mpiPosition);
        }

        group.open(handles.get(mpi_pos), group_path);

        if (dataName)
        {
            // read from the dataset or group
            if (H5Lexists(group.getHandle(), obj_name.c_str(), H5P_LINK_ACCESS_DEFAULT))
                return H5Oopen(group.getHandle(), obj_name.c_str(), H5P_DEFAULT);
            else
            {
                throw DCException(getExceptionString("openGroup",
                        "dataset not found", obj_name.c_str()));
            }
        } else
            return -1;
    }

    DCAttributeInfo* SerialDataCollector::readGlobalAttributeMeta(
            int32_t /*id*/,
            const char* name,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (name == NULL)
            throw DCException(getExceptionString("readGlobalAttributeMeta", "a parameter was null"));

        DCGroup group_custom;
        openCustomGroup(group_custom, mpiPosition);

        return DCAttribute::readAttributeInfo(name, group_custom.getHandle());
    }

    void SerialDataCollector::readGlobalAttribute(
            const char* name,
            void* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("readGlobalAttribute", "a parameter was null"));

        DCGroup group_custom;
        openCustomGroup(group_custom, mpiPosition);

        try
        {
            DCAttribute::readAttribute(name, group_custom.getHandle(), data);
        } catch (const DCException&)
        {
            throw DCException(getExceptionString("readGlobalAttribute", "failed to open attribute", name));
        }
    }

    void SerialDataCollector::writeGlobalAttribute(const CollectionType& type,
            const char* name,
            const void* data)
    throw (DCException)
    {
        const Dimensions dims(1, 1, 1);
        writeGlobalAttribute(type, name, 1u, dims, data);
    }

    void SerialDataCollector::writeGlobalAttribute(const CollectionType& type,
            const char* name,
            uint32_t ndims,
            const Dimensions dims,
            const void* data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("writeGlobalAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("writeGlobalAttribute", "this access is not permitted"));

        if (ndims < 1u || ndims > DSP_DIM_MAX)
            throw DCException(getExceptionString("writeGlobalAttribute", "maximum dimension `ndims` is invalid"));

        DCGroup group_custom;
        group_custom.open(handles.get(0), SDC_GROUP_CUSTOM);

        try
        {
            DCAttribute::writeAttribute(name, type.getDataType(), group_custom.getHandle(), ndims, dims, data);
        } catch (const DCException& e)
        {
            std::cerr << e.what() << std::endl;
            throw DCException(getExceptionString("writeGlobalAttribute", "failed to write attribute", name));
        }
    }

    DCAttributeInfo* SerialDataCollector::readAttributeMeta(int32_t id,
            const char *dataName,
            const char *attrName,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (attrName == NULL)
            throw DCException(getExceptionString("readAttributeMeta", "a parameter was null"));

        if (strlen(attrName) == 0)
            throw DCException(getExceptionString("readAttributeMeta", "empty attribute name"));

        DCGroup group;
        H5ObjectId objId(openGroup(group, id, dataName, mpiPosition));

        // When no object is returned read attribute from the iteration group
        if (!objId)
            return DCAttribute::readAttributeInfo(attrName, group.getHandle());
        else
            return DCAttribute::readAttributeInfo(attrName, objId);
    }

    void SerialDataCollector::readAttribute(int32_t id,
            const char *dataName,
            const char *attrName,
            void* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (attrName == NULL || data == NULL)
            throw DCException(getExceptionString("readAttribute", "a parameter was null"));

        if (strlen(attrName) == 0)
            throw DCException(getExceptionString("readAttribute", "empty attribute name"));

        DCGroup group;
        H5ObjectId objId(openGroup(group, id, dataName, mpiPosition));

        // When no object is returned read attribute from the iteration group
        if (!objId)
            DCAttribute::readAttribute(attrName, group.getHandle(), data);
        else
            DCAttribute::readAttribute(attrName, objId, data);
    }

    void SerialDataCollector::writeAttribute(int32_t id,
            const CollectionType& type,
            const char *dataName,
            const char *attrName,
            const void* data)
    throw (DCException)
    {
        const Dimensions dims(1, 1, 1);
        SerialDataCollector::writeAttribute( id, type, dataName, attrName,
                                             1u, dims, data);
    }

    void SerialDataCollector::writeAttribute(int32_t id,
            const CollectionType& type,
            const char *dataName,
            const char *attrName,
            uint32_t ndims,
            const Dimensions dims,
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

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("writeAttribute", "this access is not permitted"));

        if (ndims < 1u || ndims > DSP_DIM_MAX)
            throw DCException(getExceptionString("writeAttribute", "maximum dimension `ndims` is invalid"));

        /* group_path: absolute path to the last inode
         * obj_name: last inode, can be a group or a dataset
         */
        std::string group_path, obj_name;
        std::string dataNameInternal = "";
        if (dataName)
            dataNameInternal.assign(dataName);
        DCDataSet::getFullDataPath(dataNameInternal, SDC_GROUP_DATA, id, group_path, obj_name);

        DCGroup group;
        if (dataName)
        {
            /* if the specified inode (obj_name) does not exist
             * (as dataset or group), create all missing groups along group_path
             * and even create an empty group for obj_name itself
             *
             * group_path + "/" + obj_name is the absolute path of dataName
             */
            std::string pathAndName(group_path + "/" + obj_name);
            if(!DCGroup::exists(handles.get(id), pathAndName))
                group.create(handles.get(id), pathAndName);

            // attach attribute to the dataset or group
            group.open(handles.get(0), group_path);

            hid_t obj_id = H5Oopen(group.getHandle(), obj_name.c_str(), H5P_DEFAULT);
            if (obj_id < 0)
            {
                throw DCException(getExceptionString("writeAttribute",
                        "object not found", obj_name.c_str()));
            }

            try
            {
                DCAttribute::writeAttribute(attrName, type.getDataType(), obj_id, ndims, dims, data);
            } catch (const DCException&)
            {
                H5Oclose(obj_id);
                throw;
            }
            H5Oclose(obj_id);
        } else
        {
            // attach attribute to the iteration group
            group.openCreate(handles.get(0), group_path);
            DCAttribute::writeAttribute(attrName, type.getDataType(), group.getHandle(), ndims, dims, data);
        }
    }

    void SerialDataCollector::read(int32_t id,
            const char* name,
            Dimensions &sizeRead,
            void* data)
    throw (DCException)
    {
        this->read(id, name, Dimensions(0, 0, 0), Dimensions(0, 0, 0), sizeRead, data);
    }

    void SerialDataCollector::read(int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            Dimensions &sizeRead,
            void* data)
    throw (DCException)
    {
        if (fileStatus != FST_READING && fileStatus != FST_WRITING && fileStatus != FST_MERGING)
            throw DCException(getExceptionString("read", "this access is not permitted"));

        uint32_t ndims = 0;
        readCompleteDataSet(handles.get(0), id, name, dstBuffer, dstOffset,
                Dimensions(0, 0, 0), sizeRead, ndims, data);
    }

    CollectionType* SerialDataCollector::readMeta(int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            Dimensions &sizeRead)
    throw (DCException)
    {
        if (fileStatus != FST_READING && fileStatus != FST_WRITING && fileStatus != FST_MERGING)
            throw DCException(getExceptionString("readMeta", "this access is not permitted"));

        uint32_t ndims = 0;
        return readDataSetMeta(handles.get(id), id, name, dstBuffer, dstOffset,
                Dimensions(0, 0, 0), sizeRead, ndims);
    }

    void SerialDataCollector::write(int32_t id, const CollectionType& type, uint32_t ndims,
            const Selection select, const char* name, const void* data)
    throw (DCException)
    {
        if (name == NULL)
            throw DCException(getExceptionString("write", "parameter name is NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("write", "this access is not permitted"));

        if (ndims < 1 || ndims > DSP_DIM_MAX)
            throw DCException(getExceptionString("write", "maximum dimension is invalid"));

        if (id > this->maxID)
            this->maxID = id;

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.openCreate(handles.get(0), group_path);

        // write data to the group
        try
        {
            writeDataSet(group.getHandle(), type, ndims, select, dset_name.c_str(), data);
        } catch (const DCException&)
        {
            throw;
        }
    }

    void SerialDataCollector::append(int32_t id, const CollectionType& type,
            size_t count, const char* name, const void* data)
    throw (DCException)
    {
        append(id, type, count, 0, 1, name, data);
    }

    void SerialDataCollector::append(int32_t id, const CollectionType& type,
            size_t count, size_t offset, size_t stride, const char* name, const void* data)
    throw (DCException)
    {
        if (name == NULL)
            throw DCException(getExceptionString("append", "parameter name is NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("append", "this access is not permitted"));

        if (id > this->maxID)
            this->maxID = id;

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.openCreate(handles.get(0), group_path);

        // write data to the group
        try
        {
            appendDataSet(group.getHandle(), type, count, offset,
                    stride, dset_name.c_str(), data);
        } catch (const DCException&)
        {
            throw;
        }
    }

    void SerialDataCollector::remove(int32_t id)
    throw (DCException)
    {
        log_msg(1, "removing group %d", id);

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        DCGroup::remove(handles.get(0), group_id_name.str());

        // update maxID to new highest group
        maxID = 0;
        size_t num_groups = 0;
        getEntryIDs(NULL, &num_groups);

        int32_t *groups = new int32_t[num_groups];
        getEntryIDs(groups, NULL);

        for (size_t i = 0; i < num_groups; ++i)
            if (groups[i] > maxID)
                maxID = groups[i];

        delete[] groups;
        groups = NULL;
    }

    void SerialDataCollector::remove(int32_t id, const char* name)
    throw (DCException)
    {
        log_msg(1, "removing dataset %s from group %d", name, id);

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        if (name == NULL)
            throw DCException(getExceptionString("remove", "parameter name is NULL"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(handles.get(0), group_path);

        if (H5Ldelete(group.getHandle(), dset_name.c_str(), H5P_LINK_ACCESS_DEFAULT) < 0)
        {
            throw DCException(getExceptionString("remove",
                    "failed to remove dataset", dset_name.c_str()));
        }
    }

    void SerialDataCollector::createReference(int32_t srcID,
            const char *srcName,
            int32_t dstID,
            const char *dstName)
    throw (DCException)
    {
        if (srcName == NULL || dstName == NULL)
            throw DCException(getExceptionString("createReference", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("createReference", "this access is not permitted"));

        if (srcID == dstID && srcName == dstName)
            throw DCException(getExceptionString("createReference",
                "a reference must not be identical to the referenced data", srcName));

        // open source group
        std::string src_group_path, src_dset_name;
        DCDataSet::getFullDataPath(srcName, SDC_GROUP_DATA, srcID, src_group_path, src_dset_name);

        DCGroup src_group, dst_group;
        src_group.open(handles.get(0), src_group_path);

        // open destination group
        std::string dst_group_path, dst_dset_name;
        DCDataSet::getFullDataPath(dstName, SDC_GROUP_DATA, dstID, dst_group_path, dst_dset_name);

        // if destination group does not exist, it is created
        dst_group.openCreate(handles.get(0), dst_group_path);

        // open source dataset
        try
        {
            DCDataSet src_dataset(src_dset_name.c_str());
            src_dataset.open(src_group.getHandle());

            DCDataSet dst_dataset(dst_dset_name.c_str());
            // create the actual reference as a new dataset
            dst_dataset.createReference(dst_group.getHandle(),
                    src_group.getHandle(), src_dataset);

            dst_dataset.close();
            src_dataset.close();

        } catch (const DCException& e)
        {
            throw e;
        }
    }

    void SerialDataCollector::createReference(int32_t srcID,
            const char *srcName,
            int32_t dstID,
            const char *dstName,
            Dimensions count,
            Dimensions offset,
            Dimensions stride)
    throw (DCException)
    {
        if (srcName == NULL || dstName == NULL)
            throw DCException(getExceptionString("createReference", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("createReference", "this access is not permitted"));

        if (srcID == dstID && srcName == dstName)
            throw DCException(getExceptionString("createReference",
                "a reference must not be identical to the referenced data", srcName));

        // open source group
        std::string src_group_path, src_dset_name;
        DCDataSet::getFullDataPath(srcName, SDC_GROUP_DATA, srcID, src_group_path, src_dset_name);

        DCGroup src_group, dst_group;
        src_group.open(handles.get(0), src_group_path);

        // open destination group
        std::string dst_group_path, dst_dset_name;
        DCDataSet::getFullDataPath(dstName, SDC_GROUP_DATA, dstID, dst_group_path, dst_dset_name);

        // if destination group does not exist, it is created
        dst_group.openCreate(handles.get(0), dst_group_path);

        // open source dataset
        try
        {
            DCDataSet src_dataset(src_dset_name.c_str());
            src_dataset.open(src_group.getHandle());

            DCDataSet dst_dataset(dst_dset_name.c_str());
            // create the actual reference as a new dataset
            dst_dataset.createReference(dst_group.getHandle(), src_group.getHandle(),
                    src_dataset, count, offset, stride);

            dst_dataset.close();
            src_dataset.close();

        } catch (const DCException& e)
        {
            throw e;
        }
    }

    int32_t SerialDataCollector::getMaxID()
    {
        return maxID;
    }

    void SerialDataCollector::getMPISize(Dimensions& mpiSize)
    {
        mpiSize.set(this->mpiTopology);
    }

    void SerialDataCollector::getEntryIDs(int32_t* ids, size_t* count)
    throw (DCException)
    {
        DCGroup group;
        group.open(handles.get(0), SDC_GROUP_DATA);

        hsize_t data_entries = 0;
        if (H5Gget_num_objs(group.getHandle(), &data_entries) < 0)
        {
            throw DCException(getExceptionString("getEntryIDs",
                    "Failed to get entries in data group", SDC_GROUP_DATA));
        }

        if (count != NULL)
            *count = data_entries;

        if (ids != NULL)
        {
            for (size_t i = 0; i < data_entries; i++)
            {
                char *group_id_name = NULL;
                ssize_t group_id_name_len = H5Gget_objname_by_idx(group.getHandle(), i, NULL, 0);
                if (group_id_name_len < 0)
                {
                    throw DCException(getExceptionString("getEntryIDs",
                            "Failed to get object name in group", group_id_name));
                }

                group_id_name = new char[group_id_name_len + 1];
                H5Gget_objname_by_idx(group.getHandle(), i, group_id_name, group_id_name_len + 1);
                ids[i] = atoi(group_id_name);
                delete[] group_id_name;
            }
        }
    }

    void SerialDataCollector::getEntriesForID(int32_t id, DCEntry *entries, size_t *count)
    throw (DCException)
    {
        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        // open data group for id
        DCGroup group;
        group.open(handles.get(0), group_id_name.str());

        DCGroup::VisitObjCBType param;
        param.count = 0;
        param.entries = entries;

        DCGroup::getEntriesInternal(group.getHandle(), group_id_name.str(), "", &param);

        if (count)
            *count = param.count;
    }

    /*******************************************************************************
     * PROTECTED FUNCTIONS
     *******************************************************************************/

    void SerialDataCollector::openCreate(const char *filename,
            FileCreationAttr& attr)
    throw (DCException)
    {
        this->fileStatus = FST_CREATING;

        std::string full_filename = getFullFilename(attr.mpiPosition, filename,
                attr.mpiSize.getScalarSize() == 1);

        this->enableCompression = attr.enableCompression;

        log_msg(1, "compression = %d", attr.enableCompression);

        // open file
        handles.open(full_filename, fileAccProperties, H5F_ACC_TRUNC);

        this->maxID = 0;
        this->mpiTopology.set(attr.mpiSize);

        // write datatypes and header information to the file
        SDCHelper::writeHeader(handles.get(0), attr.mpiPosition, &(this->maxID),
                &(this->enableCompression), &(this->mpiTopology), false);

        // the custom group hold user-specified attributes
        DCGroup group;
        group.create(handles.get(0), SDC_GROUP_CUSTOM);
        group.close();

        // the data group holds the actual simulation data
        group.create(handles.get(0), SDC_GROUP_DATA);
        group.close();
    }

    void SerialDataCollector::openWrite(const char* filename, FileCreationAttr& attr)
    throw (DCException)
    {
        fileStatus = FST_WRITING;

        std::string full_filename = getFullFilename(attr.mpiPosition, filename,
                attr.mpiSize.getScalarSize() == 1);

        this->enableCompression = attr.enableCompression;

        if (fileExists(full_filename))
        {
            // read reference data from target file
            SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiTopology));

            handles.open(full_filename, fileAccProperties, H5F_ACC_RDWR);
        } else
        {
            openCreate(filename, attr);
        }
    }

    void SerialDataCollector::openMerge(const char* filename)
    throw (DCException)
    {
        this->fileStatus = FST_MERGING;

        // open reference file to get mpi information
        std::string full_filename = getFullFilename(Dimensions(0, 0, 0), filename, true);

        if (!fileExists(full_filename))
        {
            this->fileStatus = FST_CLOSED;
            throw DCException(getExceptionString("openMerge", "File not found.", full_filename.c_str()));
        }

        // read reference data from target file
        SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiTopology));

        // no compression for in-memory datasets
        this->enableCompression = false;

        handles.setFileNameScheme(HandleMgr::FNS_MPI);
        handles.open(mpiTopology, filename, fileAccProperties, H5F_ACC_RDONLY);
    }

    void SerialDataCollector::openRead(const char* filename, FileCreationAttr& attr)
    throw (DCException)
    {
        this->fileStatus = FST_READING;

        std::string full_filename = getFullFilename(attr.mpiPosition, filename, true);

        if (!fileExists(full_filename))
        {
            this->fileStatus = FST_CLOSED;
            throw DCException(getExceptionString("openRead", "File not found", full_filename.c_str()));
        }

        // read reference data from target file
        SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiTopology));

        handles.open(full_filename, fileAccProperties, H5F_ACC_RDONLY);
    }

    void SerialDataCollector::writeDataSet(hid_t group,
            const CollectionType& datatype,
            uint32_t ndims,
            const Selection select,
            const char* name,
            const void* data) throw (DCException)
    {
        log_msg(2, "writeDataSet");

        DCDataSet dataset(name);
        // always create dataset but write data only if all dimensions > 0 and data available
        // not extensible
        dataset.create(datatype, group, select.count, ndims,
                this->enableCompression, false);
        if (data && (select.count.getScalarSize() > 0))
            dataset.write(select, Dimensions(0, 0, 0), data);
        dataset.close();
    }

    void SerialDataCollector::appendDataSet(hid_t group, const CollectionType& datatype,
            size_t count, size_t offset, size_t stride, const char* name, const void* data)
    throw (DCException)
    {
        log_msg(2, "appendDataSet");

        DCDataSet dataset(name);

        if (!dataset.open(group))
        {
            Dimensions data_size(count, 1, 1);
            // create dataset extensible
            dataset.create(datatype, group, data_size, 1, this->enableCompression, true);

            if (count > 0)
                dataset.write(Selection(data_size,
                    Dimensions(offset + count * stride, 1, 1),
                    Dimensions(offset, 0, 0),
                    Dimensions(stride, 1, 1)),
                    Dimensions(0, 0, 0),
                    data);
        } else
            if (count > 0)
            dataset.append(count, offset, stride, data);

        dataset.close();
    }

    size_t SerialDataCollector::getNDims(H5Handle h5File,
            int32_t id,
            const char* name)
    {
        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("getNDims", "invalid parameters"));

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(h5File, group_path);

        size_t ndims = 0;

        try
        {
            DCDataSet dataset(dset_name.c_str());
            dataset.open(group.getHandle());

            ndims = dataset.getNDims();

            dataset.close();
        } catch (const DCException& e)
        {
            throw e;
        }

        return ndims;
    }

    void SerialDataCollector::readCompleteDataSet(H5Handle h5File,
            int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            const Dimensions srcOffset,
            Dimensions &sizeRead,
            uint32_t& srcDims,
            void* dst)
    throw (DCException)
    {
        log_msg(2, "readCompleteDataSet");

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(h5File, group_path);

        DCDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());
        Dimensions src_size(dataset.getSize() - srcOffset);
        dataset.read(dstBuffer, dstOffset, src_size, srcOffset, sizeRead, srcDims, dst);
        dataset.close();
    }

    void SerialDataCollector::readDataSet(H5Handle h5File,
            int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            const Dimensions srcSize,
            const Dimensions srcOffset,
            Dimensions &sizeRead,
            uint32_t& srcDims,
            void* dst)
    throw (DCException)
    {
        log_msg(2, "readDataSet");

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(h5File, group_path);

        DCDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());
        dataset.read(dstBuffer, dstOffset, srcSize, srcOffset, sizeRead, srcDims, dst);
        dataset.close();
    }

    CollectionType* SerialDataCollector::readDataSetMeta(H5Handle h5File,
            int32_t id,
            const char* name,
            const Dimensions dstBuffer,
            const Dimensions dstOffset,
            const Dimensions srcOffset,
            Dimensions &sizeRead,
            uint32_t& srcDims)
    throw (DCException)
    {
        log_msg(2, "readDataSetMeta");

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(h5File, group_path);

        DCDataSet dataset(dset_name.c_str());
        dataset.open(group.getHandle());

        size_t entrySize;
        getEntriesForID(id, NULL, &entrySize);
        std::vector<DataCollector::DCEntry> entries(entrySize);

        getEntriesForID(id, &(*entries.begin()), NULL);

        // find entry by name
        int32_t entry_id = -1;
        for(size_t i = 0; i < entrySize; ++i)
            if(std::string(name) == entries[i].name)
            {
                entry_id = int32_t(i);
                break;
            }

        if(entry_id < 0)
            throw DCException(getExceptionString("readDataSetMeta", "Entry not found by name"));

        Dimensions src_size(dataset.getSize() - srcOffset);
        dataset.read(dstBuffer, dstOffset, src_size, srcOffset, sizeRead, srcDims, NULL);
        dataset.close();

        log_msg(3, "Entry '%s' (%d) is of type: %s",
                entries[entry_id].name.c_str(),
                entry_id,
                entries[entry_id].colType->toString().c_str());

        return entries[entry_id].colType;
    }

    void SerialDataCollector::readSizeInternal(H5Handle h5File,
            int32_t id,
            const char* name,
            Dimensions &sizeRead)
    throw (DCException)
    {
        log_msg(2, "readSizeInternal");

        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(name, SDC_GROUP_DATA, id, group_path, dset_name);

        DCGroup group;
        group.open(h5File, group_path);

        try
        {
            DCDataSet dataset(dset_name.c_str());
            dataset.open(group.getHandle());
            sizeRead.set(dataset.getSize());
            dataset.close();
        } catch (const DCException& e)
        {
            throw e;
        }
    }

    hid_t SerialDataCollector::openDatasetHandle(int32_t id,
            const char *dsetName,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        std::string group_path, dset_name;
        DCDataSet::getFullDataPath(dsetName, SDC_GROUP_DATA, id, group_path, dset_name);

        Dimensions mpi_pos(0, 0, 0);
        if ((fileStatus == FST_MERGING) && (mpiPosition != NULL))
        {
            mpi_pos.set(*mpiPosition);
        }

        DCGroup group;
        group.open(handles.get(mpi_pos), group_path);

        hid_t dataset_handle = -1;
        if (H5Lexists(group.getHandle(), dset_name.c_str(), H5P_LINK_ACCESS_DEFAULT))
        {
            dataset_handle = H5Dopen(group.getHandle(), dset_name.c_str(), H5P_DEFAULT);
        } else
        {
            throw DCException(getExceptionString("openDatasetInternal",
                    "dataset not found", dset_name.c_str()));
        }

        return dataset_handle;
    }

    void SerialDataCollector::closeDatasetHandle(hid_t handle)
    throw (DCException)
    {
        H5Dclose(handle);
    }

} // namespace DataCollector
