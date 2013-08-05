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

using namespace DCollector;

/*******************************************************************************
 * PRIVATE FUNCTIONS
 *******************************************************************************/

void ParallelDataCollector::setFileAccessParams(hid_t& fileAccProperties)
{
    fileAccProperties = H5P_FILE_ACCESS_DEFAULT;

    H5Pset_fapl_mpio(fileAccProperties, mpiComm, mpiInfo);

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
    mpiPos[2] = index % mpiSize[2];
    mpiPos[1] = (index / mpiSize[2]) % mpiSize[1];
    mpiPos[0] = index / (mpiSize[1] * mpiSize[2]);
}

/*******************************************************************************
 * PUBLIC FUNCTIONS
 *******************************************************************************/

ParallelDataCollector::ParallelDataCollector(MPI_Comm comm, MPI_Info info,
        const Dimensions topology, int mpiRank, uint32_t maxFileHandles) :
mpiComm(comm),
mpiInfo(info),
mpiRank(mpiRank),
mpiSize(topology.getDimSize()),
mpiTopology(topology),
handles(maxFileHandles, HandleMgr::FNS_ITERATIONS),
fileStatus(FST_CLOSED)
{
    if (H5open() < 0)
        throw DCException(getExceptionString("ParallelDataCollector",
            "failed to initialize/open HDF5 library"));

    // surpress automatic output of HDF5 exception messages
    if (H5Eset_auto2(H5E_DEFAULT, NULL, NULL) < 0)
        throw DCException(getExceptionString("ParallelDataCollector",
            "failed to disable error printing"));

    // set some default file access parameters
    setFileAccessParams(fileAccProperties);

    handles.registerFileCreate(fileCreateCallback);

    std::cout << "[" << mpiRank << "] ParallelDataCollector with topology " <<
            mpiTopology.toString() << std::endl;

    indexToPos(mpiRank, mpiTopology, mpiPos);
}

ParallelDataCollector::~ParallelDataCollector()
{
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
    return 0;
}

void ParallelDataCollector::getMPISize(Dimensions& mpiSize)
{
    mpiSize.set(mpiTopology);
}

void ParallelDataCollector::getEntryIDs(int32_t *ids, size_t *count)
throw (DCException)
{
    return;
}

void ParallelDataCollector::getEntriesForID(int32_t id, DCEntry *entries,
        size_t *count)
throw (DCException)
{
    return;
}

void ParallelDataCollector::readGlobalAttribute(
        const char* name,
        void* data,
        Dimensions *mpiPosition)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::writeGlobalAttribute(const CollectionType& type,
        const char* name,
        const void* data)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::readAttribute(int32_t id,
        const char *dataName,
        const char *attrName,
        void* data,
        Dimensions *mpiPosition)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::writeAttribute(int32_t id,
        const CollectionType& type,
        const char *dataName,
        const char *attrName,
        const void* data)
throw (DCException)
{
    throw DCException("Not yet implemented!");
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
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcData, const char* name, const void* data)
throw (DCException)
{
    write(id, srcData * mpiTopology, srcData * mpiPos,
            type, rank, srcData, Dimensions(1, 1, 1),
            srcData, Dimensions(0, 0, 0), name, data);
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcBuffer, const Dimensions srcData, const Dimensions srcOffset,
        const char* name, const void* data)
throw (DCException)
{
    write(id, srcData * mpiTopology, srcData * mpiPos,
            type, rank, srcBuffer, Dimensions(1, 1, 1),
            srcData, srcOffset, name, data);
}

void ParallelDataCollector::write(int32_t id, const CollectionType& type, uint32_t rank,
        const Dimensions srcBuffer, const Dimensions srcStride, const Dimensions srcData,
        const Dimensions srcOffset, const char* name, const void* data)
throw (DCException)
{
    write(id, srcData * mpiTopology, srcData * mpiPos,
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

void ParallelDataCollector::append(int32_t id, const CollectionType& type,
        uint32_t count, const char* name, const void* data)
throw (DCException)
{
    append(id, type, count, 0, 1, name, data);
}

void ParallelDataCollector::append(int32_t id, const CollectionType& type,
        uint32_t count, uint32_t offset, uint32_t stride, const char* name, const void* data)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::remove(int32_t id)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::remove(int32_t id, const char* name)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

void ParallelDataCollector::createReference(int32_t srcID,
        const char *srcName,
        const CollectionType& colType,
        int32_t dstID,
        const char *dstName,
        Dimensions count,
        Dimensions offset,
        Dimensions stride)
throw (DCException)
{
    throw DCException("Not yet implemented!");
}

/*******************************************************************************
 * PROTECTED FUNCTIONS
 *******************************************************************************/

void ParallelDataCollector::fileCreateCallback(H5Handle handle)
{
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
}

void ParallelDataCollector::openCreate(const char *filename,
        FileCreationAttr& attr)
throw (DCException)
{
    this->fileStatus = FST_CREATING;
    this->enableCompression = attr.enableCompression;

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
    fileStatus = FST_WRITING;
    this->enableCompression = attr.enableCompression;

    handles.open(Dimensions(1, 1, 1), filename, fileAccProperties, H5F_ACC_RDWR);
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
    dataset.create(datatype, group, globalSize, rank, this->enableCompression);
    if (srcData.getDimSize() > 0)
        dataset.write(srcBuffer, srcStride, srcOffset, srcData, globalOffset, data);
    dataset.close();
}

