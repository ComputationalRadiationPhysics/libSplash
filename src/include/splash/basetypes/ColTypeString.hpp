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
     * Since C-strings are NULL-terminated, len must be +1
     * of the actual length of the string for fixed length strings.
     *
     * \see http://hdfgroup.org/HDF5/doc/RM/RM_H5T.html#CreateVLString
     */
    class ColTypeString : public CollectionType
    {
    public:

        ColTypeString() :
          mySize(H5T_VARIABLE)
        {
            this->type = H5Tcopy(H5T_C_S1);
            H5Tset_size(this->type, H5T_VARIABLE);
        }

        ColTypeString(size_t len) :
          mySize(len)
        {
            this->type = H5Tcopy(H5T_C_S1);
            H5Tset_size(this->type, len);
        }

        ~ColTypeString()
        {
            H5Tclose(this->type);
        }

        size_t getSize() const
        {
            // for H5T_VARIABLE irrelevant ?
            if( mySize != H5T_VARIABLE )
               return sizeof(char) * mySize; //H5Tget_size(this->type);
            else
               return sizeof(char);
        }

    private:
        size_t mySize;
    };

}

#endif	/* COLTYPESTRING_H */
