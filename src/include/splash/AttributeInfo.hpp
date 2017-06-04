/**
 * Copyright 2016 Alexander Grund
 *
 * This file is part of libSplash.
 *
 * libSplash is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
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

#ifndef DCATTRIBUTE_INFO_H
#define DCATTRIBUTE_INFO_H

#include "splash/Dimensions.hpp"
#include "splash/DCException.hpp"
#include "splash/core/H5IdWrapper.hpp"
#include "splash/core/splashMacros.hpp"
#include <string>

namespace splash
{
    class CollectionType;

    /**
     * Class holding information about an attribute.
     *
     * Usage example:
     * \code{.cpp}
     * AttributeInfo info = dc.readAttribute(iterationId, "groupName", "attrName");
     * // Check that extents are as expected
     * if(!info.isScalar()) throw error(...);
     * // Optionally further checks
     * ...
     * int result;
     * info.read(ColTypeInt(), &result);
     * \endcode
     */
    class AttributeInfo
    {
        H5AttributeId attr_;

   public:
        explicit AttributeInfo(hid_t attr);
        /** Construct from an attribute id, takes over ownership! */
        explicit AttributeInfo(H5AttributeId& attr);
        ~AttributeInfo();

        AttributeInfo(const AttributeInfo& other);
        AttributeInfo& operator=(const AttributeInfo& other);

        /** Return the size of the required memory in bytes when querying the value */
        size_t getMemSize() throw(DCException);
        /** Return the CollectionType. Might be ColTypeUnknown */
        const CollectionType& getType() throw(DCException);
        /** Return the size in each dimension */
        Dimensions getDims() throw(DCException);
        /** Return the number of dimensions */
        uint32_t getNDims() throw(DCException);
        /** Return whether this is a scalar value */
        bool isScalar()  throw(DCException){ return getNDims() == 1 && getDims().getScalarSize() == 1; }
        /** Return true, if the attribute has a variable size in which case reading
         *  it will only return its pointer */
        bool isVarSize() throw(DCException);
        /** Read the name of the attribute. Returns an empty string on error */
        std::string readName();

        /**
         * Read the data of this attribute into the buffer of the given type.
         * Data conversion may occur, if required.
         * See the HDF5 manual for details (6.10. Data Transfer: Datatype Conversion and Selection)
         * Throws an exception if the attribute could not be read or converted
         *
         * @param colType Type of the buffer
         * @param buf     Pointer to buffer. Size must be equal to \ref getMemSize
         */
        void read(const CollectionType& colType, void* buf) throw(DCException);

        /**
         * Read the data of this attribute into the buffer of the given type.
         * Data conversion may occur, if required.
         * See the HDF5 manual for details (6.10. Data Transfer: Datatype Conversion and Selection)
         * Does not throw exception, so it can be used for trying different types
         *
         * @param colType Type of the buffer
         * @param buf     Pointer to buffer. Size must be equal to \ref getMemSize
         * @return True on success, else false
         */
        bool readNoThrow(const CollectionType& colType, void* buf);

        /**
         * Read the data of this attribute into the buffer of the given size.
         * No data conversion occurs, so the type as found in the file is used.
         * Throws an exception if the attribute could not be read or the buffer is to small.
         *
         * @param buf     Pointer to buffer
         * @param bufSize Size of the buffer
         */
        SPLASH_DEPRECATED("Pass CollectionType")
        void read(void* buf, size_t bufSize) throw(DCException);
    private:

        std::string getExceptionString(const std::string& msg);
        void loadType() throw(DCException);
        void loadSpace() throw(DCException);
        
        /** Reference counter for the attribute */
        unsigned* refCt_;
        /** Decreases the ref counter by 1 and releases the attribute or the ref counter */
        void releaseReference();

        // Those values are lazy loaded on access
        CollectionType* colType_;
        H5TypeId type_;
        H5DataspaceId space_;
        uint32_t nDims_;
};
}

#endif /* DCATTRIBUTE_INFO_H */
