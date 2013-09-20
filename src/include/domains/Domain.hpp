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
         * Constructor.
         */
        Domain(void) :
        start(0, 0, 0),
        size(1, 1, 1)
        {

        }

        /**
         * Constructor.
         * 
         * @param start start/offset of this domain in the parent domain
         * @param size extent of this domain in every dimension
         */
        Domain(Dimensions start, Dimensions size) :
        start(start),
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
         * Returns the extent of this domain in every dimension.
         * 
         * @return extent of this domain
         */
        Dimensions &getSize()
        {
            return size;
        }

        const Dimensions getSize() const
        {
            return size;
        }

        /**
         * Returns the start/offset of this domain in the parent domain.
         * 
         * @return start of this domain
         */
        Dimensions &getStart()
        {
            return start;
        }

        const Dimensions getStart() const
        {
            return start;
        }

        /**
         * Returns last element of this domain.
         * 
         * @return last element
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

        
        std::string toString()
        {
            std::stringstream stream;
            stream << "(start: " << start.toString() << ", size: " << size.toString() << ")";
            return stream.str();
        }

    protected:
        Dimensions start;
        Dimensions size;
    };

}

#endif	/* DOMAIN_HPP */

