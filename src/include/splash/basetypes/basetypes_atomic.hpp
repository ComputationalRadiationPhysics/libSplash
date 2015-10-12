/**
 * Copyright 2013-2015 Felix Schmitt, Axel Huebl, Carlchristian Eckert
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

#ifndef BASETYPES_ATOMIC_HPP
#define	BASETYPES_ATOMIC_HPP

#include <stdint.h>
#include <iostream>
#include <string>

#include "splash/CollectionType.hpp"
#include "splash/basetypes/ColTypeUnknown.hpp"

namespace splash
{

#define TYPE_ATOMIC(_name, _h5_type, _real_type)                               \
    class ColType##_name : public CollectionType                               \
    {                                                                          \
    public:                                                                    \
                                                                               \
        ColType##_name()                                                       \
        { type = _h5_type; }                                                   \
                                                                               \
        size_t getSize() const                                                 \
        { return sizeof (_real_type); }                                        \
                                                                               \
        static CollectionType* genType(hid_t datatype_id){                     \
            bool found = false;                                                \
            H5T_class_t h5_class = H5Tget_class(datatype_id);                  \
            if(h5_class == H5T_INTEGER || h5_class == H5T_FLOAT)               \
            {                                                                  \
                hid_t native = H5Tget_native_type(datatype_id,H5T_DIR_DEFAULT);\
                if(H5Tequal(native, _h5_type) == 1)                            \
                    found = true;                                              \
                H5Tclose(native);                                              \
            }                                                                  \
            if(found)                                                          \
                return new ColType##_name;                                     \
            else                                                               \
                return NULL;                                                   \
        }                                                                      \
                                                                               \
        std::string toString() const                                           \
        {                                                                      \
            return #_name;                                                     \
        }                                                                      \
                                                                               \
    };                                                                         \


TYPE_ATOMIC(Float, H5T_NATIVE_FLOAT, float);
TYPE_ATOMIC(Double, H5T_NATIVE_DOUBLE, double);

TYPE_ATOMIC(Int, H5T_NATIVE_INT, int);
TYPE_ATOMIC(Char, H5T_NATIVE_CHAR, char);

TYPE_ATOMIC(UInt8, H5T_INTEL_U8, uint8_t);
TYPE_ATOMIC(UInt16, H5T_INTEL_U16, uint16_t);
TYPE_ATOMIC(UInt32, H5T_INTEL_U32, uint32_t);
TYPE_ATOMIC(UInt64, H5T_INTEL_U64, uint64_t);

TYPE_ATOMIC(Int8, H5T_INTEL_I8, int8_t);
TYPE_ATOMIC(Int16, H5T_INTEL_I16, int16_t);
TYPE_ATOMIC(Int32, H5T_INTEL_I32, int32_t);
TYPE_ATOMIC(Int64, H5T_INTEL_I64, int64_t);

}

#endif	/* BASETYPES_ATOMIC_HPP */

