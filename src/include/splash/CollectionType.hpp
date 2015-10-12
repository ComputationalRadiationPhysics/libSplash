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
 


#ifndef COLLECTIONTYPE_H
#define	COLLECTIONTYPE_H

#include <hdf5.h>
#include <string>

#define H5DataType hid_t

namespace splash
{

    /**
     * Describes the datatype to be used for writing/reading HDF5 data to/from disk.
     */
    class CollectionType
    {
    public:

        /**
         * Returns a (implementation specific) reference to the datatype.
         *
         * @return the datatype
         */
        const H5DataType& getDataType() const
        {
            return type;
        }

        /**
         * Returns the size in bytes of the datatype.
         *
         * @return size of datatype in bytes
         */
        virtual size_t getSize() const = 0;

        /**
         * Returns a human-readable representation of the datatype.
         *
         * @return the name of the datatype as a string
         */
        virtual std::string toString() const = 0;

        /**
         * Destructor
         */
        virtual ~CollectionType()
        {
        };
    protected:
        H5DataType type;
    };

} // namespace DataCollector

#endif	/* COLLECTIONTYPE_H */

