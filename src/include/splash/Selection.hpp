/**
 * Copyright 2014-2015 Felix Schmitt, Axel Huebl
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

#ifndef SELECTION_HPP
#define SELECTION_HPP

#include <string>
#include <sstream>

#include "Dimensions.hpp"

namespace splash
{

    /**
     * 1-3D dataset selection, similar to an HDF5 hyperslap.
     */
    class Selection
    {
    public:

        /**
         * Constructor
         *
         * @param size_ size of src buffer (select complete buffer)
         */
        Selection(Dimensions size_) :
        size(size_),
        count(size_),
        offset(0, 0, 0),
        stride(1, 1, 1)
        {

        }

        /**
         * Constructor
         *
         * @param size_ size of src buffer
         * @param count_ size of selection within src buffer
         * @param offset_ offset of selection within src buffer
         */
        Selection(Dimensions size_, Dimensions count_, Dimensions offset_) :
        size(size_),
        count(count_),
        offset(offset_),
        stride(1, 1, 1)
        {

        }

        /**
         * Constructor
         *
         * @param size_ size of src buffer
         * @param count_ size of selection within src buffer
         * @param offset_ offset of selection within src buffer
         * @param stride_ stride of selection within src buffer
         */
        Selection(Dimensions size_, Dimensions count_, Dimensions offset_, Dimensions stride_) :
        size(size_),
        count(count_),
        offset(offset_),
        stride(stride_)
        {

        }

        /**
         * Swap dimensions
         *
         * @param ndims number of dimensions of this selection
         */
        void swapDims(uint32_t ndims)
        {
            size.swapDims(ndims);
            count.swapDims(ndims);
            offset.swapDims(ndims);
            stride.swapDims(ndims);
        }

        /**
         * Create a string representation of this selection
         *
         * @return string representation
         */
        std::string toString(void) const
        {
            std::stringstream stream;
            stream <<
                    "{size=" << size.toString() <<
                    ", count=" << count.toString() <<
                    ", offset=" << offset.toString() <<
                    ", stride=" << stride.toString() << "}";
            return stream.str();
        }

        Dimensions size;
        Dimensions count;
        Dimensions offset;
        Dimensions stride;
    };

}

#endif /* SELECTION_HPP */
