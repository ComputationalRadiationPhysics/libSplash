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
 


#ifndef SDCHELPER_H
#define	SDCHELPER_H

#include "basetypes/ColTypeDim.hpp"
#include "sdc_defines.hpp"
#include "core/DCAttribute.hpp"
#include "core/logging.hpp"
#include "DCException.hpp"

namespace DCollector
{

    /**
     * \cond HIDDEN_SYMBOLS
     */
    class SDCHelper
    {
    private:
        SDCHelper();

        static std::string getExceptionString(std::string msg)
        {
            return (std::string("Exception for [SDCHelper] ") + msg);
        }

    public:

        /**
         * Reads reference data (header information) from file.
         *
         * @param filename the filename to read from
         * @param maxID pointer to hold max iteration. can be NULL
         * @param mpiSize pointer to hold size of mpi grid. can be NULL
         */
        static void getReferenceData(const char* filename, int32_t* maxID, Dimensions *mpiSize)
        throw (DCException)
        {
            log_msg(1, "loading reference data from %s", filename);

            // open the file to get reference data from
            hid_t reference_file = H5Fopen(filename, H5F_ACC_RDONLY, H5P_FILE_ACCESS_DEFAULT);
            if (reference_file < 0)
                throw DCException(getExceptionString(std::string("Failed to open reference file ") +
                    std::string(filename)));

            // reference data is located in the header only
            hid_t group_header = H5Gopen(reference_file, SDC_GROUP_HEADER, H5P_DEFAULT);
            if (group_header < 0) {
                H5Fclose(reference_file);
                throw DCException(getExceptionString(
                        std::string("Failed to open header group in reference file ") +
                        std::string(filename)));
            }

            try {
                if (maxID != NULL) {
                    DCAttribute::readAttribute(SDC_ATTR_MAX_ID,
                            group_header, maxID);
                }

                if (mpiSize != NULL) {
                    ColTypeDim dim_t;
                    DCAttribute::readAttribute(SDC_ATTR_MPI_SIZE,
                            group_header, mpiSize->getPointer());
                }

            } catch (DCException attr_exception) {
                H5Fclose(reference_file);
                throw DCException(getExceptionString(
                        std::string("Failed to read attributes from reference file ") +
                        std::string(filename)));
            }

            // cleanup
            H5Gclose(group_header);
            H5Fclose(reference_file);
        }

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
        throw (DCException)
        {
            // create group for header information (position, grid size, ...)
            hid_t group_header = H5Gcreate(file, SDC_GROUP_HEADER, H5P_LINK_CREATE_DEFAULT,
                    H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
            if (group_header < 0)
                throw DCException(getExceptionString("Failed to create header group in reference file"));

            try {
                ColTypeDim dim_t;

                // create master specific header attributes
                if (master) {
                    DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, H5T_NATIVE_INT32,
                            group_header, maxID);
                } else {
                    DCAttribute::writeAttribute(SDC_ATTR_MPI_POSITION, dim_t.getDataType(),
                            group_header, mpiPosition.getPointer());
                }

                DCAttribute::writeAttribute(SDC_ATTR_COMPRESSION, H5T_NATIVE_HBOOL,
                        group_header, enableCompression);

                DCAttribute::writeAttribute(SDC_ATTR_MPI_SIZE, dim_t.getDataType(),
                        group_header, mpiSize->getPointer());

            } catch (DCException attr_error) {
                throw DCException(getExceptionString(
                        std::string("Failed to write header attribute in reference file. Error was: ") +
                        std::string(attr_error.what())));
            }

            H5Gclose(group_header);
        }
    };
}

#endif	/* SDCHELPER_H */

