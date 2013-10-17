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
 


#ifndef DCATTRIBUTE_H
#define	DCATTRIBUTE_H

#include <hdf5.h>

#include "DCException.hpp"

namespace splash
{
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
    private:
        DCAttribute();
        
        static std::string getExceptionString(const char *name, std::string msg);
    };
    /**
     * \endcond
     */

}

#endif	/* DCATTRIBUTE_H */

