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



#ifndef DCGROUP_HPP
#define	DCGROUP_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <hdf5.h>

#include "splash/DataCollector.hpp"
#include "splash/DCException.hpp"
#include "splash/core/HandleMgr.hpp"

namespace splash
{

    /**
     * \cond HIDDEN_SYMBOLS
     */
    class DCGroup
    {
    private:
        typedef std::vector<H5Handle> HandlesList;

    public:

        typedef struct
        {
            DataCollector::DCEntry *entries;
            size_t count;
        } VisitObjCBType;

        DCGroup();
        virtual ~DCGroup();

        H5Handle open(H5Handle base, std::string path) throw (DCException);
        H5Handle create(H5Handle base, std::string path) throw (DCException);
        H5Handle openCreate(H5Handle base, std::string path) throw (DCException);
        void close() throw (DCException);

        static bool exists(H5Handle base, std::string path);
        static void remove(H5Handle base, std::string path) throw (DCException);

        H5Handle getHandle();

        static void getEntriesInternal(H5Handle base, const std::string baseGroup,
                std::string baseName, VisitObjCBType *param) throw (DCException);

    protected:
        bool checkExistence;

    private:
        HandlesList handles;

        static std::string getExceptionString(std::string msg, std::string name);
    };
    /**
     * \endcond
     */

}

#endif	/* DCGROUP_HPP */

