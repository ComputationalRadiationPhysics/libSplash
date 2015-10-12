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

#ifndef COLTYPEGENERATOR_H
#define COLTYPEGENERATOR_H


#include "splash/basetypes/basetypes.hpp"
#include "splash/CollectionType.hpp"

namespace splash
{

#define COLLECTION_TEST(_name)                                                 \
{                                                                              \
    CollectionType* t = ColType##_name::genType(datatype_id);                  \
    if(t != NULL && typeid(*t) == typeid(ColType##_name)) return t;            \
}                                                                              \


class CollectionTypeGenerator{
public:

    static CollectionType* genCollectionType(hid_t datatype_id)
    {
        // basetypes atomic
        COLLECTION_TEST(Int8)
        COLLECTION_TEST(Int16)
        COLLECTION_TEST(Int32)
        COLLECTION_TEST(Int64)

        COLLECTION_TEST(UInt8)
        COLLECTION_TEST(UInt16)
        COLLECTION_TEST(UInt32)
        COLLECTION_TEST(UInt64)

        COLLECTION_TEST(Float)
        COLLECTION_TEST(Double)
        COLLECTION_TEST(Char)
        COLLECTION_TEST(Int)


        // ColType Bool -> must be before the other enum types!
        COLLECTION_TEST(Bool)


        // ColType String
        COLLECTION_TEST(String)


        // ColTypeArray()
        COLLECTION_TEST(Float2Array)
        COLLECTION_TEST(Float3Array)
        COLLECTION_TEST(Float4Array)

        COLLECTION_TEST(Double2Array)
        COLLECTION_TEST(Double3Array)
        COLLECTION_TEST(Double4Array)

        COLLECTION_TEST(Int4Array)
        COLLECTION_TEST(Int3Array)
        COLLECTION_TEST(Int2Array)


        // ColTypeDimArray
        COLLECTION_TEST(DimArray)


        // ColType Dim
        COLLECTION_TEST(Dim)


        // Coltype Compound
        COLLECTION_TEST(Float2)
        COLLECTION_TEST(Float3)
        COLLECTION_TEST(Float4)

        COLLECTION_TEST(Double2)
        COLLECTION_TEST(Double3)
        COLLECTION_TEST(Double4)

        COLLECTION_TEST(Int2)
        COLLECTION_TEST(Int3)
        COLLECTION_TEST(Int4)

        return new ColTypeUnknown;
    }

};

}


#endif /* COLTYPEGENERATOR_H */
