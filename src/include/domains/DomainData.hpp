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



#ifndef DOMAINDATA_HPP
#define	DOMAINDATA_HPP

#include <sstream>
#include <cassert>
#include <string>
#include <hdf5.h>

#include "Dimensions.hpp"
#include "domains/Domain.hpp"
#include "core/DCDataSet.hpp"

namespace DCollector
{

    /**
     * Internal loading reference data structure.
     */
    typedef struct
    {
        int dataClass;
        hid_t handle;
        int32_t id;
        std::string name;
        Dimensions dstBuffer;
        Dimensions dstOffset;
        Dimensions srcSize;
        Dimensions srcOffset;
    } DomainH5Ref;

    /**
     * Extends a Domain with actual data elements.
     */
    class DomainData : public Domain
    {
    public:

        /**
         * Constructor.
         * Allocates enough memory to hold 'elements' data of 'type'.
         * 
         * @param domain the underlying Domain.
         * @param elements the number of data elements in every dimension.
         * @param datatypeSize size of each element in bytes
         * @param datatype hdf5 datatype
         */
        DomainData(Domain& domain, const Dimensions elements,
                size_t datatypeSize, DCDataSet::DCDataType datatype) :
        Domain(domain),
        elements(elements),
        data(NULL),
        loadingReference(NULL),
        datatype(datatype),
        datatypeSize(datatypeSize)
        {
            data = new uint8_t[datatypeSize * elements.getScalarSize()];
            assert(data != NULL);
        }

        /**
         * Destructor.
         * Deletes allocated memory.
         */
        virtual ~DomainData()
        {
            if (loadingReference != NULL)
            {
                delete loadingReference;
                loadingReference = NULL;
            }

            freeData();
        }

        /**
         * Returns the number of data elements in this domain.
         * 
         * @return number of data elements
         */
        Dimensions &getElements()
        {
            return elements;
        }

        /**
         * Returns a pointer to the internal buffer.
         * The pointer has to be casted to the actual data type.
         * 
         * @return pointer to internal buffer.
         */
        void* getData()
        {
            return data;
        }

        /**
         * Deallocate data of this subdomain, should be used for lazy loading.
         */
        void freeData()
        {
            if (data != NULL)
            {
                delete[] data;
                data = NULL;
            }
        }

        /**
         * Returns the size in bytes of the buffer's data type.
         * 
         * @return size of data type in bytes
         */
        size_t getTypeSize()
        {
            return datatypeSize;
        }

        /**
         * Returns the DCDataType of this DomainData.
         * 
         * @return the datatype
         */
        DCDataSet::DCDataType getDataType()
        {
            return datatype;
        }

        /**
         * Set the internal loading reference for lazy loading.
         */
        void setLoadingReference(int dataClass, hid_t handle, int32_t id,
                const char *name, const Dimensions dstBuffer,
                const Dimensions dstOffset, const Dimensions srcSize,
                const Dimensions srcOffset)
        {
            if (loadingReference != NULL)
            {
                delete loadingReference;
                loadingReference = NULL;
            }

            loadingReference = new DomainH5Ref();
            loadingReference->dataClass = dataClass;
            loadingReference->handle = handle;
            loadingReference->id = id;
            loadingReference->name = name;
            loadingReference->dstBuffer.set(dstBuffer);
            loadingReference->dstOffset.set(dstOffset);
            loadingReference->srcSize.set(srcSize);
            loadingReference->srcOffset.set(srcOffset);
        }

        /**
         * Rteurns the internal loading reference for lazy loading.
         * 
         * @return internal loading reference
         */
        DomainH5Ref *getLoadingReference()
        {
            return loadingReference;
        }

        /**
         * Returns the format string for the data type, e.g. %lu or %f.
         * If no format string can be generated, e.g. if the data type is
         * a compound type, an exception is thrown.
         * 
         * @return format string for the data type
         */
        std::string getFormatString() throw (DCException)
        {
            std::stringstream format_string;
            format_string << "%";

            switch (datatype)
            {
                case DCDataSet::DCDT_FLOAT32:
                case DCDataSet::DCDT_FLOAT64:
                    format_string << "f";
                    break;
                case DCDataSet::DCDT_UINT32:
                    format_string << "u";
                    break;
                case DCDataSet::DCDT_UINT64:
                    format_string << "lu";
                    break;
                case DCDataSet::DCDT_INT32:
                    format_string << "d";
                    break;
                case DCDataSet::DCDT_INT64:
                    format_string << "ld";
                    break;
                default:
                    throw DCException("cannot identify datatype");
            }

            return format_string.str();
        }

    protected:
        Dimensions elements;
        uint8_t* data;
        DomainH5Ref *loadingReference;

        DCDataSet::DCDataType datatype;
        size_t datatypeSize;
    };

}

#endif	/* DOMAINDATA_HPP */

