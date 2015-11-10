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

#ifndef COLTYPEUNKNOWN_H
#define COLTYPEUNKNOWN_H

#include "splash/CollectionType.hpp"
#include <string>

namespace splash
{
    class ColTypeUnknown : public CollectionType
    {
    public:

        size_t getSize() const
        {
            return 0;
        }

        std::string toString() const
        {
            return "Unknown";
        }
    };
}

#endif	/* COLTYPEUNKNOWN_H */
