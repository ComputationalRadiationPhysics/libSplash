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

#include "splash/DCAttributeInfo.hpp"
#include "splash/CollectionType.hpp"
#include "splash/DCException.hpp"
#include "splash/basetypes/generateCollectionType.hpp"
#include <cassert>

namespace splash
{

DCAttributeInfo::DCAttributeInfo(hid_t attr): attr_(attr), colType_(NULL)
{
}

DCAttributeInfo::~DCAttributeInfo()
{
    delete colType_;
}

inline void DCAttributeInfo::loadType()
{
    if(!type_)
    {
        type_.reset(H5Aget_type(attr_));
        if (!type_)
            throw DCException("Could not get type of attribute");
    }
}

inline void DCAttributeInfo::loadSpace()
{
    if(!space_)
    {
        space_.reset(H5Aget_space(attr_));
        if (!space_)
            throw DCException("Could not get dataspace of attribute");
        // Currently only simple dataspaces exist and are supported by this library
        if (H5Sis_simple(space_) <= 0)
            throw DCException("Dataspace is not simple");
        H5S_class_t spaceClass = H5Sget_simple_extent_type(space_);
        if (spaceClass == H5S_SCALAR)
            nDims_ = 0;
        else
        {
            int nDims = H5Sget_simple_extent_ndims(space_);
            if (nDims < 0)
                throw DCException("Could not get dimensionality of dataspace");
            nDims_ = static_cast<uint32_t>(nDims);
        }
    }
}

size_t DCAttributeInfo::getMemSize()
{
    loadType();
    // For variable length strings the storage size is always the size of 2 pointers
    // So we need the types size for that
    if(H5Tis_variable_str(type_))
        return H5Tget_size(type_);
    else
    {
        size_t result = H5Aget_storage_size(attr_);
        // This should be the same, but the above does not require getting the dims
        assert(result == H5Tget_size(type_) * getDims().getScalarSize());
        return result;
    }
}

const CollectionType& DCAttributeInfo::getType()
{
    if(!colType_)
    {
        loadType();
        colType_ = generateCollectionType(type_);
    }
    return *colType_;
}

Dimensions DCAttributeInfo::getDims()
{
    loadSpace();
    Dimensions dims(1, 1, 1);
    // For scalars we don't need more info on the dataspace
    if (nDims_ > 0)
    {
        if (nDims_ > DSP_DIM_MAX)
            throw DCException("Dimensionality of dataspace is greater than the maximum supported value");
        int nDims2 = H5Sget_simple_extent_dims(space_, dims.getPointer(), NULL);
        // Conversion to int is save due to limited range
        if (nDims2 != static_cast<int>(nDims_))
            throw DCException("Could not get dimensions of dataspace");
    }
    return dims;
}

uint32_t DCAttributeInfo::getNDims()
{
    loadSpace();
    // A value of 0 means a scalar, treat as 1D
    return nDims_ == 0 ? 1 : nDims_;
}

bool DCAttributeInfo::isVarSize()
{
    loadType();
    return H5Tis_variable_str(type_);
}

}
