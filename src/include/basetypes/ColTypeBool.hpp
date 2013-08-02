/**
 * Copyright 2013 Felix Schmitt, René Widera
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
 


#ifndef COLTYPEBOOL_H
#define	COLTYPEBOOL_H

#include "CollectionType.hpp"

namespace DCollector
{

class ColTypeBool : public CollectionType
{
public:

    ColTypeBool()
    {
        const hsize_t dim[] = {sizeof (bool)};
        this->type = H5Tarray_create(H5T_NATIVE_B8, 1, dim);
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

