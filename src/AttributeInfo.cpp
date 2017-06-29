/*
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

#include "splash/AttributeInfo.hpp"
#include "splash/CollectionType.hpp"
#include "splash/basetypes/generateCollectionType.hpp"
#include <cassert>
#include <sstream>

namespace splash
{

AttributeInfo::AttributeInfo(hid_t attr): attr_(attr), colType_(NULL)
{
}

AttributeInfo::AttributeInfo(H5AttributeId& attr): attr_(attr.release()), colType_(NULL)
{
}

AttributeInfo::~AttributeInfo()
{
    delete colType_;
}

AttributeInfo::AttributeInfo(const AttributeInfo& other):  attr_(other.attr_), colType_(NULL)
{
}

AttributeInfo& AttributeInfo::operator=(const AttributeInfo& other)
{
    if(&other == this)
        return *this;
    attr_ = other.attr_;
    // Reset lazy loaded values!
    delete colType_;
    colType_ = NULL;
    type_.release();
    space_.release();
    return *this;
}

std::string AttributeInfo::getExceptionString(const std::string& msg)
{
    return (std::string("Exception for DCAttribute [") + readName() +
            std::string("] ") + msg);
}

inline void AttributeInfo::loadType() throw(DCException)
{
    if(!type_)
    {
        type_.reset(H5Aget_type(attr_));
        if (!type_)
            throw DCException(getExceptionString("Could not get type"));
    }
}

inline void AttributeInfo::loadSpace() throw(DCException)
{
    if(!space_)
    {
        space_.reset(H5Aget_space(attr_));
        if (!space_)
            throw DCException(getExceptionString("Could not get dataspace"));
        // Currently only simple dataspaces exist and are supported by this library
        if (H5Sis_simple(space_) <= 0)
            throw DCException(getExceptionString("Dataspace is not simple"));
        H5S_class_t spaceClass = H5Sget_simple_extent_type(space_);
        if (spaceClass == H5S_SCALAR)
            nDims_ = 0;
        else
        {
            int nDims = H5Sget_simple_extent_ndims(space_);
            if (nDims < 0)
                throw DCException(getExceptionString("Could not get dimensionality of dataspace"));
            nDims_ = static_cast<uint32_t>(nDims);
        }
    }
}

std::string AttributeInfo::readName()
{
    ssize_t nameLen = H5Aget_name(attr_, 0, NULL);
    if(nameLen <= 0)
        return std::string();
    char* name = new char[nameLen + 1];
    // Read and in error case (unlikely) just store a zero-len string
    if(H5Aget_name(attr_, nameLen + 1, name) < 0)
        name[0] = 0;
    std::string strName(name);
    delete[] name;
    return strName;
}

size_t AttributeInfo::getMemSize() throw(DCException)
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

const CollectionType& AttributeInfo::getType() throw(DCException)
{
    if(!colType_)
    {
        loadType();
        colType_ = generateCollectionType(type_);
    }
    return *colType_;
}

Dimensions AttributeInfo::getDims() throw(DCException)
{
    loadSpace();
    Dimensions dims(1, 1, 1);
    // For scalars we don't need more info on the dataspace
    if (nDims_ > 0)
    {
        if (nDims_ > DSP_DIM_MAX)
            throw DCException(getExceptionString("Dimensionality of dataspace is greater than the maximum supported value"));
        int nDims2 = H5Sget_simple_extent_dims(space_, dims.getPointer(), NULL);
        // Conversion to int is save due to limited range
        if (nDims2 != static_cast<int>(nDims_))
            throw DCException(getExceptionString("Could not get dimensions of dataspace"));
        dims.swapDims(nDims_);
    }
    return dims;
}

uint32_t AttributeInfo::getNDims() throw(DCException)
{
    loadSpace();
    // A value of 0 means a scalar, treat as 1D
    return nDims_ == 0 ? 1 : nDims_;
}

bool AttributeInfo::isVarSize() throw(DCException)
{
    loadType();
    return H5Tis_variable_str(type_);
}

void AttributeInfo::read(const CollectionType& colType, void* buf) throw(DCException)
{
    if(!readNoThrow(colType, buf))
        throw DCException(getExceptionString("Could not read or convert data"));
}

bool AttributeInfo::readNoThrow(const CollectionType& colType, void* buf)
{
    return H5Aread(attr_, colType.getDataType(), buf) >= 0;
}

void AttributeInfo::read(void* buf, size_t bufSize) throw(DCException)
{
    loadType();
    if(getMemSize() != bufSize)
    {
        std::stringstream ss;
        ss << "Buffer size (" << bufSize << ") does not match attribute size (" << getMemSize() << ")";
        throw DCException(getExceptionString(ss.str()));
    }
    if(H5Aread(attr_, type_, buf) < 0)
        throw DCException(getExceptionString("Could not read data"));
}

void AttributeInfo::close()
{
    AttributeInfo tmp(-1);
    swap(*this, tmp);
}

void swap(AttributeInfo& lhs, AttributeInfo& rhs)
{
    using std::swap;
    swap(lhs.attr_, rhs.attr_);
    swap(lhs.colType_, rhs.colType_);
    swap(lhs.type_, rhs.type_);
    swap(lhs.space_, rhs.space_);
    swap(lhs.nDims_, rhs.nDims_);
}

} // namespace splash
