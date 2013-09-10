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



#ifndef SERIALDATACOLLECTOR_H
#define SERIALDATACOLLECTOR_H

#include <hdf5.h>
#include <sstream>
#include <iostream>

#include "DataCollector.hpp"

#include "DCException.hpp"
#include "core/HandleMgr.hpp"
#include "sdc_defines.hpp"

namespace DCollector
{

    /**
     * Realizes a DataCollector which creates a hdf5 file for each mpi process and can merge data to single file.
     *
     * see DataCollector interface for function documentation.
     */
    class SerialDataCollector : public DataCollector
    {
    private:
        /**
         * Set properties for file access property list.
         *
         * @param fileAccProperties reference to fileAccProperties to set parameters for
         */
        void setFileAccessParams(hid_t& fileAccProperties);

        /**
         * Test if a file exists.
         * 
         * @param filename file to test
         * @return if the file exists
         */
        bool fileExists(std::string filename);

        /**
         * Constructs a filename from a base filename and the process' mpi position
         * such as baseFilename+mpiPos+.h5
         * 
         * @param mpiPos MPI position of the process
         * @param baseFilename base filename for the new file
         * @return newly constructed filename iucluding file exitension
         */
        std::string getFullFilename(const Dimensions mpiPos, std::string baseFilename) const;

        /**
         * Internal function for formatting exception messages.
         * 
         * @param func name of the throwing function
         * @param msg exception message
         * @param info optional info text to be appended, e.g. the group name
         * @return formatted exception message string
         */
        std::string getExceptionString(std::string func, std::string msg, const char *info = NULL);

        static herr_t visitObjCallback(hid_t o_id, const char *name,
                const H5O_info_t *object_info, void *op_data);

    protected:

        /**
         * internal type to save file access mode
         */
        enum FileStatusType
        {
            FST_CLOSED, FST_WRITING, FST_READING, FST_CREATING, FST_MERGING
        };

        // internal hdf5 file handles
        HandleMgr handles;

        // property list for hdf5 file access
        hid_t fileAccProperties;

        // current file access type
        FileStatusType fileStatus;

        // id for last written iteration
        int32_t maxID;

        // the current size of the mpi grid
        Dimensions mpiSize;

        // enable data compression
        bool enableCompression;

