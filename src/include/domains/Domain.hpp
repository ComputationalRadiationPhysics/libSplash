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



#ifndef DOMAIN_HPP
#define	DOMAIN_HPP

#include <string>
#include <sstream>
#include <stdint.h>

#include "Dimensions.hpp"

namespace splash
{

    /**
     * Represents a (1-3)-dimensional logical domain or subdomain,
     * e.g. a logical grid in a simulation code.
     */
    class Domain
    {
    public:

        /**
         * Default constructor.
         */
        Domain() :
        offset(0, 0, 0),
        size(1, 1, 1)
        {

        }

        /**
         * Constructor.
         * 
         * @param offset Offset of this domain in the parent domain.
         * @param size Size of this domain in every dimension.
         */
        Domain(Dimensions offset, Dimensions size) :
        offset(offset),
        size(size)
        {

        }

        /**
         * Destructor.
         */
        virtual ~Domain()
        {

        }

        /**
         * Returns the number of dimensions of this domain.
         * 
         * @return Number of dimensions (1-3).
         */
        uint32_t getDims() const
        {
            return size.getDims();
        }

        /**
         * Returns the size of this domain in every dimension.
         * 
         * @return Size of this domain.
         */
        Dimensions &getSize()
        {
            return size;
        }

        /**
         * Returns the size of this domain in every dimension.
         * 
         * @return Size of this domain.
         */
        const Dimensions getSize() const
        {
            return size;
        }

        /**
         * Returns the offset/offset of this domain in the parent domain.
         * 
         * @return Offset of this domain.
         */
        Dimensions &getOffset()
        {
            return offset;
        }

        /**
         * Returns the offset/offset of this domain in the parent domain.
         * 
         * @return Offset of this domain.
         */
        const Dimensions getOffset() const
        {
            return offset;
        }

        /**
         * Returns the last index of this domain which is the combination
         * of its offset and size - 1
         *
         * @return Last index of domain.
         */
        Dimensions getBack() const
        {
            return offset + size - Dimensions(1, 1, 1);
        }

        bool operator==(Domain const& other) const
        {
            return offset == other.getOffset() && size == other.getSize();
        }

        bool operator!=(Domain const& other) const
        {
            return !(*this == other);
        }

        /**
         * Returns a string representation.
         * 
         * @return String representation.
         */
        std::string toString() const
        {
            std::stringstream stream;
            stream << "(offset: " << offset.toString() << ", size: " << size.toString() << ")";
            return stream.str();
        }

        /**
         * Tests if two domains intersect.
         * 
         * @param d1 First domain.
         * @param d2 Decond domain.
         * @return True if domains overlap, false otherwise.
         */
        static bool testIntersection(const Domain& d1, const Domain& d2)
        {
            Dimensions d1_offset = d1.getOffset();
            Dimensions d2_offset = d2.getOffset();
            Dimensions d1_end = d1.getBack();
            Dimensions d2_end = d2.getBack();

            return (d1_offset[0] <= d2_end[0] && d1_end[0] >= d2_offset[0] &&
                    d1_offset[1] <= d2_end[1] && d1_end[1] >= d2_offset[1] &&
                    d1_offset[2] <= d2_end[2] && d1_end[2] >= d2_offset[2]) ||
                    (d2_offset[0] <= d1_end[0] && d2_end[0] >= d1_offset[0] &&
                    d2_offset[1] <= d1_end[1] && d2_end[1] >= d1_offset[1] &&
                    d2_offset[2] <= d1_end[2] && d2_end[2] >= d1_offset[2]);
        }

    protected:
        Dimensions offset;
        Dimensions size;
    };

}

#endif	/* DOMAIN_HPP */

