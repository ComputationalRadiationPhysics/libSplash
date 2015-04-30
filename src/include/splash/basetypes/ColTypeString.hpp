/**
 * Copyright 2013-2015 Felix Schmitt, Axel Huebl
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

#ifndef COLTYPESTRING_H
#define	COLTYPESTRING_H

#include "splash/CollectionType.hpp"

namespace splash
{
    /** Variable and fixed length strings
     *
     * Do not forget that C-strings are NULL-terminated.
     * As in <cstring>'s strlen() our length in the API
     * does not count the terminator, but c-strings still MUST
     * end with it, else HDF5 write/reads will fail.
     *
     * \see https://hdfgroup.org/HDF5/doc/RM/RM_H5T.html#Datatype-SetSize
     * \see http://hdfgroup.org/HDF5/doc/RM/RM_H5T.html#CreateVLString
     */
    class ColTypeString : public CollectionType
    {
    public:

        ColTypeString()
        {
            this->type = H5Tcopy(H5T_C_S1);
            H5Tset_size(this->type, H5T_VARIABLE);
        }

        ColTypeString(size_t len)
        {
            this->type = H5Tcopy(H5T_C_S1);
            /* HDF5 requires space for the \0-terminator character,
             * otherwise it will not be stored or retrieved */
            H5Tset_size(this->type, len + 1);
        }

        ~ColTypeString()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            size_t myElements = H5Tget_size(this->type);

            // for H5T_VARIABLE irrelevant ?
            if( myElements != H5T_VARIABLE )
               return sizeof(char) * myElements;
            else
               return sizeof(char);
        }
    };

}

#endif	/* COLTYPESTRING_H */
