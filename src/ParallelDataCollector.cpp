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

#include <assert.h>

#include "ParallelDataCollector.hpp"
#include "DCParallelDataSet.hpp"
#include "DCAttribute.hpp"

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
    if (filename == NULL)
        throw DCException(getExceptionString("open", "filename must not be null"));

    if (fileStatus != FST_CLOSED)
        throw DCException(getExceptionString("open", "this access is not permitted"));

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
    // close opened hdf5 file handles
    handles.close();

    fileStatus = FST_CLOSED;
}

int32_t ParallelDataCollector::getMaxID()
{
    return options.maxID;
}

void ParallelDataCollector::getMPISize(Dimensions& mpiSize)
{
    mpiSize.set(options.mpiTopology);
}

void ParallelDataCollector::getEntryIDs(int32_t *ids, size_t *count)
throw (DCException)
{
    size_t num_ids = options.maxID > -1 ? 1 : 0;

    if (count != NULL)
        *count = num_ids;

    if (ids != NULL && (num_ids > 0))
        ids[0] = options.maxID;
}

void ParallelDataCollector::getEntriesForID(int32_t id, DCEntry *entries,
        size_t *count)
throw (DCException)
{
    throw DCException("Not yet implemented!");
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

    hid_t group_custom = H5Gopen(handles.get(id), SDC_GROUP_CUSTOM, H5P_DEFAULT);
    if (group_custom < 0)
    {
        throw DCException(getExceptionString("readGlobalAttribute",
                "failed to open custom group", SDC_GROUP_CUSTOM));
    }

    try
    {
        DCAttribute::readAttribute(name, group_custom, data);
    } catch (DCException e)
    {
        throw DCException(getExceptionString("readGlobalAttribute", "failed to open attribute", name));
    }

    H5Gclose(group_custom);
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

    hid_t group_custom = H5Gopen(handles.get(id), SDC_GROUP_CUSTOM, H5P_DEFAULT);
    if (group_custom < 0)
        throw DCException(getExceptionString("writeGlobalAttribute",
            "custom group not found", SDC_GROUP_CUSTOM));

    try
    {
        DCAttribute::writeAttribute(name, type.getDataType(), group_custom, data);
    } catch (DCException e)
    {
        std::cerr << e.what() << std::endl;
        throw DCException(getExceptionString("writeGlobalAttribute", "failed to write attribute", name));
    }

    H5Gclose(group_custom);
}

void ParallelDataCollector::readAttribute(int32_t id,
        const char *dataName,
        const char *attrName,
        void* data,
        Dimensions *mpiPosition)
throw (DCException)
{
    // mpiPosition is ignored
    if (attrName == NULL || dataName == NULL || data == NULL)
        throw DCException(getExceptionString("readAttribute", "a parameter was null"));

    if (fileStatus == FST_CLOSED || fileStatus == FST_CREATING)
        throw DCException(getExceptionString("readAttribute", "this access is not permitted"));

    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;
    std::string group_id_string = group_id_name.str();

    hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
    if (group_id < 0)
        throw DCException(getExceptionString("readAttribute", "group not found",
            group_id_string.c_str()));

    hid_t dataset_id = -1;
    dataset_id = H5Dopen(group_id, dataName, H5P_DEFAULT);
    if (dataset_id < 0)
    {
        H5Gclose(group_id);
        throw DCException(getExceptionString("readAttribute", "dataset not found", dataName));
    }

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

    // cleanup
    H5Gclose(group_id);
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

    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;
    std::string group_id_string = group_id_name.str();

    hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
    if (group_id < 0)
    {
        throw DCException(getExceptionString("writeAttribute", "group not found",
                group_id_string.c_str()));
    }

    hid_t dataset_id = H5Dopen(group_id, dataName, H5P_DEFAULT);
    if (dataset_id < 0)
    {
        H5Gclose(group_id);
        throw DCException(getExceptionString("writeAttribute", "dataset not found", dataName));
    }

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

    // cleanup
    H5Gclose(group_id);
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
        const CollectionType& type,
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
        const CollectionType& type,
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
    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;

    hid_t group_id = -1;
    H5Handle file_handle = handles.get(id);

    group_id = H5Gopen(file_handle, group_id_name.str().c_str(), H5P_DEFAULT);
    if (group_id < 0)
        group_id = H5Gcreate(file_handle, group_id_name.str().c_str(), H5P_LINK_CREATE_DEFAULT,
            H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);

    if (group_id < 0)
        throw DCException(getExceptionString("write", "failed to open or create group"));

    // write data to the group
    try
    {
        writeDataSet(group_id, globalSize, globalOffset, type, rank,
                srcBuffer, srcStride, srcData, srcOffset, name, data);
    } catch (DCException)
    {
        H5Gclose(group_id);
        throw;
    }

    // cleanup
    H5Gclose(group_id);
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
    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;

    hid_t group_id = -1;
    H5Handle file_handle = handles.get(id);

    group_id = H5Gopen(file_handle, group_id_name.str().c_str(), H5P_DEFAULT);
    if (group_id < 0)
        group_id = H5Gcreate(file_handle, group_id_name.str().c_str(), H5P_LINK_CREATE_DEFAULT,
            H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);

    if (group_id < 0)
        throw DCException(getExceptionString("write", "failed to open or create group"));

    DCParallelDataSet dataset(name);
    // create the empty dataset
    dataset.create(type, group_id, global_size, rank, this->options.enableCompression);
    dataset.close();

    // cleanup
    H5Gclose(group_id);
    
    if (globalSize)
        globalSize->set(global_size);
    
    if (globalOffset)
        globalOffset->set(global_offset);
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

    if (H5Gunlink(handles.get(id), group_id_name.str().c_str()) < 0)
        throw DCException(getExceptionString("remove", "failed to remove group"));

    // update maxID
    options.maxID = -1;
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

    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;

    hid_t group_id = H5Gopen(handles.get(id), group_id_name.str().c_str(), H5P_DEFAULT);
    if (group_id < 0)
    {
        throw DCException(getExceptionString("remove", "failed to open group",
                group_id_name.str().c_str()));
    }

    if (H5Gunlink(group_id, name) < 0)
    {
        H5Gclose(group_id);
        throw DCException(getExceptionString("remove", "failed to remove dataset", name));
    }

    H5Gclose(group_id);
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
    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << srcID;

    hid_t src_group_id = H5Gopen(handles.get(srcID), group_id_name.str().c_str(), H5P_DEFAULT);
    if (src_group_id < 0)
    {
        throw DCException(getExceptionString("createReference",
                "source/destination group not found",
                group_id_name.str().c_str()));
    }

    // open source dataset
    try
    {
        DCParallelDataSet src_dataset(srcName);
        src_dataset.open(src_group_id);

        DCParallelDataSet dst_dataset(dstName);
        // create the actual reference as a new dataset
        // identical src and dst groups
        dst_dataset.createReference(src_group_id, src_group_id, src_dataset);

        dst_dataset.close();
        src_dataset.close();

    } catch (DCException e)
    {
        H5Gclose(src_group_id);
        throw e;
    }

    // close group
    H5Gclose(src_group_id);
}

