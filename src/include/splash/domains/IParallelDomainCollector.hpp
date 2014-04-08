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

#ifndef IPARALLELDOMAINCOLLECTOR_HPP
#define	IPARALLELDOMAINCOLLECTOR_HPP

#include "splash/domains/IDomainCollector.hpp"

namespace splash
{

    class IParallelDomainCollector : public IDomainCollector
    {
    public:
        /**
         * The global size and the write offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         * 
         * The global domain size and the domain offset for the calling process are
         * determined automatically via MPI among all participating processes.
         */
        virtual void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const Domain localDomain,
                const Domain globalDomain,
                DomDataClass dataClass,
                const void* buf) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id ID for iteration.
         * @param globalSize Dimensions for global collective buffer.
         * @param globalOffset 3D-offset in the globalSize-buffer this process writes to.
         * @param type Type information for data.
         * @param ndims Number of dimensions (1-3) of the buffer.
         * @param select Selection in src buffer.
         * @param name Name of the dataset.
         * @param globalDomain Global domain information.
         * @param dataClass Domain type annotation.
         * @param buf Buffer with data.
         */
        virtual void writeDomain(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const Domain globalDomain,
                DomDataClass dataClass,
                const void* buf) = 0;
        
        /**
         * Reserves a dataset with annotated domain information for parallel access. 
         * 
         * @param id ID for iteration.
         * @param globalSize Global size for reserved dataset.
         * @param ndims Number of dimensions (1-3).
         * @param type Type information for data.
         * @param name Name for the dataset.
         * @param domain Global domain information.
         * @param dataClass Subdomain type annotation.
         */
        virtual void reserveDomain(int32_t id,
                const Dimensions globalSize,
                uint32_t ndims,
                const CollectionType& type,
                const char* name,
                const Domain domain,
                DomDataClass dataClass) = 0;
        
        /**
         * Reserves a dataset with annotated domain information for parallel access. 
         * 
         * The global size and the global offset for the calling process are
         * determined automatically via MPI among all participating processes.
         * Note: This is not possible when writing 2D data with a 3D MPI topology.
         * 
         * @param id ID for iteration.
         * @param size Global size for reserved dataset.
         * @param globalSize Returns the global size of the dataset, can be NULL.
         * @param globalOffset Returns the global offset for the calling process, can be NULL.
         * @param ndims Number of dimensions (1-3).
         * @param type Type information for data.
         * @param name Name for the dataset.
         * @param domain Global domain information.
         * @param dataClass Subdomain type annotation.
         */
        virtual void reserveDomain(int32_t id,
                const Dimensions size,
                Dimensions *globalSize,
                Dimensions *globalOffset,
                uint32_t ndims,
                const CollectionType& type,
                const char* name,
                const Domain domain,
                DomDataClass dataClass) = 0;
    };

}

#endif	/* IPARALLELDOMAINCOLLECTOR_HPP */

