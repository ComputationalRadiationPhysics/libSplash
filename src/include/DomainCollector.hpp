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

#include "SerialDataCollector.hpp"
#include "Dimensions.hpp"
#include "DCException.hpp"

#include "DataContainer.hpp"
#include "DomainData.hpp"
#include "Domain.hpp"

#define DOMCOL_ATTR_CLASS "_class"
#define DOMCOL_ATTR_SIZE "_size"
#define DOMCOL_ATTR_START "_start"
#define DOMCOL_ATTR_ELEMENTS "_elements"

namespace DCollector
{

    /**
     * DomainCollector extends SerialDataCollector with domain management features.
     * It allows to efficiently read subdomains (sub-partitions) from
     * multi-process hdf5 files with entries annotated with domain information.
     */
    class DomainCollector : public SerialDataCollector
    {
    public:

        /**
         * Annotation (attribute) class for domain data.
         */
        enum DomDataClass
        {
            UndefinedType = 0, PolyType = 10, GridType = 20
        };

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
         * Returns the total number of  domain elements for a dataset 
         * which is the sum of all subdomain elements.
         * This function does not check that domain classes match.
         * 
         * @param id id of the group to read from
         * @param name name of the dataset
         * @return total domain elements
         */
        size_t getTotalElements(int32_t id,
                const char* name) throw (DCException);

        /**
         * Returns the total domain that is spanned by all
         * datasets.
         * The user is responsible to guarantee that the actual subdomains
         * form a line/rectangle/cuboid.
         * 
         * @param id id of the group to read from
         * @param name name of the dataset
         * @return total domain size and offset spanned of all subdomains
         */
        Domain getTotalDomain(int32_t id,
                const char* name) throw (DCException);

        /**
         * Efficiently reads domain-annotated data.
         * In comparison to the standard read method, only data required 
         * for the requested partition is read.
         * It passes back a DataContainer holding all subdomains or subdomain
         * portions, respectively, which make up the requested partition.
         * Data from the files is immediately copied to the DomainData objects in
         * the container and DomainData buffers are dealloated when the DomainData
         * object is deleted, which must be done by the user.
         * 
         * @tparam DomDataType data type of the data to be read (e.g. int, float, ...)
         * @param id id of the group to read from
         * @param name name of the dataset
         * @param domainOffset domain offset for reading (logical start of the requested partition)
         * @param domainSize domain size for reading (logical size of the requested partition)
         * @param dataClass optional domain type annotation (can be NULL)
         * @param lazyLoad set to load only size information for each subdomain, data must be loaded later using readDomainLazy
         * @return returns a pointer to a newly allocated DataContainer holding all subdomains
         */
        DataContainer *readDomain(int32_t id,
                const char* name,
                Dimensions domainOffset,
                Dimensions domainSize,
                DomDataClass* dataClass,
                bool lazyLoad = false) throw (DCException);

        /**
         * Reads a subdomain which has been loaded using readDomain with lazyLoad activated.
         * The DomainCollector instance must not have been closed in between.
         * 
         * @param domainData pointer to subdomain loaded using lazyLoad == true
         */
        void readDomainLazy(DomainData *domainData) throw (DCException);

        /**
         * Writes data with annotated subdomain information.
         * 
         * @param id id of the group to read from
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcData dimensions of the data in the buffer
         * @param name name of the dataset
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param dataClass subdomain type annotation
         * @param data buffer with data
         */
        void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* data) throw (DCException);

        /**
         * Writes data with annotated subdomain information.
         * 
         * @param id id of the group to read from
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcBuffer dimensions of the buffer to read from
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset offset of srcData in srcBuffer
         * @param name name of the dataset
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param dataClass subdomain type annotation
         * @param data buffer with data
         */
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

        /**
         * Writes data with annotated subdomain information.
         * 
         * @param id id of the group to read from
         * @param type type information for data
         * @param rank number of dimensions (1-3) of the data
         * @param srcBuffer dimensions of the buffer to read from
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset offset of srcData in srcBuffer
         * @param name name of the dataset
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param dataClass subdomain type annotation
         * @param data buffer with data
         */
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

        /**
         * Appends 1D data with annotated subdomain information.
         * 
         * @param id id of the group to read from
         * @param type type information for data
         * @param count number of elements to append
         * @param name name for the dataset to create/append to, e.g. 'ions'
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param data buffer with data
         */
        void appendDomain(int32_t id,
                const CollectionType& type,
                uint32_t count,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *data) throw (DCException);

        /**
         * Appends 1D data with annotated subdomain information.
         * 
         * @param id id of the group to read from
         * @param type type information for data
         * @param count number of elements to append
         * @param offset offset in elements to start reading from
         * @param striding striding to be used for reading. 
         * data must contain at least (striding * count) elements. 
         * 1 mean 'no striding'
         * @param name name for the dataset to create/append to, e.g. 'ions'
         * @param domainOffset offset of this subdomain in the domain
         * @param domainSize size of this subdomain in the domain
         * @param data buffer with data
         */
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
        /**
         * Tests if two 1-3 dimensional domains intersect.
         * Both domains must be of the same dimensionality.
         * 
         * @param d1 first domain
         * @param d2 second domain
         * @return if domains overlap
         */
        bool testIntersection(Domain& d1, Domain& d2);

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

