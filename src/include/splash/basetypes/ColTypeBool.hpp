/**
 * Copyright 2013, 2015 Felix Schmitt, René Widera, Axel Huebl
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
};

}

#endif	/* COLTYPEBOOL_H */

