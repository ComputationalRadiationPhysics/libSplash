/**
 * Copyright 2015 Carlchristian Eckert
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

#ifndef GENERATE_COLLECTION_TYPE_H
#define GENERATE_COLLECTION_TYPE_H


#include "splash/basetypes/basetypes.hpp"
#include "splash/CollectionType.hpp"

namespace splash
{

/**
 * Checks if a datatype_id (from an outer scope) can be used to generate a
 * specific CollectionType derivate.
 *
 * If the call to the genType member function of a specific collection type
 * is successful, it will return the pointer to an instance of this type.
 * Otherwise, the pointer (which points to NULL) will be destroyed at the end
 * of the local scope.
 *
 * @param _name the suffix of the ColType. i.e. to check for ColTypeBool,
 * _name should be set to Bool.
 */
#define TRY_COLTYPE(_name)                                                     \
{                                                                              \
    CollectionType* t = ColType##_name::genType(datatype_id);                  \
    if(t != NULL && typeid(*t) == typeid(ColType##_name)) return t;            \
}                                                                              \


/**
 * Creates a new instance of a CollectionType based on the given datatype_id
 *
 * @param datatype_id the H5 datatype_id that should be converted into a
 *                    CollectionType
 *
 * @return A pointer to a heap-allocated CollectionType, if a matching
 *         CollectionType could be determined. The allocated object must be
 *         freed by the caller at the end of its lifetime.
 *         If no matching CollectionType was found, returns NULL.
 */
CollectionType* generateCollectionType(hid_t datatype_id)
{
    // basetypes atomic
    TRY_COLTYPE(Int8)
    TRY_COLTYPE(Int16)
    TRY_COLTYPE(Int32)
    TRY_COLTYPE(Int64)

    TRY_COLTYPE(UInt8)
    TRY_COLTYPE(UInt16)
    TRY_COLTYPE(UInt32)
    TRY_COLTYPE(UInt64)

    TRY_COLTYPE(Float)
    TRY_COLTYPE(Double)
    TRY_COLTYPE(Char)
    TRY_COLTYPE(Int)


    // ColType Bool -> must be before the other enum types!
    TRY_COLTYPE(Bool)


    // ColType String
    TRY_COLTYPE(String)


    // ColTypeArray()
    TRY_COLTYPE(Float2Array)
    TRY_COLTYPE(Float3Array)
    TRY_COLTYPE(Float4Array)

    TRY_COLTYPE(Double2Array)
    TRY_COLTYPE(Double3Array)
    TRY_COLTYPE(Double4Array)

    TRY_COLTYPE(Int4Array)
    TRY_COLTYPE(Int3Array)
    TRY_COLTYPE(Int2Array)


    // ColTypeDimArray
    TRY_COLTYPE(DimArray)


    // ColType Dim
    TRY_COLTYPE(Dim)


    // Coltype Compound
    TRY_COLTYPE(Float2)
    TRY_COLTYPE(Float3)
    TRY_COLTYPE(Float4)

    TRY_COLTYPE(Double2)
    TRY_COLTYPE(Double3)
    TRY_COLTYPE(Double4)

    TRY_COLTYPE(Int2)
    TRY_COLTYPE(Int3)
    TRY_COLTYPE(Int4)

    return new ColTypeUnknown;
}


} /* namespace splash */


#endif /* GENERATE_COLLECTION_TYPE_H */
