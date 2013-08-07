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

#ifndef IPARALLELDATACOLLECTOR_HPP
#define	IPARALLELDATACOLLECTOR_HPP

#include "DataCollector.hpp"

namespace DCollector
{

    class IParallelDataCollector : public DataCollector
    {
    public:

        /**
         * {@link DataCollector#write}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         */
        virtual void write(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data) = 0;

        /**
         * {@link DataCollector#write}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
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
         * {@link DataCollector#write}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
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
         * Writes data to HDF5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcData intended 3D dimension for local dataset
         * @param name name for the dataset, e.g. 'ions'
         * @param data data local buffer to write to file
         */
        virtual void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const void* data) = 0;

        /**
         * Writes data to HDF5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcBuffer dimensions of memory buffer
         * @param srcData intended 3D dimension for local dataset
         * @param srcOffset offset of dataset in memory buffer
         * @param name name for the dataset, e.g. 'ions'
         * @param data data buffer to write to file
         */
        virtual void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) = 0;

        /**
         * Writes data to HDF5 file.
         *
         * @param id id for fileentry. e.g. iteration
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param rank maximum dimension (must be between 1-3)
         * @param srcBuffer dimensions of memory buffer
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcData intended 3D dimension for local dataset
         * @param srcOffset offset of dataset in memory buffer
         * @param name name for the dataset, e.g. 'ions'
         * @param data data buffer to write to file
         */
        virtual void write(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const void* data) = 0;

        /**
         * Reads global attribute from HDF5 file.
         *
         * @param id id for iteration
         * @param name name for the attribute
         * @param data data buffer to read attribute to
         */
        virtual void readGlobalAttribute(int32_t id,
                const char* name,
                void* data) = 0;

        /**
         * Writes global attribute to HDF5 file (default group).
         *
         * @param id id for iteration
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param name name of the attribute
         * @param data data buffer to write to attribute
         */
        virtual void writeGlobalAttribute(int32_t id,
                const CollectionType& type,
                const char *name,
                const void* data) = 0;

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
    };
}

#endif	/* IPARALLELDATACOLLECTOR_HPP */

