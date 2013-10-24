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
 


#ifndef COLTYPEFLOAT2ARRAY_H
#define	COLTYPEFLOAT2ARRAY_H

#include "CollectionType.hpp"

namespace splash
{
    class ColTypeFloat2Array : public CollectionType
    {
    public:

        ColTypeFloat2Array()
        {
            const hsize_t dim[] = {2};
            this->type = H5Tarray_create(H5T_FLOAT, 1, dim);
        }

        ~ColTypeFloat2Array()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            return sizeof (float) * 2;
        }
    };

}

#endif	/* COLTYPEFLOAT2ARRAY_H */

