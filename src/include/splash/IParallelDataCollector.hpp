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

#ifndef IPARALLELDATACOLLECTOR_HPP
#define	IPARALLELDATACOLLECTOR_HPP

#include "splash/DataCollector.hpp"

namespace splash
{

    /**
     * Interface for a parallel DataCollector.
     */
    class IParallelDataCollector : public DataCollector
    {
    public:

        /**
         * Implements \ref DataCollector::write
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Selection select,
                const char* name,
                const void* buf) = 0;

        /**
         * Writes data to HDF5 file.
         *
         * @param id ID for iteration
         * @param globalSize Size of global collective write buffer.
         * @param globalOffset Offset in \p globalSize buffer where this process writes to.
         * @param type Type information for data.
         * @param rank Number of dimensions (1-3).
         * @param select Selection in src buffer.
         * @param name Name for the dataset.
         * @param buf Local buffer for writing.
         */
        virtual void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Selection select,
                const char* name,
                const void* buf) = 0;
        
        /**
         * Reserves a dataset for parallel access. 
         * 
         * @param id ID for iteration.
         * @param globalSize Size to reserve for global data.
         * @param rank Number of dimensions (1-3).
         * @param type Type information for data.
         * @param name Name for the dataset.
         */
        virtual void reserve(int32_t id,
                const Dimensions globalSize,
                uint32_t rank,
                const CollectionType& type,
                const char* name) = 0;

        /**
         * Reserves a dataset for parallel access. 
         * 
         * @param id ID for iteration.
         * @param size Size to reserve for local data.
         * @param globalSize Returns the global size of the dataset, can be NULL.
         * @param globalOffset Returns the global offset for the calling process, can be NULL.
         * @param rank Number of dimensions (1-3).
         * @param type Type information for data.
         * @param name Name for the dataset.
         */
        virtual void reserve(int32_t id,
                const Dimensions size,
                Dimensions *globalSize,
                Dimensions *globalOffset,
                uint32_t rank,
                const CollectionType& type,
                const char* name) = 0;

        /**
         * Reads global attribute from HDF5 file.
         *
         * @param id ID for iteration.
         * @param name Name of the attribute.
         * @param buf Destination buffer for attribute.
         */
        virtual void readGlobalAttribute(int32_t id,
                const char* name,
                void* buf) = 0;

        /**
         * Writes global attribute to HDF5 file (default group).
         *
         * @param id ID for iteration.
         * @param type Type information for data.
         * @param name Name of the attribute.
         * @param buf Source buffer for attribute.
         */
        virtual void writeGlobalAttribute(int32_t id,
                const CollectionType& type,
                const char *name,
                const void* buf) = 0;

        virtual void writeGlobalAttribute(int32_t id,
                const CollectionType& type,
                const char *name,
                uint32_t ndims,
                const Dimensions dims,
                const void* buf) = 0;

        virtual void readGlobalAttribute(const char *name,
                void* buf,
                Dimensions *mpiPosition = NULL) = 0;

        virtual void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                const void* buf) = 0;

        virtual void writeGlobalAttribute(const CollectionType& type,
                const char *name,
                uint32_t ndims,
                const Dimensions dims,
                const void* buf) = 0;

        /**
         * Appends (1-3) dimensional data to a dataset created with
         * \ref IParallelDataCollector::reserve.
         *
         * @param id ID for iteration.
         * @param size Size of the data to be appended.
         * @param rank Number of dimensions (1-3),
         * @param globalOffset Offset in destination dataset to append at.
         * @param name Name for the dataset to append to.
         * @param buf Buffer to append.
         */
        virtual void append(int32_t id,
                const Dimensions size,
                uint32_t rank,
                const Dimensions globalOffset,
                const char *name,
                const void *buf) = 0;
        
        virtual void append(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const void *buf) = 0;

        virtual void append(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t stride,
                const char *name,
                const void *buf) = 0;
        
        /**
         * Finalizes by freeing all MPI resources.
         * Must be called before MPI_Finalize.
         */
        virtual void finalize(void) = 0;

    private:
    };
}

#endif	/* IPARALLELDATACOLLECTOR_HPP */