void ParallelDataCollector::createReference(int32_t srcID,
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
    hid_t group_custom = H5Gcreate(handle, SDC_GROUP_CUSTOM, H5P_LINK_CREATE_DEFAULT,
            H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
    if (group_custom < 0)
        throw DCException(getExceptionString("openCreate",
            "failed to create custom group", SDC_GROUP_CUSTOM));

    H5Gclose(group_custom);

    // the data group holds the actual simulation data
    hid_t group_data = H5Gcreate(handle, SDC_GROUP_DATA, H5P_LINK_CREATE_DEFAULT,
            H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
    if (group_data < 0)
        throw DCException(getExceptionString("openCreate",
            "failed to create custom group", SDC_GROUP_DATA));

    H5Gclose(group_data);

    writeHeader(handle, index, options->enableCompression, options->mpiTopology);
}

void ParallelDataCollector::fileOpenCallback(H5Handle handle, uint32_t index, void *userData)
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
    hid_t group_header = H5Gcreate(fHandle, SDC_GROUP_HEADER, H5P_LINK_CREATE_DEFAULT,
            H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
    if (group_header < 0)
        throw DCException(getExceptionString("writeHeader",
            "Failed to create header group", NULL));

    try
    {
        ColTypeDim dim_t;

        int32_t index = id;
        bool compression = enableCompression;

        // create master specific header attributes
        DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, H5T_NATIVE_INT32,
                group_header, &index);

        DCAttribute::writeAttribute(SDC_ATTR_COMPRESSION, H5T_NATIVE_HBOOL,
                group_header, &compression);

        DCAttribute::writeAttribute(SDC_ATTR_MPI_SIZE, dim_t.getDataType(),
                group_header, mpiTopology.getPointer());

    } catch (DCException attr_error)
    {
        throw DCException(getExceptionString("writeHeader",
                "Failed to write header attribute.",
                attr_error.what()));
    }

    H5Gclose(group_header);
}

void ParallelDataCollector::openCreate(const char *filename,
        FileCreationAttr& attr)
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

    // open file
    handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_TRUNC);
}

void ParallelDataCollector::openRead(const char* filename, FileCreationAttr& attr)
throw (DCException)
{
    this->fileStatus = FST_READING;

    handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_RDONLY);
}

void ParallelDataCollector::openWrite(const char* filename, FileCreationAttr& attr)
throw (DCException)
{
    this->fileStatus = FST_WRITING;

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

    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;
    std::string group_id_string = group_id_name.str();

    hid_t group_id = H5Gopen(h5File, group_id_string.c_str(), H5P_DEFAULT);
    if (group_id < 0)
        throw DCException(getExceptionString("readInternal", "group not found", group_id_string.c_str()));

    Dimensions src_size(srcSize);
    Dimensions src_offset(srcOffset);

    try
    {
        DCParallelDataSet dataset(name);
        dataset.open(group_id);
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
        H5Gclose(group_id);
        throw e;
    }

    // cleanup
    H5Gclose(group_id);
}

void ParallelDataCollector::writeDataSet(hid_t &group, const Dimensions globalSize,
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

    std::stringstream group_id_name;
    group_id_name << SDC_GROUP_DATA << "/" << id;
    std::string group_id_string = group_id_name.str();

    hid_t group_id = H5Gopen(h5File, group_id_string.c_str(), H5P_DEFAULT);
    if (group_id < 0)
    {
        throw DCException(getExceptionString("getRank", "group not found",
                group_id_string.c_str()));
    }

    size_t rank = 0;

    try
    {
        DCParallelDataSet dataset(name);
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
