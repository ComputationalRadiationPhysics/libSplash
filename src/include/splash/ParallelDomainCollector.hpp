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

#ifndef PARALLELDOMAINCOLLECTOR_HPP
#define	PARALLELDOMAINCOLLECTOR_HPP

#include "splash/domains/IParallelDomainCollector.hpp"
#include "splash/ParallelDataCollector.hpp"

namespace splash
{

    /**
     * \image html domains_parallel.jpg
     */
    class ParallelDomainCollector : public IParallelDomainCollector, public ParallelDataCollector
    {
    private:
        /**
         * Internal function for formatting exception messages.
         * 
         * @param func name of the throwing function
         * @param msg exception message
         * @param info optional info text to be appended, e.g. the group name
         * @return formatted exception message string
         */
        static std::string getExceptionString(std::string func, std::string msg,
                const char *info);

        void writeDomainAttributes(
                int32_t id,
                const char *name,
                DomDataClass dataClass,
                const Domain localDomain,
                const Domain globalDomain);

    public:
        /**
         * Constructor
         * 
         * @param comm The communicator.
         * All processes in this communicator must participate in accessing data.
         * @param info The MPI_Info object.
         * @param topology Number of MPI processes in each dimension.
         * @param maxFileHandles Maximum number of concurrently opened file handles (0=infinite).
         */
        ParallelDomainCollector(MPI_Comm comm, MPI_Info info, const Dimensions topology,
                uint32_t maxFileHandles);

        /**
         * Destructor
         */
        virtual ~ParallelDomainCollector();

        /**
         * Returns the global domain information for a dataset.
         * 
         * @param id dataset iteration
         * @param name dataset name
         * @return global domain
         */
        Domain getGlobalDomain(int32_t id,
                const char* name) throw (DCException);

        /**
         * Returns the local domain information for a dataset.
         * 
         * @param id dataset iteration
         * @param name dataset name
         * @return global domain
         */
        Domain getLocalDomain(int32_t id,
                const char* name) throw (DCException);

        DataContainer *readDomain(int32_t id,
                const char* name,
                const Domain requestDomain,
                DomDataClass* dataClass,
                bool lazyLoad = false) throw (DCException);

        void readDomainLazy(DomainData *domainData) throw (DCException);

        void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const Domain localDomain,
                const Domain globalDomain,
                DomDataClass dataClass,
                const void* buf) throw (DCException);

        void writeDomain(int32_t id,
                const Dimensions globalSize,
                const Dimensions globalOffset,
                const CollectionType& type,
                uint32_t ndims,
                const Selection select,
                const char* name,
                const Domain globalDomain,
                DomDataClass dataClass,
                const void* buf) throw (DCException);

        void reserveDomain(int32_t id,
                const Dimensions globalSize,
                uint32_t ndims,
                const CollectionType& type,
                const char* name,
                const Domain domain,
                DomDataClass dataClass) throw (DCException);

        void reserveDomain(int32_t id,
                const Dimensions size,
                Dimensions *globalSize,
                Dimensions *globalOffset,
                uint32_t ndims,
                const CollectionType& type,
                const char* name,
                const Domain domain,
                DomDataClass dataClass) throw (DCException);

    protected:

        bool readDomainDataForRank(
                DataContainer *dataContainer,
                DomDataClass *dataClass,
                int32_t id,
                const char* name,
                const Domain requestDomain,
                bool lazyLoad) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const Domain localDomain,
                const Domain globalDomain,
                const void *buf) throw (DCException);

        void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t striding,
                const char *name,
                const Domain localDomain,
                const Domain globalDomain,
                const void *buf) throw (DCException);

    };

}

#endif	/* PARALLELDOMAINCOLLECTOR_HPP */

