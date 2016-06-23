/**
 * Copyright 2013, 2015 Felix Schmitt, Axel Huebl
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

#ifndef DCATTRIBUTE_H
#define DCATTRIBUTE_H

#include <hdf5.h>

#include "splash/DCException.hpp"
#include "splash/Dimensions.hpp"

namespace splash
{
    class DCAttributeInfo;

    /**
     * Class for static convenience attribute operations.
     * \cond HIDDEN_SYMBOLS
     */
    class DCAttribute
    {
    public:
        /**
         * Basic static method for reading an attribute.
         *
         * @param name name of the attribute
         * @param parent parent object to get attribute from
         * @param dst buffer to hold read value
         */
        static void readAttribute(const char *name,
                hid_t parent,
                void *dst) throw (DCException);

        /**
         * Static method for reading the attribute meta info
         *
         * @param name name of the attribute
         * @param parent parent object to get attribute from
         * @return Pointer to heap allocated DCAttributeInfo instance
         *         or NULL if not found.
         *         Must be freed by the caller.
         *
         */
        static DCAttributeInfo* readAttributeInfo(const char *name,
                hid_t parent) throw (DCException);

        /**
         * Basic static method for writing an attribute.
         *
         * @param name name of the attribute
         * @param type datatype information of the attribute
         * @param parent parent object to add attribute to
         * @param src buffer with value
         */
        static void writeAttribute(const char *name,
                const hid_t type,
                hid_t parent,
                const void *src) throw (DCException);


        /**
         * Basic static method for writing an attribute.
         *
         * @param name name of the attribute
         * @param type datatype information of the attribute
         * @param parent parent object to add attribute to
         * @param ndims Number of dimensions (1-3)
         * @param dims Number of elements
         * @param src buffer with value
         */
        static void writeAttribute(const char *name,
                const hid_t type,
                hid_t parent,
                uint32_t ndims,
                const Dimensions dims,
                const void *src) throw (DCException);

    private:
        DCAttribute();

        static std::string getExceptionString(const char *name, std::string msg);
    };
    /**
     * \endcond
     */

}

#endif /* DCATTRIBUTE_H */
