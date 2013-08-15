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

#ifndef IPARALLELDOMAINCOLLECTOR_HPP
#define	IPARALLELDOMAINCOLLECTOR_HPP

#include "IDomainCollector.hpp"

namespace DCollector
{

    class IParallelDomainCollector : public IDomainCollector
    {
    public:
        /**
         * {@link IDomainCollector#writeDomain}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         * 
         * The global domain size and the domain offset for the calling process are
         * determined automatically via MPI among all participating processes.
         */
        virtual void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* data) = 0;

        /**
         * {@link IDomainCollector#writeDomain}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         * 
         * The global domain size and the domain offset for the calling process are
         * determined automatically via MPI among all participating processes.
         */
        virtual void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* data) = 0;

        /**
         * {@link IDomainCollector#writeDomain}
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         * 
         * The global domain size and the domain offset for the calling process are
         * determined automatically via MPI among all participating processes.
         */
        virtual void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* data) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id id of the group to read from
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcData dimensions of the data in the buffer
         * @param name name of the dataset
         * @param globalDomainOffset global offset of this domain
         * @param globalDomainSize global domain size
         * @param dataClass domain type annotation
         * @param data buffer with data
         */
        virtual void writeDomain(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const Dimensions globalDomainOffset,
                const Dimensions globalDomainSize,
                DomDataClass dataClass,
                const void* data) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id id of the group to read from
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcBuffer dimensions of the buffer to read from
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset offset of srcData in srcBuffer
         * @param name name of the dataset
         * @param globalDomainOffset global offset of this domain
         * @param globalDomainSize global domain size
         * @param dataClass domain type annotation
         * @param data buffer with data
         */
        virtual void writeDomain(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const Dimensions globalDomainOffset,
                const Dimensions globalDomainSize,
                DomDataClass dataClass,
                const void* data) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id id of the group to read from
         * @param globalSize dimensions for global collective buffer
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcBuffer dimensions of the buffer to read from
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset offset of srcData in srcBuffer
         * @param name name of the dataset
         * @param globalDomainOffset global offset of this domain
         * @param globalDomainSize global domain size
         * @param dataClass domain type annotation
         * @param data buffer with data
         */
        virtual void writeDomain(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcStride,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const Dimensions globalDomainOffset,
                const Dimensions globalDomainSize,
                DomDataClass dataClass,
                const void* data) = 0;
        
        /**
         * Reserves a dataset with annotated domain information for parallel access. 
         * 
         * @param id id for fileentry. e.g. iteration
         * @param size intended 3D dimension for local dataset
         * @param globalSize if not NULL, returns the global size of the dataset
         * @param globalOffset if not NULL, returns the global offset for the calling process
         * @param rank maximum dimension (must be between 1-3)
         * @param type type information for data. available types must be
         * implemented by concrete DataCollector
         * @param name name for the dataset, e.g. 'ions'
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param dataClass subdomain type annotation
         */
        virtual void reserveDomain(int32_t id,
                const Dimensions size,
                Dimensions *globalSize,
                Dimensions *globalOffset,
                uint32_t rank,
                const CollectionType& type,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass) = 0;
    };

}

#endif	/* IPARALLELDOMAINCOLLECTOR_HPP */

