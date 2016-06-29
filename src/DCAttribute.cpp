/**
 * Copyright 2013-2016 Felix Schmitt, Axel Huebl, Alexander Grund
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

#include "splash/core/DCAttribute.hpp"
#include "splash/core/H5IdWrapper.hpp"
#include "splash/DCAttributeInfo.hpp"

#include <cassert>

namespace splash
{
    std::string DCAttribute::getExceptionString(const char *name, std::string msg)
    {
        return (std::string("Exception for DCAttribute [") + std::string(name) +
                std::string("] ") + msg);
    }

    DCAttributeInfo* DCAttribute::readAttributeInfo(const char* name, hid_t parent)
    throw (DCException)
    {
        hid_t attr = H5Aopen(parent, name, H5P_DEFAULT);
        if (!attr)
            throw DCException(getExceptionString(name, "Attribute could not be opened for reading"));
        return new DCAttributeInfo(attr);
    }

    void DCAttribute::readAttribute(const char* name, hid_t parent, void* dst)
    throw (DCException)
    {
        H5AttributeId attr(H5Aopen(parent, name, H5P_DEFAULT));
        if (!attr)
            throw DCException(getExceptionString(name, "Attribute could not be opened for reading"));

        H5TypeId attr_type(H5Aget_type(attr));
        if (!attr_type)
            throw DCException(getExceptionString(name, "Could not get type of attribute"));

        if (H5Aread(attr, attr_type, dst) < 0)
            throw DCException(getExceptionString(name, "Attribute could not be read"));
    }

    void DCAttribute::writeAttribute(const char* name, const hid_t type, hid_t parent,
                                     uint32_t ndims, Dimensions dims, const void* src)
    throw (DCException)
    {
        H5AttributeId attr;
        if (H5Aexists(parent, name))
            attr.reset(H5Aopen(parent, name, H5P_DEFAULT));
        else
        {
            dims.swapDims(ndims);
            H5DataspaceId dsp;
            if( ndims == 1 && dims.getScalarSize() == 1 )
                dsp.reset(H5Screate(H5S_SCALAR));
            else
                dsp.reset(H5Screate_simple( ndims, dims.getPointer(), dims.getPointer() ));

            attr.reset(H5Acreate(parent, name, type, dsp, H5P_DEFAULT, H5P_DEFAULT));
        }

        if (!attr)
            throw DCException(getExceptionString(name, "Attribute could not be opened or created"));

        if (H5Awrite(attr, type, src) < 0)
            throw DCException(getExceptionString(name, "Attribute could not be written"));
    }

    void DCAttribute::writeAttribute(const char* name, const hid_t type, hid_t parent, const void* src)
    throw (DCException)
    {
        const Dimensions dims(1, 1, 1);
        writeAttribute(name, type, parent, 1u, dims, src);
    }

}
