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
 


#ifndef DOMAINCOLLECTOR_HPP
#define	DOMAINCOLLECTOR_HPP

#include "domains/IDomainCollector.hpp"
#include "SerialDataCollector.hpp"
#include "Dimensions.hpp"
#include "DCException.hpp"

namespace DCollector
{

    /**
     * DomainCollector extends SerialDataCollector with domain management features.
     * A domain is a logical view to data in memory in or files, in contrast to the
     * physical/memory view.
     * DomainCollector allows to efficiently read subdomains (sub-partitions) from
     * multi-process HDF5 files with entries annotated with domain information.
     * 
     * The following concept is used:
     * Each process (of the MPI topology) annotates its local data with
     * local subdomain information when writing.
     * When reading from these files, data from all files can be accessed transparently
     * as if in one single file. This global view uses the global domain information,
     * created from all local subdomains.
     */
    class DomainCollector : public IDomainCollector, public SerialDataCollector
    {
    public:
        /**
         * Constructor
         * @param maxFileHandles Maximum number of concurrently opened file handles (0=unlimited).
         */
        DomainCollector(uint32_t maxFileHandles);

        /**
         * Destructor
         */
        virtual ~DomainCollector();

        size_t getTotalElements(int32_t id,
                const char* name) throw (DCException);

        Domain getTotalDomain(int32_t id,
                const char* name) throw (DCException);

        DataContainer *readDomain(int32_t id,
                const char* name,
                Dimensions domainOffset,
                Dimensions domainSize,
                DomDataClass* dataClass,
                bool lazyLoad = false) throw (DCException);

        void readDomainLazy(DomainData *domainData) throw (DCException);

        void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* buf) throw (DCException);

        void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcBuffer,
                const Dimensions srcData,
                const Dimensions srcOffset,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* buf) throw (DCException);

        void writeDomain(int32_t id,
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
                const void* buf) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *buf) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t striding,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *buf) throw (DCException);

    protected:
        bool readDomainInfoForRank(
                Dimensions mpiPosition,
                int32_t id,
                const char* name,
                Dimensions requestOffset,
                Dimensions requestSize,
                Domain &fileDomain);

        bool readDomainDataForRank(
                DataContainer *dataContainer,
                DomDataClass *dataClass,
                Dimensions mpiPosition,
                int32_t id,
                const char* name,
                Dimensions requestOffset,
                Dimensions requestSize,
                bool lazyLoad);
    };

}

#endif	/* DOMAINCOLLECTOR_HPP */

