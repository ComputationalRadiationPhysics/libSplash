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

#ifndef SDCHELPER_H
#define SDCHELPER_H

#include "splash/sdc_defines.hpp"
#include "splash/Dimensions.hpp"
#include "splash/DCException.hpp"

#include <string>

namespace splash
{

    /**
     * \cond HIDDEN_SYMBOLS
     */
    class SDCHelper
    {
    private:
        SDCHelper();

        static std::string getExceptionString(std::string msg);

    public:

        /**
         * Reads reference data (header information) from file.
         *
         * @param filename the filename to read from
         * @param maxID pointer to hold max iteration. can be NULL
         * @param mpiSize pointer to hold size of mpi grid. can be NULL
         */
        static void getReferenceData(const char* filename, int32_t* maxID, Dimensions *mpiSize)
        throw (DCException);

        /**
         * Writes header information to file.
         *
         * @param file file to write header information to
         * @param mpiPosition mpi position of file. is ignored for master == true
         * @param maxID pointer to maxID
         * @param enableCompression pointer to enableCompression
         * @param mpiSize pointer to mpiSize
         * @param master file is a master file. mpiPosition is ignored
         */
        static void writeHeader(hid_t file, Dimensions mpiPosition,
                int32_t *maxID, bool *enableCompression, Dimensions *mpiSize,
                bool master = false)
        throw (DCException);
    };
}

#endif	/* SDCHELPER_H */
