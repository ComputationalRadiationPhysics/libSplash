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

#ifndef PARALLELDATACOLLECTOR_HPP
#define	PARALLELDATACOLLECTOR_HPP

#include <mpi.h>
#include <sstream>
#include <string>
#include <iostream>
#include <set>
#include <hdf5.h>

#include "IParallelDataCollector.hpp"

#include "DCException.hpp"
#include "sdc_defines.hpp"
#include "core/HandleMgr.hpp"

namespace DCollector
{

    /**
     * Realizes an IParallelDataCollector which creates a single HDF5 file for
     * all MPI processes and accesses the file using collective MPI I/O.
     *
     * see IParallelDataCollector interface for function documentation.
     */
    class ParallelDataCollector : public IParallelDataCollector
    {
    private:
        /**
         * Set properties for file access property list.
         *
         * @param fileAccProperties reference to fileAccProperties to set parameters for
         */
        void setFileAccessParams(hid_t& fileAccProperties);

        /**
         * Constructs a filename from a base filename and the current id
         * such as baseFilename+id+.h5
         * 
         * @param id current iteration
         * @param baseFilename base filename for the new file
         * @return newly constructed filename including file extension
         */
        static std::string getFullFilename(uint32_t id, std::string baseFilename);

        /**
         * Internal function for formatting exception messages.
         * 
         * @param func name of the throwing function
         * @param msg exception message
         * @param info optional info text to be appended, e.g. the group name
         * @return formatted exception message string
         */
        static std::string getExceptionString(std::string func, std::string msg,
                const char *info = NULL);

        static void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos);
        
        static void listFilesInDir(const std::string baseFilename, std::set<int32_t> &ids);
    protected:

        typedef struct
        {
            // internal MPI structures
            MPI_Comm mpiComm;
            MPI_Info mpiInfo;
            int mpiRank;
            int mpiSize;
            Dimensions mpiPos;
            Dimensions mpiTopology;
            // enable data compression
            bool enableCompression;
            // id for maximum accessed iteration
            int32_t maxID;
        } Options;

        /**
         * internal type to save file access mode
         */
        enum FileStatusType
        {
            FST_CLOSED, FST_WRITING, FST_READING, FST_CREATING
        };

        Options options;

        // internal hdf5 file handles
        HandleMgr handles;

        // property list for hdf5 file access
        hid_t fileAccProperties;

        // current file access type
        FileStatusType fileStatus;
        
        // filename passed to PDC
        std::string baseFilename;

        static void writeHeader(hid_t fHandle, uint32_t id,
                bool enableCompression, Dimensions mpiTopology) throw (DCException);

        static void fileCreateCallback(H5Handle handle, uint32_t index,
                void *userData) throw (DCException);

        static void fileOpenCallback(H5Handle handle, uint32_t index,
                void *userData) throw (DCException);

        void openCreate(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void openRead(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void openWrite(const char *filename,
                FileCreationAttr &attr) throw (DCException);

        void readDataSet(H5Handle h5File,
                int32_t id,
                const char* name,
                bool parallelRead,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                const Dimensions srcSize,
                const Dimensions srcOffset,
                Dimensions &sizeRead,
                uint32_t& srcRank,
                void* dst) throw (DCException);

        void writeDataSet(
                H5Handle group,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& datatype,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) throw (DCException);

        void gatherMPIWrites(int rank, const Dimensions localSize,
                Dimensions &globalSize, Dimensions &globalOffset) throw (DCException);

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

    public:
        /**
         * Constructor
         * @param comm the MPI_Comm object
         * @param info the MPI_Info object
         * @param topology number of MPI processes in each dimension
         * @param maxFileHandles maximum number of concurrently opened file handles
         */
        ParallelDataCollector(MPI_Comm comm, MPI_Info info, const Dimensions topology,
                uint32_t maxFileHandles);

        /**
         * Destructor
         */
        virtual ~ParallelDataCollector();

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

        /**
         * {@link DataCollector#getEntriesForID}
         */
        void getEntriesForID(int32_t id, DCEntry *entries, size_t *count) throw (DCException);

        /**
         * {@link IParallelDataCollector#write}
         */
        void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data) throw (DCException);

        /**
         * {@link IParallelDataCollector#write}
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
         * {@link IParallelDataCollector#write}
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
         * {@link IParallelDataCollector#write}
         */
        void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data);

        /**
         * {@link IParallelDataCollector#write}
         */
        void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data);

        /**
         * {@link IParallelDataCollector#write}
         */
        void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data);

        /**
         * {@link IParallelDataCollector#reserve}
         */
        void reserve(int32_t id,
                const Dimensions size,
                Dimensions *globalSize,
                Dimensions *globalOffset,
                uint32_t rank,
                const CollectionType& type,
                const char* name) throw (DCException);
        
        /**
         * {@link IParallelDataCollector#append}
         */
        void append(int32_t id,
                const Dimensions size,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions globalOffset,
                const char *name,
                const void *data);

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
         * {@link DataCollector#readGlobalAttribute}
         */
        void readGlobalAttribute(int32_t id,
                const char* name,
                void* data) throw (DCException);

        /**
         * {@link DataCollector#writeGlobalAttribute}
         */
        void writeGlobalAttribute(int32_t id,
                const CollectionType& type,
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
                Dimensions &sizeRead,
                void* data) throw (DCException);
        /**
         * {@link DataCollector#read}
         */
        void read(int32_t id,
                const CollectionType& type,
                const char* name,
                const Dimensions dstBuffer,
                Dimensions &sizeRead,
                const Dimensions dstOffset,
                void* data) throw (DCException);

        /**
         * {@link IParallelDataCollector#read}
         */
        void read(int32_t id,
                const Dimensions localSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                const char* name,
                Dimensions &sizeRead,
                void* data) throw (DCException);

        /**
         * {@link IParallelDataCollector#read}
         */
        void read(int32_t id,
                const Dimensions localSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                const char* name,
                const Dimensions dstBuffer,
                Dimensions &sizeRead,
                const Dimensions dstOffset,
                void* data) throw (DCException);
    private:
        
        void readGlobalAttribute(const char *name,
                void* data,
                Dimensions *mpiPosition = NULL)
        {
            
        }

        void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* data)
        {
            
        }
        
        void append(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const void *data)
        {
            
        }

        void append(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t stride,
                const char *name,
                const void *data)
        {
            
        }

        void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) throw (DCException);
    };

}

#endif	/* PARALLELDATACOLLECTOR_HPP */

