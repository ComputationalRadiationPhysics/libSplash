/**
 * Copyright 2013, 2015 Felix Schmitt, René Widera, Axel Huebl,
 *                      Carlchristian Eckert
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


#ifndef COLTYPEBOOL_H
#define	COLTYPEBOOL_H

#include "splash/CollectionType.hpp"

#include <string>
#include <cstring>

namespace splash
{

class ColTypeBool : public CollectionType
{
public:

    ColTypeBool()
    {
        // 8 bit (very) short int, see
        //   http://www.hdfgroup.org/HDF5/doc/RM/PredefDTypes.html
        // this is a h5py compatible implementation for bool, see:
        //   http://docs.h5py.org/en/latest/faq.html
        this->type = H5Tenum_create(H5T_NATIVE_INT8);
        const char *names[2] = {"true", "false"};
        const int64_t val[2] = {1, 0};
        H5Tenum_insert(this->type, names[0], &val[0]);
        H5Tenum_insert(this->type, names[1], &val[1]);
    }

    ~ColTypeBool()
    {
        H5Tclose(this->type);
    }

    size_t getSize() const
    {
        return sizeof (bool);
    }

    static CollectionType* genType(hid_t datatype_id)
    {
        bool found = false;
        H5T_class_t h5_class = H5Tget_class(datatype_id);
        if(h5_class == H5T_ENUM)
        {
            if(H5Tget_nmembers(datatype_id) == 2)
            {
                char* m0 = H5Tget_member_name(datatype_id,0);
                char* m1 = H5Tget_member_name(datatype_id,1);
                if(strcmp("true" , m0) == 0 && strcmp("false", m1) == 0)
                    found = true;
                free(m1);
                free(m0);
            }
        }

        if(found)
            return new ColTypeBool;
        else
            return NULL;
    }

    std::string toString() const
    {
        return "Bool";
    }
};

}

#endif	/* COLTYPEBOOL_H */

