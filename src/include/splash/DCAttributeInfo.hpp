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
#include "splash/core/H5IdWrapper.hpp"

namespace splash
{
    class CollectionType;

    /**
     * Class holding information about an attribute.
     */
    class DCAttributeInfo
    {
        H5AttributeId attr_;

   public:
        DCAttributeInfo(hid_t attr);
        ~DCAttributeInfo();

        /** Return the size of the required memory in bytes when querying the value */
        size_t getMemSize();
        /** Return the CollectionType. Might be ColTypeUnknown */
        const CollectionType& getType();
        /** Return the size in each dimension */
        Dimensions getDims();
        /** Return the number of dimensions */
        uint32_t getNDims();
        /** Return whether this is a scalar value */
        bool isScalar() { return getNDims() == 1 && getDims().getScalarSize() == 1; }
        /** Return true, if the attribute has a variable size in which case reading
         *  it will only return its pointer */
        bool isVarSize();

    private:
        // Don't copy
        DCAttributeInfo(const DCAttributeInfo&);
        DCAttributeInfo& operator=(const DCAttributeInfo&);
        void loadType();
        void loadSpace();

        // Those values are lazy loaded on access
        CollectionType* colType_;
        H5TypeId type_;
        H5DataspaceId space_;
        uint32_t nDims_;
};
}

#endif /* DCATTRIBUTE_INFO_H */
