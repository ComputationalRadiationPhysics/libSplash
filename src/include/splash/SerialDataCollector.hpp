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

#ifndef SERIALDATACOLLECTOR_H
#define SERIALDATACOLLECTOR_H

#include <hdf5.h>
#include <sstream>
#include <iostream>

#include "splash/DataCollector.hpp"
#include "splash/DCException.hpp"
#include "splash/core/HandleMgr.hpp"
#include "splash/sdc_defines.hpp"

namespace splash
{

    class DCGroup;

    /**
     * Realizes a DataCollector which creates an HDF5 file for each MPI process.
     *
     * See \ref DataCollector interface for function documentation.
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
         * such as myfile_0_1_0.h5
         * Does nothing if the filename already ends with ".h5" (only allowed
         * for Prod(mpiSize)==1 or reading).
         *
         * @param mpiPos MPI position of the process
         * @param baseFilename base filename for the new file
         * @param isFullNameAllowed If false, an exception is raised when a full name is passed
         * @return newly constructed filename including file extension
         */
        std::string getFullFilename(const Dimensions mpiPos, std::string baseFilename,
                bool isFullNameAllowed) const throw (DCException);

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

        void openCustomGroup(DCGroup& group, Dimensions *mpiPosition = NULL) throw (DCException);
        /** @return H5 object id if name!=NULL, else -1 */
        hid_t openGroup(DCGroup& group, int32_t id, const char* name,
                Dimensions *mpiPosition = NULL) throw (DCException);

    protected:

        /**
         * internal type to save file access mode
         */
        enum FileStatusType
        {
            FST_CLOSED, FST_WRITING, FST_READING, FST_CREATING, FST_MERGING
        };

        // internal HDF5 file handles
        HandleMgr handles;

        // property list for HDF5 file access
        hid_t fileAccProperties;

        // current file access type
        FileStatusType fileStatus;

        // id for last written iteration
        int32_t maxID;

        // the MPI topology for a distributed file
        Dimensions mpiTopology;

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
         * Reads from single HDF5 files and merges data to single master buffer.
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
         * Returns the number of dimensions for a dataset.
         */
        size_t getNDims(H5Handle h5File,
                int32_t id,
                const char* name);

        /**
         * Internal reading method.
         */
        void readCompleteDataSet(H5Handle h5File,
                int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                const Dimensions srcOffset,
                Dimensions &sizeRead,
                uint32_t& srcDims,
                void* dst)
        throw (DCException);

        /**
         * Internal reading method.
         */
        void readDataSet(H5Handle h5File,
                int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                const Dimensions srcSize,
                const Dimensions srcOffset,
                Dimensions& sizeRead,
                uint32_t& srcDims,
                void* dst) throw (DCException);

        /**
         * Internal meta data reading method.
         */
        CollectionType* readDataSetMeta(H5Handle h5File,
                int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                const Dimensions srcOffset,
                Dimensions &sizeRead,
                uint32_t& srcDims) throw (DCException);

        void readSizeInternal(H5Handle h5File,
                int32_t id,
                const char* name,
                Dimensions &sizeRead) throw (DCException);

        /**
         * Basic method for writing to a single DataSet.
         */
        void writeDataSet(
                hid_t group,
                const CollectionType& datatype,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const void* data) throw (DCException);

        /**
         * Basic method for appending data to a 1-dimensional DataSet.
         *
         * The targeted DataSet is created or extended if necessary.
         */
        void appendDataSet(
                hid_t group,
                const CollectionType& datatype,
                size_t count,
                size_t offset,
                size_t stride,
                const char *name,
                const void *data) throw (DCException);

        hid_t openDatasetHandle(int32_t id,
                const char *dsetName,
                Dimensions *mpiPosition = NULL) throw (DCException);

        void closeDatasetHandle(hid_t handle) throw (DCException);
    public:

        /**
         * Constructor
         * @param maxFileHandles Maximum number of concurrently opened file handles (0=unlimited).
         */
        SerialDataCollector(uint32_t maxFileHandles);

        /**
         * Destructor
         */
        virtual ~SerialDataCollector();

        void open(const char *filename,
                FileCreationAttr& attr) throw (DCException);

        void close();

        int32_t getMaxID();

        void getMPISize(Dimensions& mpiSize);

        void getEntryIDs(int32_t *ids, size_t *count) throw (DCException);

        void getEntriesForID(int32_t id, DCEntry *entries, size_t *count) throw (DCException);

        void write(int32_t id,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const void* data) throw (DCException);

        void append(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const void *data) throw (DCException);

        void append(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t striding,
                const char *name,
                const void *data) throw (DCException);

        void remove(int32_t id) throw (DCException);

        void remove(int32_t id,
                const char *name) throw (DCException);

        void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName) throw (DCException);

        void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) throw (DCException);

        AttributeInfo readGlobalAttributeInfo(
                int32_t id,
                const char *name,
                Dimensions *mpiPosition = NULL) throw (DCException);

        SPLASH_DEPRECATED("Use safer readGlobalAttributeInfo")
        void readGlobalAttribute(
                const char *name,
                void* data,
                Dimensions *mpiPosition = NULL) throw (DCException);

        void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* data) throw (DCException);

        void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                uint32_t ndims,
                const Dimensions dims,
                const void* data) throw (DCException);

        AttributeInfo readAttributeInfo(int32_t id,
                const char *dataName,
                const char *attrName,
                Dimensions *mpiPosition = NULL) throw (DCException);

        SPLASH_DEPRECATED("Use safer readAttributeInfo")
        void readAttribute(int32_t id,
                const char *dataName,
                const char *attrName,
                void *data,
                Dimensions *mpiPosition = NULL) throw (DCException);

        void writeAttribute(int32_t id,
                const CollectionType& type,
                const char *dataName,
                const char *attrName,
                const void *data) throw (DCException);

        void writeAttribute(int32_t id,
                const CollectionType& type,
                const char *dataName,
                const char *attrName,
                uint32_t ndims,
                const Dimensions dims,
                const void *data) throw (DCException);

        void read(int32_t id,
                const char* name,
                Dimensions &sizeRead,
                void* data) throw (DCException);

        void read(int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                Dimensions &sizeRead,
                void* data) throw (DCException);

        CollectionType* readMeta(int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                Dimensions &sizeRead) throw (DCException);
    };

} // namespace DataCollector

#endif  /* SERIALDATACOLLECTOR_H */
