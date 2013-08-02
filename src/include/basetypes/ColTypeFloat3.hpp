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
 


#ifndef COLTYPEFLOAT3_H
#define	COLTYPEFLOAT3_H

#include "CollectionType.hpp"

namespace DCollector
{

    class ColTypeFloat3 : public CollectionType
    {
    public:

        ColTypeFloat3()
        {
            this->type = H5Tcreate(H5T_COMPOUND, sizeof (float) * 3);
            H5Tinsert(this->type, "x", 0, H5T_NATIVE_FLOAT);
            H5Tinsert(this->type, "y", sizeof (float), H5T_NATIVE_FLOAT);
            H5Tinsert(this->type, "z", 2 * sizeof (float), H5T_NATIVE_FLOAT);
        }

        ~ColTypeFloat3()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            return sizeof (float) * 3;
        }
    };

}

#endif	/* COLTYPEFLOAT2_H */

