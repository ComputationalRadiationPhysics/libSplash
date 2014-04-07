/**
 * Copyright 2013-2014 Felix Schmitt
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
 * @authors Axel Huebl
 */

#ifndef _DATACOLLECTOR_H
#define	_DATACOLLECTOR_H

#include <stdint.h>

#include "splash/CollectionType.hpp"
#include "splash/Dimensions.hpp"
#include "splash/Selection.hpp"

namespace splash
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

        /**
         * Attributes passed when calling {@link DataCollector#open}
         */
        typedef struct _FileCreationAttr
        {

            _FileCreationAttr() :
            fileAccType(FAT_CREATE),
            mpiSize(1, 1, 1),
            mpiPosition(0, 0, 0),
            enableCompression(false)
            {

            }

            /**
             * File access mode.
             */
            FileAccType fileAccType;
            
            /**
             * MPI topology.
             */
            Dimensions mpiSize;
            
            /**
             * MPi position.
             */
            Dimensions mpiPosition;
            
            /**
             * Enable compression, if supported.
             */
            bool enableCompression;
        } FileCreationAttr;

        /**
         * Information on a specific dataset in a HDF5 file.
         */
        typedef struct _DCEntry
        {
            /**
             * Fully-qualified name of this dataset.
             */
            std::string name;
        } DCEntry;

        /**
         * Initializes FileCreationAttr with default values.
         * (compression = false, access type = FAT_CREATE, position = (0, 0, 0), size = (1, 1, 1))
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

        /**
         * Destructor
         */
        virtual ~DataCollector()
        {
        };

        /**
         * Opens one or multiple files.
         *
         * @param filename Name of the file(s) to open (the common part without index identifiers).
         * @param attr Struct passing several parameters on how to access files.
         */
        virtual void open(
                const char *filename,
                FileCreationAttr& attr) = 0;

        /**
         * Closes open files and releases internal buffers.
         *
         * Must be called by user when finished writing/reading data.
         */
        virtual void close() = 0;

        /**
         * @return Returns highest iteration ID.
         */
        virtual int32_t getMaxID() = 0;

        /**
         * Returns the MPI topology used for creating files.
         * @param mpiSize Contains MPI topology.
         */
        virtual void getMPISize(Dimensions& mpiSize) = 0;

        /**
         * Returns all IDs in an opened file.
         * The caller must ensure that there is enough space in \p ids
         * to hold \p count elements.
         *
         * @param ids Pointer to an array with at least \p count entries, can be NULL.
         * @param count Returns the number of entries in \p ids, can be NULL.
         */
        virtual void getEntryIDs(int32_t *ids, size_t *count) = 0;

        /**
         * Returns all datasets for an ID.
         * The caller must ensure that there is enough space in \p entries
         * to hold \p count elements.
         * 
         * @param id ID for iteration.
         * @param entries Pointer to an array with at least \p count elements, can be NULL.
         * @param count Returns the number of elements in \p entries, can be NULL.
         */
        virtual void getEntriesForID(int32_t id, DCEntry *entries, size_t *count) = 0;

        /**
         * Writes data to HDF5 file.
         *
         * @param id ID for iteration.
         * @param type Type information for data.
         * @param ndims Number of dimensions (1-3).
         * @param select Selection in src buffer
         * @param name Name for the dataset.
         * @param buf Buffer for writing.
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const void* buf) = 0;

        /**
         * Appends 1-dimensional data in a HDF5 file.
         *
         * The target dataset is created if necessary.
         * If it already exists, data is appended to the end.
         *
         * @param id ID for iteration.
         * @param type Type information for data.
         * @param count Number of elements to append.
         * @param name Name for the dataset to create/append.
         * @param buf Buffer to append.
         */
        virtual void append(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const void *buf) = 0;

        /**
         * Appends 1-dimensional data in a HDF5 file using striding.
         *
         * The target dataset is created if necessary.
         * If it already exists, data is appended to the end.
         * The data is read using striding from source buffer \p buf.
         *
         * @param id id for fileentry. e.g. iteration
         * @param type Type information for data.
         * @param count Number of elements to append.
         * @param offset Offset in elements to start reading from in \p buf.
         * @param stride Striding to be used for reading from \p buf, 1 means 'no striding'.
         * \p buf must contain at least (striding * count) elements. 
         * @param name Name for the dataset to create/append.
         * @param buf Buffer to append.
         */
        virtual void append(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t stride,
                const char *name,
                const void *buf) = 0;

        /**
         * Removes a simulation step from the HDF5 file.
         * 
         * @param id ID to remove.
         */
        virtual void remove(int32_t id) = 0;

        /**
         * Removes a dataset from a HDF5 file.
         * 
         * @param id ID holding the dataset to be removed.
         * @param name Name of the dataset to be removed.
         */
        virtual void remove(int32_t id,
                const char *name) = 0;

        /**
         * Creates an object reference to an existing dataset in the same HDF5 file.
         * 
         * @param srcID ID of the iteration holding the source dataset.
         * @param srcName Name of the existing source dataset.
         * @param dstID ID of the simulation step holding the created reference dataset.
         * If this group does not exist, it is created.
         * @param dstName Name of the created reference.
         */
        virtual void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName) = 0;

        /**
         * Creates a dataset region reference to an existing dataset in the same HDF5 file.
         * 
         * @param srcID ID of the iteration holding the source dataset.
         * @param srcName Name of the existing source dataset.
         * @param dstID ID of the simulation step holding the created reference dataset.
         * If this group does not exist, it is created.
         * @param dstName Name of the created reference.
         * @param count Number of elements referenced from the source dataset.
         * @param offset Offset in elements in the source dataset.
         * @param stride Striding in elements in the source dataset.
         */
        virtual void createReference(int32_t srcID,
                const char *srcName,
                int32_t dstID,
                const char *dstName,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) = 0;

        /**
         * Reads global attribute from HDF5 file.
         *
         * @param name Name for the attribute.
         * @param buf Buffer to read attribute to.
         * @param mpiPosition Pointer to Dimensions class.
         * Identifies MPI-position-specific custom group.
         * Use NULL to read from default group.
         */
        virtual void readGlobalAttribute(
                const char *name,
                void* buf,
                Dimensions *mpiPosition = NULL) = 0;

        /**
         * Writes global attribute to HDF5 file (default group).
         *
         * @param type Type information for data.
         * @param name Name of the attribute.
         * @param buf Buffer to be written as attribute.
         */
        virtual void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* buf) = 0;

        /**
         * Reads an attribute from a single dataset.
         * 
         * @param id ID for iteration.
         * @param dataName Name of the dataset in group \p id to read attribute from.
         * If dataName is NULL, the attribute is read from the iteration group.
         * @param attrName Name of the attribute.
         * @param buf Buffer to read attribute to.
         * @param mpiPosition Pointer to Dimensions class.
         * Identifies MPI-position-specific file while in FAT_READ_MERGED mode.
         * Use NULL to read from default file index.
         */
        virtual void readAttribute(int32_t id,
                const char *dataName,
                const char *attrName,
                void *buf,
                Dimensions *mpiPosition = NULL) = 0;

        /**
         * Writes an attribute to a single dataset.
         * 
         * @param id ID for iteration.
         * @param type Type information for data.
         * @param dataName Name of the dataset in group \p id to write attribute to.
         * If dataName is NULL, the attribute is written for the iteration group.
         * @param attrName Name of the attribute.
         * @param buf Buffer to be written as attribute.
         */
        virtual void writeAttribute(int32_t id,
                const CollectionType& type,
                const char *dataName,
                const char *attrName,
                const void *buf) = 0;
        
        /**
         * Reads data from HDF5 file.
         * If data is to be read (instead of only its size in the file),
         * the destination buffer (\p buf) must be allocated already.
         *
         * @param id ID for iteration.
         * @param name Name for the dataset.
         * @param sizeRead Returns the size of the data in the file.
         * @param buf Buffer to read from file, can be NULL.
         */
        virtual void read(int32_t id,
                const char* name,
                Dimensions &sizeRead,
                void* buf) = 0;

        /**
         * Reads data from HDF5 file.
         * If data is to be read (instead of only its size in the file),
         * the destination buffer (\p buf) must be allocated already.
         *
         * @param id ID for iteration.
         * @param name Name for the dataset.
         * @param dstBuffer Size of the buffer \p buf to read to.
         * @param dstOffset Offset in destination buffer to read to.
         * @param sizeRead Returns the size of the data in the file.
         * @param buf Buffer to read from file, can be NULL.
         */
        virtual void read(int32_t id,
                const char* name,
                const Dimensions dstBuffer,
                const Dimensions dstOffset,
                Dimensions &sizeRead,
                void* buf) = 0;
    };

}

#endif	/* _DATACOLLECTOR_H */

