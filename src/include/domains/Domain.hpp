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

namespace DCollector
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
        start(0, 0, 0),
        size(1, 1, 1),
        rank(3)
        {

        }

        /**
         * Constructor.
         * 
         * @param rank Number of dimensions (1-3).
         */
        Domain(uint32_t rank) :
        start(0, 0, 0),
        size(1, 1, 1),
        rank(rank)
        {

        }

        /**
         * Constructor.
         * 
         * @param rank Number of dimensions (1-3).
         * @param start Start/offset of this domain in the parent domain.
         * @param size Size of this domain in every dimension.
         */
        Domain(uint32_t rank, Dimensions start, Dimensions size) :
        start(start),
        size(size),
        rank(rank)
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
        uint32_t getRank() const
        {
            return rank;
        }

        /**
         * Sets the number of dimensions of this domain.
         * 
         * @param rank New number of dimensions (1-3).
         */
        void setRank(uint32_t rank)
        {
            this->rank = rank;
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
         * Returns the start/offset of this domain in the parent domain.
         * 
         * @return Start of this domain.
         */
        Dimensions &getStart()
        {
            return start;
        }

        /**
         * Returns the start/offset of this domain in the parent domain.
         * 
         * @return Start of this domain.
         */
        const Dimensions getStart() const
        {
            return start;
        }

        /**
         * Returns last element of this domain.
         * 
         * @return Last element.
         */
        Dimensions getEnd() const
        {
            return start + size - Dimensions(1, 1, 1);
        }

        bool operator==(Domain const& other) const
        {
            return start == other.getStart() && size == other.getSize();
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
            stream << "(start: " << start.toString() << ", size: " << size.toString() << ")";
            return stream.str();
        }

    protected:
        Dimensions start;
        Dimensions size;
        uint32_t rank;
    };

}

#endif	/* DOMAIN_HPP */

