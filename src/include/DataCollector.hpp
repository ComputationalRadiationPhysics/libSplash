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



/**
 * @mainpage libSplash
 *
 * libSplash is a combined project of the Center for Information Services and HPC (ZIH)
 * of the Technical University of Dresden and the Helmholtz-Zentrum Dresden-Rossendorf (HZDR).
 * 
 * The project aims at developing a HDF5-based I/O library for HPC simulations.
 * It is created as an easy-to-use frontend for the standard HDF5 library with 
 * support for MPI processes in a cluster environment.
 * 
 * While the standard HDF5 library provides detailed low-level control,
 * libSplash simplifies tasks commonly found in large-scale
 * HPC simulations, such as iterative computations and MPI distributed processes.
 *
 * \image html zih_logo.jpg
 * \image html hzdr_logo.jpg
 *
 * @authors Felix Schmitt
 * @authors René Widera
 */

#ifndef _DATACOLLECTOR_H
#define	_DATACOLLECTOR_H

#include <stdint.h>
#include <stdexcept>

#include "CollectionType.hpp"
#include "Dimensions.hpp"

namespace DCollector
{

    class DataCollector
    {
    public:

        /**
         * flags for opening files
         */
        enum FileAccType
        {
            FAT_CREATE, FAT_READ, FAT_READ_MERGED, FAT_WRITE
        };

        typedef struct _FileCreationAttr
        {

            _FileCreationAttr() :
            fileAccType(FAT_CREATE),
            mpiSize(1, 1, 1),
            mpiPosition(0, 0, 0),
            enableCompression(false),
            maxFileHandles(100)
            {

            }

            FileAccType fileAccType;
            Dimensions mpiSize;
            Dimensions mpiPosition;
            bool enableCompression;
            uint32_t maxFileHandles;
        } FileCreationAttr;

        typedef struct _DCEntry
        {
            std::string name;
        } DCEntry;

        /**
         * Initialises FileCreationAttr with default values.
         * (compression = false, access type = create, position = (0, 0, 0), size = (1, 1, 1))
         * 
         * @param attr file attributes to initialize
         */
        static void initFileCreationAttr(FileCreationAttr& attr)
        {
            attr.enableCompression = false;
            attr.fileAccType = FAT_CREATE;
            attr.mpiPosition.set(0, 0, 0);
            attr.mpiSize.set(1, 1, 1);
        }

        virtual ~DataCollector()
        {
        };

        /**
         * Opens one or multiple files for reading/writing data.
         *
         * @param filename name of the file(s) to open (the common part without position identifiers)
         * @param attr struct passing several parameters on accessing files
         */
        virtual void open(
                const char *filename,
                FileCreationAttr& attr) = 0;

        /**
         * Closes open files and releases buffers.
         *
         * Must be called by user when finished writing/reading data.
         * Should not be called after merge().
         */
        virtual void close() = 0;

        /**
         * @return get highest iteration id
         */
        virtual int32_t getMaxID() = 0;

        /**
         * @param mpiSize returns size of mpi grid
         */
        virtual void getMPISize(Dimensions& mpiSize) = 0;

        /**
         * Returns all IDs in an opened file.
         *
         * @param ids pointer to an array with at least count entries. can be NULL
         * @param count returns the number of entries in the ids array. can be NULL
         */
        virtual void getEntryIDs(int32_t *ids, size_t *count) = 0;

        /**
         * Returns all entries for an ID.
         * 
         * @param id id for fileentry, e.g. iteration
         * @param entries pointer to an array with at least count elements. can be NULL
         * @param count returns the (potential) number of elements in entries. can be NULL
         */
        virtual void getEntriesForID(int32_t id, DCEntry *entries, size_t *count) = 0;

        /**
         * Writes data to hdf5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcData intended 3D dimension for dataset
         * @param name name for the dataset, e.g. 'ions'
         * @param data data buffer to write to file
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data) = 0;

        /**
         * Writes data to hdf5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcBuffer dimensions of memory buffer
         * @param srcData intended 3D dimension for dataset
         * @param srcOffset offset of dataset in memory buffer
         * @param name name for the dataset, e.g. 'ions'
         * @param data data buffer to write to file
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) = 0;

        /**
         * Writes data to hdf5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcBuffer dimensions of memory buffer
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcData intended 3D dimension for dataset
         * @param srcOffset offset of dataset in memory buffer
         * @param name name for the dataset, e.g. 'ions'
         * @param data data buffer to write to file
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) = 0;

        /**
         * Appends 1-dimensional data in a hdf5 file.
         *
         * The target dataset is created if necessary.
         * If it already exists, data is appended to the end.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data.
         * @param count number of elements to append
         * @param name name for the dataset to create/append to, e.g. 'ions'
         * @param data data buffer to append
         */
        virtual void append(int32_t id,
                const CollectionType& type,
                uint32_t count,
                const char *name,
                const void *data) = 0;

