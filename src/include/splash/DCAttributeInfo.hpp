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

namespace splash
{
    class CollectionType;

    /**
     * Class holding information about an attribute.
     */
    class DCAttributeInfo
    {
        size_t memSize_;
        CollectionType* colType_;
        Dimensions dims_;
        uint32_t ndims_;

        friend class DCAttribute;
        // Don't copy
        DCAttributeInfo(const DCAttributeInfo&);
        DCAttributeInfo& operator=(const DCAttributeInfo&);
    public:
        DCAttributeInfo();
        ~DCAttributeInfo();

        /** Return the size of the required memory in bytes when querying the value */
        size_t getMemSize() const { return memSize_; }
        /** Return the CollectionType. Might be ColTypeUnknown */
        const CollectionType& getType() const { return *colType_; }
        /** Return the size in each dimension */
        const Dimensions& getDims() const { return dims_; }
        /** Return the number of dimensions */
        uint32_t getNDims() const { return ndims_; }
        /** Return whether this is a scalar value */
        bool isScalar() const { return ndims_ == 1 && dims_.getScalarSize() == 1; }
    };
}

#endif /* DCATTRIBUTE_INFO_H */
