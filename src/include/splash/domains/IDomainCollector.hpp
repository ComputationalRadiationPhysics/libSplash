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

#ifndef IDOMAINCOLLECTOR_HPP
#define	IDOMAINCOLLECTOR_HPP

#include "splash/Dimensions.hpp"
#include "splash/Selection.hpp"
#include "splash/domains/DataContainer.hpp"
#include "splash/domains/DomainData.hpp"
#include "splash/domains/Domain.hpp"

#define DOMCOL_ATTR_CLASS "_class"
#define DOMCOL_ATTR_SIZE "_size"
#define DOMCOL_ATTR_GLOBAL_SIZE "_global_size"
#define DOMCOL_ATTR_OFFSET "_start"
#define DOMCOL_ATTR_GLOBAL_OFFSET "_global_start"
#define DOMCOL_ATTR_ELEMENTS "_elements"

namespace splash
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
         * Returns the global domain that is spanned by all
         * local datasets (subdomains) of the currently accessed files.
         * The user is responsible to guarantee that the actual subdomains
         * form a line/rectangle/cuboid.
         * 
         * @param id ID of iteration.
         * @param name Name of the dataset.
         * @return Global domain spanned by all subdomains.
         */
        virtual Domain getGlobalDomain(int32_t id,
                const char* name) = 0;

        /**
         * Returns the local domain for a dataset 
         * in currently accessed file.
         * 
         * @param id ID of iteration.
         * @param name Name of the dataset.
         * @return Local sudomain.
         */
        virtual Domain getLocalDomain(int32_t id,
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
         * @param id ID of the iteration.
         * @param name Name of the dataset.
         * @param domain Domain for reading.
         * @param dataClass Optional domain type annotation, can be NULL.
         * @param lazyLoad Set to load only size information for each subdomain,
         * data must be loaded later using \ref IDomainCollector::readDomainLazy.
         * @return Returns a pointer to a newly allocated DataContainer holding all subdomains.
         */
        virtual DataContainer *readDomain(int32_t id,
                const char* name,
                const Domain domain,
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
         * @param ndims Number of dimensions (1-3).
         * @param select Selection in src buffer.
         * @param name Name for the dataset to create/append.
         * @param localDomain Local subdomain.
         * @param globalDomain Logical global domain.
         * @param dataClass Subdomain type annotation.
         * @param buf Buffer with data.
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
         * Appends 1D data with annotated domain information.
         * 
         * @param id ID of iteration.
         * @param type Type information for data.
         * @param count Number of elements to append.
         * @param name Name for the dataset to create/append.
         * @param localDomain Local subdomain.
         * @param globalDomain Logical global domain.
         * @param buf Buffer with data.
         */
        virtual void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                const char *name,
                const Domain localDomain,
                const Domain globalDomain,
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
         * @param localDomain Local subdomain.
         * @param globalDomain Logical global domain.
         * @param buf Buffer with data.
         */
        virtual void appendDomain(int32_t id,
                const CollectionType& type,
                size_t count,
                size_t offset,
                size_t striding,
                const char *name,
                const Domain localDomain,
                const Domain globalDomain,
                const void *buf) = 0;
    };

}

#endif	/* IDOMAINCOLLECTOR_HPP */