        /**
         * Appends 1-dimensional data in a hdf5 file using striding.
         *
         * The target dataset is created if necessary.
         * If it already exists, data is appended to the end.
         * The data is read using striding from the src buffer.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data.
         * @param count number of elements to append
         * @param offset offset in elements to start reading from
         * @param stride striding to be used for reading. 
         * data must contain at least (striding * count) elements. 
         * 1 mean 'no striding'
         * @param name name for the dataset to create/append to, e.g. 'ions'
         * @param data data buffer to append
         */
        virtual void append(int32_t id,
                const CollectionType& type,
                uint32_t count,
                uint32_t offset,
                uint32_t stride,
                const char *name,
                const void *data) = 0;

        /**
         * Removes a simulation step from the hdf5 file.
         * 
         * @param id id of the step to be removed
         */
        virtual void remove(int32_t id) = 0;

        /**
         * Removes a dataset from a hdf5 file.
         * 
         * @param id id of the simulation step holding the dataset
         * @param name name of the dataset to be removed
         */
        virtual void remove(int32_t id,
                const char *name) = 0;

        virtual void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName) = 0;

        /**
         * Creates a reference to an existing dataset in the same hdf5 file.
         * 
         * @param srcID id of the simulation step holding the source dataset
         * @param srcName name of the existing source dataset
         * @param dstID if of the simulation step holding the created reference dataset.
         * If this group does not exist, it is created.
         * @param dstName name of the created reference dataset
         * @param count number of elements referenced from the source dataset
         * @param offset offset in elements in the source dataset
         * @param stride striding in elements in the source dataset
         */
        virtual void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) = 0;

        /**
         * Reads global attribute from hdf5 file.
         *
         * @param name name for the attribute
         * @param data data buffer to read attribute to
         * @param mpiPosition pointer to Dimensions class.
         * Identifies mpi-position-specific custom group.
         * use NULL to read from default group.
         */
        virtual void readGlobalAttribute(
                const char *name,
                void* data,
                Dimensions *mpiPosition = NULL) = 0;

        /**
         * Writes global attribute to hdf5 file (default group).
         *
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param name name of the attribute
         * @param data data buffer to write to attribute
         */
        virtual void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* data) = 0;

        /**
         * Reads an attribute from a single DataSet.
         * 
         * @param id id for fileentry. e.g. iteration
         * @param dataName name of the DataSet in group id to read attribute from
         * @param attrName name of the attribute
         * @param data data data buffer to read attribute to
         * @param mpiPosition pointer to Dimensions class.
         * Identifies mpi-position-specific file while in FAT_READ_MERGED.
         * use NULL to read from default file (0, 0, 0).
         */
        virtual void readAttribute(int32_t id,
                const char *dataName,
                const char *attrName,
                void *data,
                Dimensions *mpiPosition = NULL) = 0;

        /**
         * Writes an attribute to a single DataSet.
         * 
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param dataName name of the DataSet in group id to write attribute to
         * @param attrName name of the attribute
         * @param data data buffer to write to attribute
         */
        virtual void writeAttribute(int32_t id,
                const CollectionType& type,
                const char *dataName,
                const char *attrName,
                const void *data) = 0;

        /**
         * Reads data from hdf5 file.
         * If data is to be read (instead of only a buffer size),
         * the destination buffer (data) has to be allocated already.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param name name for the dataset, e.g. 'ions'
         * @param dstBuffer dimensions of the buffer to read into
         * @param sizeRead returns the dimensions of the read data buffer
         * @param dstOffset offset in destination buffer to read to
         * @param data data buffer to read from file. Can be NULL.
         */
        virtual void read(int32_t id,
                const CollectionType& type,
                const char* name,
                const Dimensions dstBuffer,
                Dimensions &sizeRead,
                const Dimensions dstOffset,
                void* data) = 0;

        /**
         * Reads data from hdf5 file.
         * If data is to be read (instead of only a buffer size),
         * the destination buffer (data) has to be allocated already.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param name name for the dataset, e.g. 'ions'
         * @param sizeRead returns the dimensions of the read data buffer
         * @param data data buffer to read from file. Can be NULL.
         */
        virtual void read(int32_t id,
                const CollectionType& type,
                const char* name,
                Dimensions &sizeRead,
                void* data) = 0;
    };

} // namespace DCollector

#endif	/* _DATACOLLECTOR_H */

