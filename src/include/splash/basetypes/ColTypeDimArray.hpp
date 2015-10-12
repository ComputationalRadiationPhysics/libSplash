/**
 * Copyright 2013 Felix Schmitt
 *           2015 Carlchristian Eckert
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
 


#ifndef COLTYPEDIMARRAY_H
#define	COLTYPEDIMARRAY_H

#include "splash/CollectionType.hpp"

#include <string>

namespace splash
{
    class ColTypeDimArray : public CollectionType
    {
    public:

        ColTypeDimArray()
        {
            const hsize_t dim[] = {3};
            this->type = H5Tarray_create(H5T_NATIVE_HSIZE, 1, dim);
        }

        ~ColTypeDimArray()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            return sizeof (hsize_t) * 3;
        }

        static CollectionType* genType(hid_t datatype_id)
        {
            bool found = false;
            H5T_class_t h5_class = H5Tget_class(datatype_id);
            if(h5_class == H5T_ARRAY)
            {
                hid_t base = H5Tget_super(datatype_id);
                if(H5Tequal(base, H5T_NATIVE_HSIZE) == 1){
                    if(H5Tget_array_ndims(datatype_id) == 1)
                    {
                        hsize_t adims_out[1];
                        H5Tget_array_dims(datatype_id, adims_out);
                        if(adims_out[0] == 3)
                            found = true;
                    }
                }
                H5Tclose(base);
            }
            if(found)
                return new ColTypeDimArray;
            else
                return NULL;
        }

        std::string toString() const
        {
            return "DimArray";
        }
    };

}

#endif	/* COLTYPEDIMARRAY_H */

