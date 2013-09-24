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

#ifndef IDOMAINCOLLECTOR_HPP
#define	IDOMAINCOLLECTOR_HPP

#include "Dimensions.hpp"
#include "domains/DataContainer.hpp"
#include "domains/DomainData.hpp"
#include "domains/Domain.hpp"

#define DOMCOL_ATTR_CLASS "_class"
#define DOMCOL_ATTR_RANK "_rank"
#define DOMCOL_ATTR_SIZE "_size"
#define DOMCOL_ATTR_START "_start"
#define DOMCOL_ATTR_ELEMENTS "_elements"

namespace DCollector
{

    /**
     * Interface for managing domains.
     * See \ref DomainCollector for more information.
     */
    class IDomainCollector
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
         * Tests if two domains intersect.
         * Both domains must be of the same dimensionality.
         * 
         * @param d1 First domain.
         * @param d2 Decond domain.
         * @return True if domains overlap, false otherwise.
         */
        static bool testIntersection(const Domain& d1, const Domain& d2)
        {
            Dimensions d1_start = d1.getStart();
            Dimensions d2_start = d2.getStart();
            Dimensions d1_end = d1.getEnd();
            Dimensions d2_end = d2.getEnd();

            return (d1_start[0] <= d2_end[0] && d1_end[0] >= d2_start[0] &&
                    d1_start[1] <= d2_end[1] && d1_end[1] >= d2_start[1] &&
                    d1_start[2] <= d2_end[2] && d1_end[2] >= d2_start[2]) ||
                    (d2_start[0] <= d1_end[0] && d2_end[0] >= d1_start[0] &&
                    d2_start[1] <= d1_end[1] && d2_end[1] >= d1_start[1] &&
                    d2_start[2] <= d1_end[2] && d2_end[2] >= d1_start[2]);
        }

        /**
         * Returns the total number of domain elements for a dataset,
         * which is the sum of all subdomain elements.
         * This function does not check that domain classes match.
         * 
         * @param id ID of iteration.
         * @param name name of the dataset
         * @return total domain elements
         */
        virtual size_t getTotalElements(int32_t id,
                const char* name) = 0;

        /**
         * Returns the total domain that is spanned by all
         * datasets (subdomains) of the currently accessed files.
         * The user is responsible to guarantee that the actual subdomains
         * form a line/rectangle/cuboid that starts at (0, 0, 0)
         * 
         * @param id ID of iteration.
         * @param name Name of the dataset.
         * @return Total domain size and offset spanned by all subdomains.
         */
        virtual Domain getTotalDomain(int32_t id,
                const char* name) = 0;

        /**
         * Efficiently reads domain-annotated data.
         * In comparison to the standard read method, only data required 
         * for the requested partition is read.
         * It passes back a \ref DataContainer holding all subdomains or subdomain
         * portions, respectively, which make up the requested partition.
         * Data from the files is immediately copied to the DomainData objects in
         * the container and DomainData buffers are dealloated when the DomainData
         * object is deleted, which must be done by the user.
         * 
         * @tparam DomDataType data type of the data to be read (e.g. int, float, ...)
         * @param id ID of the iteration for reading.
         * @param name name of the dataset
         * @param domainOffset domain offset for reading (logical start of the requested partition)
         * @param domainSize domain size for reading (logical size of the requested partition)
         * @param dataClass optional domain type annotation (can be NULL)
         * @param lazyLoad set to load only size information for each subdomain, data must be loaded later using readDomainLazy
         * @return returns a pointer to a newly allocated DataContainer holding all subdomains
         */
        virtual DataContainer *readDomain(int32_t id,
                const char* name,
                Dimensions domainOffset,
                Dimensions domainSize,
                DomDataClass* dataClass,
                bool lazyLoad = false) = 0;

        /**
         * Reads a subdomain which has been loaded using readDomain with lazyLoad activated.
         * The DomainCollector instance must not have been closed in between.
         * Currently only supported for domain annotated data of type DomDataClass::PolyType.
         * 
         * @param domainData Pointer to subdomain loaded using lazyLoad == true.
         */
        virtual void readDomainLazy(DomainData *domainData) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id ID of the iteration for writing.
         * @param type Type information for data.
         * @param rank Number of dimensions (1-3).
         * @param srcData dimensions of the data in the buffer
         * @param name Name for the dataset to create/append.
         * @param domainOffset Offset of this subdomain in the domain.
         * @param domainSize Size of this subdomain in the domain.
         * @param dataClass Subdomain type annotation.
         * @param buf Buffer with data.
         */
        virtual void writeDomain(int32_t id,
                const CollectionType& type,
                uint32_t rank,
                const Dimensions srcData,
                const char* name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                DomDataClass dataClass,
                const void* buf) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id ID of the iteration for writing.
         * @param type Type information for data.
         * @param rank Number of dimensions (1-3).
         * @param srcBuffer Size of the buffer to read from.
         * @param srcData dimensions of the data in the buffer
         * @param srcOffset Offset of \p srcData in \p srcBuffer.
         * @param name Name for the dataset to create/append.
         * @param domainOffset Offset of this subdomain in the domain.
         * @param domainSize Size of this subdomain in the domain.
         * @param dataClass Subdomain type annotation.
         * @param buf Buffer with data.
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
                const void* buf) = 0;

        /**
         * Writes data with annotated domain information.
         * 
         * @param id ID of the iteration for writing.
         * @param type Type information for data.
         * @param rank Number of dimensions (1-3).
         * @param srcBuffer Size of the buffer to read from.
         * @param srcStride Size of striding in each dimension. 1 means 'no stride'.
         * @param srcData Size of the data in the buffer.
         * @param srcOffset Offset of \p srcData in \p srcBuffer.
         * @param name Name for the dataset to create/append.
         * @param domainOffset Offset of this subdomain in the domain.
         * @param domainSize Size of this subdomain in the domain.
         * @param dataClass Subdomain type annotation.
         * @param buf Buffer with data.
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
                const void* buf) = 0;

        /**
         * Appends 1D data with annotated domain information.
         * 
         * @param id ID of iteration.
         * @param type Type information for data.
         * @param count Number of elements to append.
         * @param name Name for the dataset to create/append.
         * @param domainOffset Offset of this subdomain in the domain.
         * @param domainSize Size of this subdomain in the domain.
         * @param buf Buffer with data.
         */
        virtual void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *buf) = 0;

        /**
         * Appends 1D data with annotated domain information.
         * 
         * @param id ID of iteration.
         * @param type Type information for data.
         * @param count Number of elements to append.
         * @param offset Offset in elements to start reading from.
         * @param striding Striding to be used for reading. 
         * Data must contain at least (striding * count) elements. 
         * 1 mean 'no striding'.
         * @param name Name for the dataset to create/append.
         * @param domainOffset Offset of this subdomain in the domain.
         * @param domainSize Size of this subdomain in the domain.
         * @param buf Buffer with data.
         */
        virtual void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t striding,
                const char *name,
                const Dimensions domainOffset,
                const Dimensions domainSize,
                const void *buf) = 0;
    };

}

#endif	/* IDOMAINCOLLECTOR_HPP */

