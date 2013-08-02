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
 


#include <H5public.h>
#include <sstream>

#include "DCAttribute.hpp"
#include "include/DCAttribute.hpp"

namespace DCollector
{

    DCAttribute::DCAttribute()
    {
    }

    std::string DCAttribute::getExceptionString(const char *name, std::string msg)
    {
        return (std::string("Exception for DCAttribute [") + std::string(name) +
                std::string("] ") + msg);
    }

    void DCAttribute::readAttribute(const char* name, hid_t& parent, void* dst)
    throw (DCException)
    {
        hid_t attr = H5Aopen(parent, name, H5P_DEFAULT);
        if (attr < 0)
            throw DCException(getExceptionString(name, "Attribute could not be opened for reading"));

        hid_t attr_type = H5Aget_type(attr);
        if (attr_type < 0)
        {
            H5Aclose(attr);
            throw DCException(getExceptionString(name, "Could not get type of attribute"));
        }
        
        if (H5Aread(attr, attr_type, dst) < 0)
        {
            H5Aclose(attr);
            throw DCException(getExceptionString(name, "Attribute could not be read"));
        }

        H5Aclose(attr);
    }

    void DCAttribute::writeAttribute(const char* name, const hid_t& type, hid_t& parent, const void* src)
    throw (DCException)
    {
        hid_t attr = -1;
        if (H5Aexists(parent, name))
            attr = H5Aopen(parent, name, H5P_DEFAULT);
        else
        {
            hid_t dsp = H5Screate(H5S_SCALAR);
            attr = H5Acreate(parent, name, type, dsp, H5P_DEFAULT, H5P_DEFAULT);
            H5Sclose(dsp);
        }

        if (attr < 0)
            throw DCException(getExceptionString(name, "Attribute could not be opened or created"));

        if (H5Awrite(attr, type, src) < 0)
        {
            H5Aclose(attr);
            throw DCException(getExceptionString(name, "Attribute could not be written"));
        }

        H5Aclose(attr);
    }

}