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

#ifndef COLTYPEDIM_H
#define COLTYPEDIM_H

#include "splash/CollectionType.hpp"
#include "splash/Dimensions.hpp"

#include <cstdlib>
#include <string>
#include <cstring>

namespace splash
{

    class ColTypeDim : public CollectionType
    {
    public:

        ColTypeDim()
        {
            this->type = H5Tcreate(H5T_COMPOUND, getSize());
            H5Tinsert(this->type, "x", 0, H5T_NATIVE_HSIZE);
            H5Tinsert(this->type, "y", sizeof(hsize_t), H5T_NATIVE_HSIZE);
            H5Tinsert(this->type, "z", sizeof(hsize_t) * 2, H5T_NATIVE_HSIZE);
        }

        ~ColTypeDim()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            return getSize_();
        }

        std::string toString() const
        {
            return "Dim";
        }

        static CollectionType* genType(hid_t datatype_id)
        {
            bool found = false;
            H5T_class_t h5_class = H5Tget_class(datatype_id);
            if(h5_class == H5T_COMPOUND)
            {
                if(H5Tget_nmembers(datatype_id) == 3)
                {
                    if(H5Tget_size(datatype_id) == getSize_())
                    {
                        char* m0 = H5Tget_member_name(datatype_id, 0);
                        char* m1 = H5Tget_member_name(datatype_id, 1);
                        char* m2 = H5Tget_member_name(datatype_id, 2);
                        if(strcmp("x", m0) == 0 && strcmp("y", m1) == 0 && strcmp("z", m2) == 0)
                            found = true;

                        H5free_memory(m2);
                        H5free_memory(m1);
                        H5free_memory(m0);
                    }
                }
            }
            if(found)
                return new ColTypeDim;
            else
                return NULL;
        }
    
    private:
        static size_t getSize_()
        {
            return sizeof(hsize_t) * 3;
        }

   };
}

#endif /* COLTYPEDIM_H */
