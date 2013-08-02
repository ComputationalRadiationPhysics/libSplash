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
 


#include <cstring>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <stdexcept>
#include <cassert>
#include <sys/stat.h> 
#include <H5Ppublic.h>

#include "SerialDataCollector.hpp"

#include "DCAttribute.hpp"
#include "DCDataSet.hpp"
#include "DCHelper.hpp"
#include "SDCHelper.hpp"

namespace DCollector
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
        rawCacheSize = 64 * 1024 * 1024;
        H5Pset_cache(fileAccProperties, metaCacheElements, rawCacheElements, rawCacheSize, policy);

        //H5Pset_chunk_cache(0, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, H5D_CHUNK_CACHE_NBYTES_DEFAULT, H5D_CHUNK_CACHE_W0_DEFAULT);

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "Raw Data Cache = " << rawCacheSize / 1024 << " KiB" << std::endl;
#endif
    }

    bool SerialDataCollector::fileExists(std::string filename)
    {
        struct stat fileInfo;
        return (stat(filename.c_str(), &fileInfo) == 0);
    }

    std::string SerialDataCollector::getFullFilename(const Dimensions mpiPos, std::string baseFilename) const
    {
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
    handles(maxFileHandles),
    fileStatus(FST_CLOSED),
    maxID(-1),
    mpiSize(0, 0, 0)
    {
#ifdef COL_TYPE_CPP
        throw DCException("Check your defines !");
#endif

        if (H5open() < 0)
            throw DCException(getExceptionString("SerialDataCollector",
                "failed to initialize/open HDF5 library"));

        // surpress automatic output of HDF5 exception messages
        if (H5Eset_auto2(H5E_DEFAULT, NULL, NULL) < 0)
            throw DCException(getExceptionString("SerialDataCollector",
                "failed to disable error printing"));

        // set some default file access parameters
        setFileAccessParams(fileAccProperties);
    }

    SerialDataCollector::~SerialDataCollector()
    {
    }

    void SerialDataCollector::open(const char* filename, FileCreationAttr &attr)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "opening data collector..." << std::endl;
#endif

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
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "closing data collector..." << std::endl;
#endif

        if (fileStatus == FST_CREATING || fileStatus == FST_WRITING)
        {
            hid_t group = H5Gopen(handles.get(0), SDC_GROUP_HEADER, H5P_DEFAULT);
            if (group < 0)
            {
                std::cerr << getExceptionString("close", "unable to open group for writing", SDC_GROUP_HEADER) << std::endl;
                std::cerr << "continuing..." << std::endl;
            } else
            {
                // write number of iterations
                try
                {
                    DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, H5T_NATIVE_INT32, group, &maxID);
                } catch (DCException e)
                {
                    std::cerr << e.what() << std::endl;
                    std::cerr << "continuing..." << std::endl;
                }

                H5Gclose(group);
            }
        }

        maxID = -1;
        for (int i = 0; i < 3; i++)
        {
            mpiSize[i] = 0;
        }

        // close opened hdf5 file handles
        handles.close();

        fileStatus = FST_CLOSED;
    }

    void SerialDataCollector::readGlobalAttribute(
            const char* name,
            void* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("readCustomAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
            throw DCException(getExceptionString("readCustomAttribute", "this access is not permitted"));

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

            mpi_rank = mpi_pos[2] * mpiSize[0] * mpiSize[1] + mpi_pos[1] * mpiSize[0] + mpi_pos[0];
        }

        hid_t group_custom = H5Gopen(handles.get(mpi_rank), custom_string.c_str(), H5P_DEFAULT);
        if (group_custom < 0)
            throw DCException(getExceptionString("readCustomAttribute", "failed to open custom group", custom_string.c_str()));

        try
        {
            DCAttribute::readAttribute(name, group_custom, data);
        } catch (DCException e)
        {
            throw DCException(getExceptionString("readCustomAttribute", "failed to open attribute", name));
        }

        H5Gclose(group_custom);
    }

    void SerialDataCollector::writeGlobalAttribute(const CollectionType& type,
            const char* name,
            const void* data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("writeCustomAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("writeCustomAttribute", "this access is not permitted"));

        hid_t group_custom = H5Gopen(handles.get(0), SDC_GROUP_CUSTOM, H5P_DEFAULT);
        if (group_custom < 0)
            throw DCException(getExceptionString("writeCustomAttribute", "custom group not found", SDC_GROUP_CUSTOM));

        try
        {
            DCAttribute::writeAttribute(name, type.getDataType(), group_custom, data);
        } catch (DCException e)
        {
            std::cerr << e.what() << std::endl;
            throw DCException(getExceptionString("writeCustomAttribute", "failed to write attribute", name));
        }

        H5Gclose(group_custom);
    }

    void SerialDataCollector::readAttribute(int32_t id,
            const char *dataName,
            const char *attrName,
            void* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        // mpiPosition is allowed to be NULL here
        if (attrName == NULL || dataName == NULL || data == NULL)
            throw DCException(getExceptionString("readAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
            throw DCException(getExceptionString("readAttribute", "this access is not permitted"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;
        std::string group_id_string = group_id_name.str();

        Dimensions mpi_pos(0, 0, 0);
        if ((fileStatus == FST_MERGING) && (mpiPosition != NULL))
        {
            mpi_pos.set(*mpiPosition);
        }

        hid_t group_id = -1;
        if (!H5Lexists(handles.get(mpi_pos), group_id_string.c_str(), H5P_LINK_ACCESS_DEFAULT))
            throw DCException(getExceptionString("readAttribute", "group not found", group_id_string.c_str()));
        else
            group_id = H5Gopen(handles.get(mpi_pos), group_id_string.c_str(), H5P_DEFAULT);

        hid_t dataset_id = -1;
        if (H5Lexists(group_id, dataName, H5P_LINK_ACCESS_DEFAULT))
        {
            dataset_id = H5Dopen(group_id, dataName, H5P_DEFAULT);
            try
            {
                DCAttribute::readAttribute(attrName, dataset_id, data);
            } catch (DCException)
            {
                H5Dclose(dataset_id);
                H5Gclose(group_id);
                throw;
            }
            H5Dclose(dataset_id);
        } else
        {
            H5Gclose(group_id);
            throw DCException(getExceptionString("readAttribute", "dataset not found", dataName));
        }

        // cleanup
        H5Gclose(group_id);
    }

    void SerialDataCollector::writeAttribute(int32_t id,
            const CollectionType& type,
            const char *dataName,
            const char *attrName,
            const void* data)
    throw (DCException)
    {
        if (attrName == NULL || dataName == NULL || data == NULL)
            throw DCException(getExceptionString("writeAttribute", "a parameter was null"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("writeAttribute", "this access is not permitted"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;
        std::string group_id_string = group_id_name.str();

        hid_t group_id = -1;
        if (!H5Lexists(handles.get(0), group_id_string.c_str(), H5P_LINK_ACCESS_DEFAULT))
            throw DCException(getExceptionString("writeAttribute", "group not found", group_id_string.c_str()));
        else
            group_id = H5Gopen(handles.get(0), group_id_string.c_str(), H5P_DEFAULT);

        hid_t dataset_id = -1;
        if (H5Lexists(group_id, dataName, H5P_LINK_ACCESS_DEFAULT))
        {
            dataset_id = H5Dopen(group_id, dataName, H5P_DEFAULT);
            try
            {
                DCAttribute::writeAttribute(attrName, type.getDataType(), dataset_id, data);
            } catch (DCException)
            {
                H5Dclose(dataset_id);
                H5Gclose(group_id);
                throw;
            }
            H5Dclose(dataset_id);
        } else
        {
            H5Gclose(group_id);
            throw DCException(getExceptionString("writeAttribute", "dataset not found", dataName));
        }

        // cleanup
        H5Gclose(group_id);
    }

    void SerialDataCollector::read(int32_t id,
            const CollectionType& type,
            const char* name,
            Dimensions &sizeRead,
            void* data)
    throw (DCException)
    {
        this->read(id, type, name, Dimensions(0, 0, 0), sizeRead, Dimensions(0, 0, 0), data);
    }

    void SerialDataCollector::read(int32_t id,
            const CollectionType& type,
            const char* name,
            Dimensions dstBuffer,
            Dimensions &sizeRead,
            Dimensions dstOffset,
            void* data)
    throw (DCException)
    {
        if (fileStatus != FST_READING && fileStatus != FST_WRITING && fileStatus != FST_MERGING)
            throw DCException(getExceptionString("read", "this access is not permitted"));

        if (fileStatus == FST_MERGING)
        {
            readMerged(id, type, name, dstBuffer, dstOffset, sizeRead, data);
        } else
        {
            uint32_t rank = 0;
            readInternal(handles.get(0), id, name, dstBuffer, dstOffset,
                    Dimensions(0, 0, 0), Dimensions(0, 0, 0), sizeRead, rank, data);
        }
    }

    void SerialDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
            const Dimensions srcData, const char* name, const void* data)
    throw (DCException)
    {
        write(id, type, rank, srcData, Dimensions(1, 1, 1), srcData, Dimensions(0, 0, 0), name, data);
    }

    void SerialDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
            const Dimensions srcBuffer, const Dimensions srcData, const Dimensions srcOffset,
            const char* name, const void* data)
    throw (DCException)
    {
        write(id, type, rank, srcBuffer, Dimensions(1, 1, 1), srcData, srcOffset, name, data);
    }

    void SerialDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
            const Dimensions srcBuffer, const Dimensions srcStride, const Dimensions srcData,
            const Dimensions srcOffset, const char* name, const void* data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("write", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("write", "this access is not permitted"));

        if (rank < 1 || rank > 3)
            throw DCException(getExceptionString("write", "maximum dimension is invalid"));

        if (id > this->maxID)
            this->maxID = id;

        // create group for this id/iteration
        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        hid_t group_id = -1;
        H5Handle file_handle = handles.get(0);
        if (H5Lexists(file_handle, group_id_name.str().c_str(), H5P_LINK_ACCESS_DEFAULT))
            group_id = H5Gopen(file_handle, group_id_name.str().c_str(), H5P_DEFAULT);
        else
            group_id = H5Gcreate(file_handle, group_id_name.str().c_str(), H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);

        if (group_id < 0)
            throw DCException(getExceptionString("write", "failed to open or create group"));

        // write data to the group
        try
        {
            writeDataSet(group_id, type, rank,
                    srcBuffer, srcStride, srcData, srcOffset, name, data);
        } catch (DCException)
        {
            H5Gclose(group_id);
            throw;
        }

        // cleanup
        H5Gclose(group_id);
    }

    void SerialDataCollector::append(int32_t id, const CollectionType& type,
            uint32_t count, const char* name, const void* data)
    throw (DCException)
    {
        append(id, type, count, 0, 1, name, data);
    }

    void SerialDataCollector::append(int32_t id, const CollectionType& type,
            uint32_t count, uint32_t offset, uint32_t stride, const char* name, const void* data)
    throw (DCException)
    {
        if (name == NULL || data == NULL)
            throw DCException(getExceptionString("append", "a parameter was NULL"));

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("append", "this access is not permitted"));

        if (id > this->maxID)
            this->maxID = id;

        assert(offset >= 0);
        assert(stride >= 1);

        // create group for this id/iteration
        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        hid_t group_id = -1;
        if (H5Lexists(handles.get(0), group_id_name.str().c_str(), H5P_LINK_ACCESS_DEFAULT))
            group_id = H5Gopen(handles.get(0), group_id_name.str().c_str(), H5P_DEFAULT);
        else
            group_id = H5Gcreate(handles.get(0), group_id_name.str().c_str(), H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);

        if (group_id < 0)
            throw DCException(getExceptionString("append", "failed to open or create group"));

        // write data to the group
        try
        {
            appendDataSet(group_id, type, count, offset, stride, name, data);
        } catch (DCException)
        {
            H5Gclose(group_id);
            throw;
        }

        // cleanup
        H5Gclose(group_id);
    }

    void SerialDataCollector::remove(int32_t id)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "removing group " << id << std::endl;
#endif

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        H5Handle file_handle = handles.get(0);
        if (H5Lexists(file_handle, group_id_name.str().c_str(), H5P_LINK_ACCESS_DEFAULT))
            if (H5Gunlink(file_handle, group_id_name.str().c_str()) < 0)
                throw DCException(getExceptionString("remove", "failed to remove group"));

        // update maxID to new highest group
        maxID = 0;
        size_t num_groups = 0;
        getEntryIDs(NULL, &num_groups);

        int32_t *groups = new int32_t[num_groups];
        getEntryIDs(groups, NULL);

        for (uint32_t i = 0; i < num_groups; ++i)
            if (groups[i] > maxID)
                maxID = groups[i];

        delete[] groups;
        groups = NULL;
    }

    void SerialDataCollector::remove(int32_t id, const char* name)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "removing dataset " << name << " from group " << id << std::endl;
#endif

        if (fileStatus == FST_CLOSED || fileStatus == FST_READING || fileStatus == FST_MERGING)
            throw DCException(getExceptionString("remove", "this access is not permitted"));

        if (name == NULL)
            throw DCException(getExceptionString("remove", "a parameter was NULL"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;

        if (H5Lexists(handles.get(0), group_id_name.str().c_str(), H5P_LINK_ACCESS_DEFAULT))
        {
            hid_t group_id = H5Gopen(handles.get(0), group_id_name.str().c_str(), H5P_DEFAULT);
            if (group_id < 0)
                throw DCException(getExceptionString("remove", "failed to open group", group_id_name.str().c_str()));

            if (H5Gunlink(group_id, name) < 0)
            {
                H5Gclose(group_id);
                throw DCException(getExceptionString("remove", "failed to remove dataset", name));
            }

            H5Gclose(group_id);
        } else
            throw DCException(getExceptionString("remove", "group does not exist", group_id_name.str().c_str()));
    }

    void SerialDataCollector::createReference(int32_t srcID,
            const char *srcName,
            const CollectionType& colType,
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
        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << srcID;

        hid_t src_group_id = H5Gopen(handles.get(0), group_id_name.str().c_str(), H5P_DEFAULT);
        if (src_group_id < 0)
            throw DCException(getExceptionString("createReference", "source group not found",
                group_id_name.str().c_str()));

        // open destination group
        group_id_name.str("");
        group_id_name << SDC_GROUP_DATA << "/" << dstID;

        // if destination group does not exist, it is created
        hid_t dst_group_id = -1;
        if (H5Lexists(handles.get(0), group_id_name.str().c_str(), H5P_LINK_ACCESS_DEFAULT))
            dst_group_id = H5Gopen(handles.get(0), group_id_name.str().c_str(), H5P_DEFAULT);
        else
            dst_group_id = H5Gcreate(handles.get(0), group_id_name.str().c_str(), H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);

        if (dst_group_id < 0)
        {
            H5Gclose(src_group_id);
            throw DCException(getExceptionString("createReference",
                    "destination group could be be created/not found",
                    group_id_name.str().c_str()));
        }

        // open source dataset
        try
        {
            DCDataSet src_dataset(srcName);
            src_dataset.open(src_group_id);

            DCDataSet dst_dataset(dstName);
            // create the actual reference as a new dataset
            dst_dataset.createReference(dst_group_id, src_group_id,
                    src_dataset, count, offset, stride);

            dst_dataset.close();
            src_dataset.close();

        } catch (DCException e)
        {
            H5Gclose(src_group_id);
            H5Gclose(dst_group_id);
            throw e;
        }

        // close groups
        H5Gclose(src_group_id);
        H5Gclose(dst_group_id);
    }

    /*******************************************************************************
     * PROTECTED FUNCTIONS
     *******************************************************************************/

    void SerialDataCollector::openCreate(const char *filename,
            FileCreationAttr& attr)
    throw (DCException)
    {
        this->fileStatus = FST_CREATING;

        // appends the mpiPosition to the filename (e.g. myfile_0_1_0.h5)
        std::string full_filename = getFullFilename(attr.mpiPosition, filename);
        DCHelper::testFilename(full_filename);

        this->enableCompression = attr.enableCompression;

#if defined SDC_DEBUG_OUTPUT
        if (attr.enableCompression)
            std::cerr << "compression is ON" << std::endl;
        else
            std::cerr << "compression is OFF" << std::endl;
#endif

        // open file
        handles.open(full_filename, fileAccProperties, H5F_ACC_TRUNC);

        this->maxID = 0;
        this->mpiSize.set(attr.mpiSize);

        // write datatypes and header information to the file
        SDCHelper::writeHeader(handles.get(0), attr.mpiPosition, &(this->maxID),
                &(this->enableCompression), &(this->mpiSize), false);

        // the custom group hold user-specified attributes
        hid_t group_custom = H5Gcreate(handles.get(0), SDC_GROUP_CUSTOM, H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
        if (group_custom < 0)
            throw DCException(getExceptionString("openCreate", "failed to create custom group", SDC_GROUP_CUSTOM));

        H5Gclose(group_custom);

        // the data group holds the actual simulation data
        hid_t group_data = H5Gcreate(handles.get(0), SDC_GROUP_DATA, H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
        if (group_data < 0)
            throw DCException(getExceptionString("openCreate", "failed to create custom group", SDC_GROUP_DATA));

        H5Gclose(group_data);
    }

    void SerialDataCollector::openWrite(const char* filename, FileCreationAttr& attr)
    throw (DCException)
    {
        fileStatus = FST_WRITING;

        std::string full_filename = filename;
        if (full_filename.find(".h5") == std::string::npos)
            full_filename = getFullFilename(attr.mpiPosition, filename);

        DCHelper::testFilename(full_filename);

        this->enableCompression = attr.enableCompression;

        if (fileExists(full_filename))
        {
            // read reference data from target file
            SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiSize));

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
        std::string full_filename = getFullFilename(Dimensions(0, 0, 0), filename);

        DCHelper::testFilename(full_filename);

        if (!fileExists(full_filename))
        {
            this->fileStatus = FST_CLOSED;
            throw DCException(getExceptionString("openMerge", "File not found.", full_filename.c_str()));
        }

        // read reference data from target file
        SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiSize));

        // no compression for in-memory datasets
        this->enableCompression = false;

        handles.open(mpiSize, filename, fileAccProperties, H5F_ACC_RDONLY);
    }

    void SerialDataCollector::openRead(const char* filename, FileCreationAttr& attr)
    throw (DCException)
    {
        this->fileStatus = FST_READING;

        std::string full_filename = filename;
        if (full_filename.find(".h5") == std::string::npos)
            full_filename = getFullFilename(attr.mpiPosition, filename);

        DCHelper::testFilename(full_filename);

        if (!fileExists(full_filename))
        {
            this->fileStatus = FST_CLOSED;
            throw DCException(getExceptionString("openRead", "File not found", full_filename.c_str()));
        }

        // read reference data from target file
        SDCHelper::getReferenceData(full_filename.c_str(), &(this->maxID), &(this->mpiSize));

        handles.open(full_filename, fileAccProperties, H5F_ACC_RDONLY);
    }

    void SerialDataCollector::writeDataSet(hid_t &group, const CollectionType& datatype, uint32_t rank,
            const Dimensions srcBuffer, const Dimensions srcStride,
            const Dimensions srcData, const Dimensions srcOffset,
            const char* name, const void* data) throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# SerialDataCollector::writeDataSet #" << std::endl;
#endif
        DCDataSet dataset(name);
        // always create dataset but write data only if all dimensions > 0
        dataset.create(datatype, group, srcData, rank, this->enableCompression);
        if (srcData.getDimSize() > 0)
            dataset.write(srcBuffer, srcStride, srcOffset, srcData, data);
        dataset.close();
    }

    void SerialDataCollector::appendDataSet(hid_t& group, const CollectionType& datatype,
            uint32_t count, uint32_t offset, uint32_t stride, const char* name, const void* data)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# SerialDataCollector::appendDataSet #" << std::endl;
#endif
        DCDataSet dataset(name);

        if (!dataset.open(group))
        {
            Dimensions data_size(count, 1, 1);
            dataset.create(datatype, group, data_size, 1, this->enableCompression);

            if (count > 0)
                dataset.write(Dimensions(offset + count * stride, 1, 1),
                    Dimensions(stride, 1, 1),
                    Dimensions(offset, 0, 0),
                    data_size,
                    data);
        } else
            if (count > 0)
            dataset.append(count, offset, stride, data);

        dataset.close();
    }

    size_t SerialDataCollector::getRank(H5Handle h5File,
            int32_t id,
            const char* name)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# SerialDataCollector::getDataTypeInfo #" << std::endl;
#endif

        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("getDataTypeInfo", "invalid parameters"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;
        std::string group_id_string = group_id_name.str();

        hid_t group_id = H5Gopen(h5File, group_id_string.c_str(), H5P_DEFAULT);
        if (group_id < 0)
            throw DCException(getExceptionString("getDataTypeInfo", "group not found", group_id_string.c_str()));

        size_t rank = 0;

        try
        {
            DCDataSet dataset(name);
            dataset.open(group_id);

            rank = dataset.getRank();

            dataset.close();
        } catch (DCException e)
        {
            H5Gclose(group_id);
            throw e;
        }

        // cleanup
        H5Gclose(group_id);

        return rank;
    }

    void SerialDataCollector::readInternal(H5Handle h5File,
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
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# SerialDataCollector::readInternal #" << std::endl;
#endif

        if (h5File < 0 || name == NULL)
            throw DCException(getExceptionString("readInternal", "invalid parameters"));

        std::stringstream group_id_name;
        group_id_name << SDC_GROUP_DATA << "/" << id;
        std::string group_id_string = group_id_name.str();

        hid_t group_id = H5Gopen(h5File, group_id_string.c_str(), H5P_DEFAULT);
        if (group_id < 0)
            throw DCException(getExceptionString("readInternal", "group not found", group_id_string.c_str()));

        try
        {
            DCDataSet dataset(name);
            dataset.open(group_id);
            dataset.read(dstBuffer, dstOffset, srcSize, srcOffset, sizeRead, srcRank, dst);
            dataset.close();
        } catch (DCException e)
        {
            H5Gclose(group_id);
            throw e;
        }

        // cleanup
        H5Gclose(group_id);
    }

    void SerialDataCollector::readMerged(int32_t id,
            const CollectionType& type,
            const char* name,
            Dimensions dstBuffer,
            Dimensions dstOffset,
            Dimensions& srcData,
            void* data)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# SerialDataCollector::readMerged #" << std::endl;
#endif

        // set the master 'file' to be in-memory only
        hid_t fileAccPList = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_core(fileAccPList, 4 * 1024 * 1024, false);
        /**
         * \todo: allow changing master filename
         */
        H5Handle master_file = H5Fcreate("/tmp/master.h5", H5F_ACC_TRUNC, H5P_FILE_CREATE_DEFAULT, fileAccPList);
        if (master_file < 0)
            throw DCException(getExceptionString("readMerged", "failed to create master file", name));

        hid_t master_group = H5Gcreate(master_file, "master_group", H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
        if (master_group < 0)
            throw DCException(getExceptionString("readMerged", "failed to create master group", name));

        Dimensions dim_master(1, 1, 1);

        int rank_1_counter = 0;

        // read size of client buffer from first file to get master buffer size
        uint32_t client_rank = 0;
        Dimensions *client_size = new Dimensions[mpiSize.getDimSize()];

        readInternal(handles.get(0), id, name, Dimensions(), Dimensions(),
                Dimensions(0, 0, 0), Dimensions(0, 0, 0), client_size[0], client_rank, NULL);

        // on first access, create master data buffer depending on tmp_buffer_size
        size_t buffer_size = 1;

        // for 2D/3D, all clients must have written same sizes
        if (client_rank > 1)
        {
            for (uint32_t i = 1; i < mpiSize.getDimSize(); i++)
                client_size[i].set(client_size[0]);

            for (uint32_t i = 0; i < client_rank; i++)
            {
                srcData[i] = client_size[0][i] * mpiSize[i];
                dim_master[i] = srcData[i];
                buffer_size *= dim_master[i];
            }
        } else
        {
            // for 1D, clients may have written differing sizes,
            // so each one has to be read seperately
            srcData.set(client_size[0]);

            for (uint32_t i = 1; i < mpiSize.getDimSize(); i++)
            {
                readInternal(handles.get(i), id, name,
                        Dimensions(), Dimensions(),
                        Dimensions(), Dimensions(0, 0, 0),
                        client_size[i], client_rank, NULL);
                assert(client_rank <= 1);
                srcData[0] += client_size[i][0];
            }

            dim_master[0] = srcData[0];
            buffer_size = dim_master[0];
        }

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "client_rank = " << client_rank << std::endl;
        std::cerr << "dim_master = " << dim_master.toString() << std::endl;

        std::cerr << "client_size[0] = " << client_size[0].toString() << std::endl;
        if (client_rank == 1)
            for (size_t i = 1; i < mpiSize.getDimSize(); i++)
                std::cerr << "client_size[" << i << "] = " << client_size[i].toString() << std::endl;
#endif

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "creating master dataset..." << std::endl;
#endif

        DCDataSet dataset_master("master");
        dataset_master.create(type, master_group, dim_master, client_rank, this->enableCompression);

        // open files and read data
        for (size_t z = 0; z < mpiSize[2]; z++)
            for (size_t y = 0; y < mpiSize[1]; y++)
                for (size_t x = 0; x < mpiSize[0]; x++)
                {
                    Dimensions mpi_position(x, y, z);

                    uint32_t mpi_rank = z * mpiSize[0] * mpiSize[1] + y * mpiSize[0] + x;

#if defined SDC_DEBUG_OUTPUT
                    std::cerr << std::endl;
                    std::cerr << "mpi position = (" << x << ", " << y << ", " << z << ")" << std::endl;
                    std::cerr << "mpi_rank == h5handle == " << mpi_rank << std::endl;
#endif
                    // copy data from client buffer to relevant position in master buffer
                    char *tmp_buffer = new char[client_size[mpi_rank].getDimSize() * type.getSize()];
                    uint32_t tmp_rank(0);
                    readInternal(handles.get(mpi_position), id, name, client_size[mpi_rank],
                            Dimensions(0, 0, 0), Dimensions(0, 0, 0), Dimensions(0, 0, 0),
                            client_size[mpi_rank], tmp_rank, tmp_buffer);

                    if (tmp_rank > 0)
                    {
                        if (tmp_rank != client_rank)
                            throw DCException(getExceptionString("readMerged",
                                "Reading dataset with invalid rank while merging", name));

                        Dimensions master_offset(0, 0, 0);

                        if (client_rank > 1)
                        {
                            for (uint32_t i = 0; i < client_rank; i++)
                                master_offset[i] = client_size[mpi_rank][i] * mpi_position[i];
                        } else
                        {
                            master_offset[0] = rank_1_counter;
                            rank_1_counter += client_size[mpi_rank][0];
                        }

#if defined SDC_DEBUG_OUTPUT
                        std::cerr << "master_offset = " << master_offset.toString() << std::endl;
                        std::cerr << "client_size[" << mpi_rank << "] = " << client_size[mpi_rank].toString() << std::endl;
#endif

                        dataset_master.write(client_size[mpi_rank], Dimensions(0, 0, 0),
                                client_size[mpi_rank], master_offset, tmp_buffer);
                    }
                    delete[] tmp_buffer;

                }



        dataset_master.read(dstBuffer, dstOffset, srcData, client_rank, data);
        //srcData.swapDims(client_rank);
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "srcBuffer = " << srcData.toString() << std::endl;
#endif

        delete[] client_size;

        dataset_master.close();

        H5Gclose(master_group);
        H5Fclose(master_file);
        H5Pclose(fileAccPList);
    }

    int32_t SerialDataCollector::getMaxID()
    {
        return maxID;
    }

    void SerialDataCollector::getMPISize(Dimensions& mpiSize)
    {
        mpiSize.set(this->mpiSize);
    }

    void SerialDataCollector::getEntryIDs(int32_t* ids, size_t* count)
    throw (DCException)
    {
        hid_t group_data = H5Gopen(handles.get(0), SDC_GROUP_DATA, H5P_DEFAULT);
        if (group_data < 0)
            throw DCException(getExceptionString("getEntryIDs", "Failed to open data group", SDC_GROUP_DATA));

        hsize_t data_entries = 0;
        if (H5Gget_num_objs(group_data, &data_entries) < 0)
            throw DCException(getExceptionString("getEntryIDs", "Failed to get entries in data group", SDC_GROUP_DATA));

        if (count != NULL)
            *count = data_entries;

        if (ids != NULL)
        {
            for (uint32_t i = 0; i < data_entries; i++)
            {
                char *group_id_name = NULL;
                ssize_t group_id_name_len = H5Gget_objname_by_idx(group_data, i, NULL, 0);
                if (group_id_name_len < 0)
                    throw DCException(getExceptionString("getEntryIDs", "Failed to get object name in group", group_id_name));

                group_id_name = new char[group_id_name_len + 1];
                H5Gget_objname_by_idx(group_data, i, group_id_name, group_id_name_len + 1);
                ids[i] = atoi(group_id_name);
                delete[] group_id_name;
            }
        }

        H5Gclose(group_data);
    }

    void SerialDataCollector::getEntriesForID(int32_t id, DCEntry *entries, size_t *count)
    throw (DCException)
    {
        char id_group_str[256];
        sprintf(id_group_str, "%s/%d", SDC_GROUP_DATA, id);

        hid_t group_id = H5Gopen(handles.get(0), id_group_str, H5P_DEFAULT);
        if (group_id < 0)
            throw DCException(getExceptionString("getEntriesForID", "Failed to open id group", id_group_str));

        hsize_t data_entries = 0;
        if (H5Gget_num_objs(group_id, &data_entries) < 0)
            throw DCException(getExceptionString("getEntriesForID", "Failed to get entries in id group", id_group_str));

        if (count != NULL)
            *count = data_entries;

        if (entries != NULL)
        {
            for (uint32_t i = 0; i < data_entries; i++)
            {
                char *group_entry_name = NULL;
                ssize_t group_entry_name_len = H5Gget_objname_by_idx(group_id, i, NULL, 0);
                if (group_entry_name_len < 0)
                    throw DCException(getExceptionString("getEntriesForID", "Failed to get object name in group", group_entry_name));

                group_entry_name = new char[group_entry_name_len + 1];
                H5Gget_objname_by_idx(group_id, i, group_entry_name, group_entry_name_len + 1);
                entries[i].name = group_entry_name;
                delete[] group_entry_name;
            }
        }

        H5Gclose(group_id);
    }

} // namespace DataCollector