        void openCreate(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void openRead(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void openWrite(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void openMerge(const char *filename) throw (DCException);

        /**
         * Reads from single hdf5 files and merges data to single master buffer.
         *
         * @param id id of the group to read from
         * @param type type information for data
         * @param name name of the dataset
         * @param dstBuffer dimensions of the buffer to read into
         * @param dstOffset offset in destination buffer to read to
         * @param srcBuffer returns the dimensions of the read data buffer
         * @param data buffer to hold read data
         */
        void readMerged(int32_t id,
                const CollectionType& type,
                const char* name,
                Dimensions dstBuffer,
                Dimensions dstOffset,
                Dimensions& srcBuffer,
                void* data) throw (DCException);

        /**
         * Returns the rank (number of dimensions) for a dataset
         * @param h5File file handle
         * @param id id of the group to read from
         * @param name name of the dataset
         * @return rank
         */
        size_t getRank(H5Handle h5File,
                int32_t id,
                const char* name);

        /**
         * Internal reading method.
         *
         * @param h5File HDF5 file to read from
         * @param id id of the group to read from
         * @param name name of the dataset
         * @param dstBuffer dimensions of the buffer to read into
         * @param dstOffset offset in destination buffer to read to
         * @param srcSize the size of the requested data buffer, starting at srcOffset
         * @param srcOffset offset in the source buffer to start reading from
         * @param sizeRead returns the size of the read dataset
         * @param srcRank returns the number of dimensions of srcBuffer
         * @param dst buffer to hold read data
         */
        void readInternal(H5Handle h5File,
                int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                const Dimensions srcSize,
                const Dimensions srcOffset,
                Dimensions& sizeRead,
                uint32_t& srcRank,
                void* dst) throw (DCException);

        /**
         * Basic method for writing to a single DataSet.
         *
         * @param group group holding the DataSet
         * @param datatype H5::DataType information for data
         * @param rank number of dimensions of the dataset
         * @param srcBuffer dimensions of the buffer
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset offset of srcData in srcBuffer
         * @param name name of the dataset
         * @param data buffer with data
         */
        void writeDataSet(
                hid_t group,
                const CollectionType& datatype,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) throw (DCException);

        /**
         * Basic method for appending data to a 1-dimensional DataSet.
         * 
         * The targeted DataSet is created or extended if necessary.
         *
         * @param group group holding the DataSet
         * @param datatype H5::DataType information for data
         * @param count number of elements in the buffer
         * @param offset offset to be used for reading from data buffer
         * @param stride stride size to be used for reading from data buffer
         * @param name name of the dataset
         * @param data buffer with data
         */
        void appendDataSet(
                hid_t group,
                const CollectionType& datatype,
                uint32_t count,
                uint32_t offset,
                uint32_t stride,
                const char *name,
                const void *data) throw (DCException);
    public:

        /**
         * Constructor
         * @param maxFileHandles maximum number of concurrently opened file handles (0=unlimited)
         */
        SerialDataCollector(uint32_t maxFileHandles);

        virtual ~SerialDataCollector();

        /**
         * {@link DataCollector#open}
         */
        void open(const char *filename,
                FileCreationAttr& attr) throw (DCException);

        /**
         * {@link DataCollector#close}
         */
        void close();

        /**
         * {@link DataCollector#getMaxID}
         */
        int32_t getMaxID();

        /**
         * {@link DataCollector#getMPISize}
         */
        void getMPISize(Dimensions& mpiSize);

        /**
         * {@link DataCollector#getEntryIDs}
         */
        void getEntryIDs(int32_t *ids, size_t *count) throw (DCException);

        void getEntriesForID(int32_t id, DCEntry *entries, size_t *count) throw (DCException);

        /**
         * {@link DataCollector#write}
         */
        void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data) throw (DCException);

        /**
         * {@link DataCollector#write}
         */
        void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) throw (DCException);

        /**
         * {@link DataCollector#write}
         */
        void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) throw (DCException);

        /**
         * {@link DataCollector#append}
         */
        void append(int32_t id,
                const CollectionType& type,
                uint32_t count,
                const char *name,
                const void *data) throw (DCException);

        /**
         * {@link DataCollector#append}
         */
        void append(int32_t id,
                const CollectionType& type,
                uint32_t count,
                uint32_t offset,
                uint32_t striding,
                const char *name,
                const void *data) throw (DCException);

        /**
         * {@link DataCollector#remove}
         */
        void remove(int32_t id) throw (DCException);

        /**
         * {@link DataCollector#remove}
         */
        void remove(int32_t id,
                const char *name) throw (DCException);

        /**
         * {@link DataCollector#createReference}
         */
        void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName) throw (DCException);

        /**
         * {@link DataCollector#createReference}
         */
        void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) throw (DCException);

        /**
         * {@link DataCollector#readGlobalAttribute}
         */
        void readGlobalAttribute(
                const char *name,
                void* data,
                Dimensions *mpiPosition = NULL) throw (DCException);

        /**
         * {@link DataCollector#writeGlobalAttribute}
         */
        void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* data) throw (DCException);

        /**
         * {@link DataCollector#readAttribute}
         */
        void readAttribute(int32_t id,
                const char *dataName,
                const char *attrName,
                void *data,
                Dimensions *mpiPosition = NULL) throw (DCException);

        /**
         * {@link DataCollector#writeAttribute}
         */
        void writeAttribute(int32_t id,
                const CollectionType& type,
                const char *dataName,
                const char *attrName,
                const void *data) throw (DCException);

        /**
         * {@link DataCollector#read}
         */
        void read(int32_t id,
                const CollectionType& type,
                const char* name,
                Dimensions &srcData,
                void* data) throw (DCException);
        /**
         * {@link DataCollector#read}
         */
        void read(int32_t id,
                const CollectionType& type,
                const char* name,
                const Dimensions dstBuffer,
                Dimensions &srcData,
                const Dimensions dstOffset,
                void* data) throw (DCException);
    };

} // namespace DataCollector

#endif	/* SERIALDATACOLLECTOR_H */
