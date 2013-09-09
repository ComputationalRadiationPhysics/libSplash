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

#include "IDomainCollector.hpp"
#include "SerialDataCollector.hpp"
#include "Dimensions.hpp"
#include "DCException.hpp"

namespace DCollector
{

    /**
     * DomainCollector extends SerialDataCollector with domain management features.
     * It allows to efficiently read subdomains (sub-partitions) from
     * multi-process hdf5 files with entries annotated with domain information.
     */
    class DomainCollector : public IDomainCollector, public SerialDataCollector
    {
    public:
        /**
         * Constructor
         * @param maxFileHandles maximum number of concurrently opened file handles (0=unlimited)
         */
        DomainCollector(uint32_t maxFileHandles);

        /**
         * Destructor
         */
        virtual ~DomainCollector();

        /**
         * {@link IDomainCollector#getTotalElements}
         */
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
                const void* data) throw (DCException);

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
                const void* data) throw (DCException);

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
                const void* data) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                uint32_t count,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *data) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                uint32_t count,
                uint32_t offset,
                uint32_t striding,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *data) throw (DCException);

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

