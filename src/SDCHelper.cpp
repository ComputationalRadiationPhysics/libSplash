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

#include "splash/core/SDCHelper.hpp"

#include "splash/version.hpp"
#include "splash/basetypes/basetypes.hpp"
#include "splash/core/DCAttribute.hpp"
#include "splash/core/logging.hpp"

#include <sstream>

namespace splash
{

    std::string SDCHelper::getExceptionString(std::string msg)
    {
        return (std::string("Exception for [SDCHelper] ") + msg);
    }

    void SDCHelper::getReferenceData(const char* filename, int32_t* maxID, Dimensions *mpiSize)
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

    void SDCHelper::writeHeader(hid_t file, Dimensions mpiPosition,
            int32_t *maxID, bool *enableCompression, Dimensions *mpiSize,
            bool master)
    throw (DCException)
    {
        // create group for header information (position, grid size, ...)
        hid_t group_header = H5Gcreate(file, SDC_GROUP_HEADER, H5P_LINK_CREATE_DEFAULT,
                H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
        if (group_header < 0)
            throw DCException(getExceptionString("Failed to create header group in reference file"));

        try {
            std::stringstream splashVersion;
            splashVersion << SPLASH_VERSION_MAJOR << "."
                          << SPLASH_VERSION_MINOR << "."
                          << SPLASH_VERSION_PATCH;
            std::stringstream splashFormat;
            splashFormat << SPLASH_FILE_FORMAT_MAJOR << "."
                         << SPLASH_FILE_FORMAT_MINOR;

            ColTypeInt32 ctInt32;
            ColTypeBool ctBool;
            ColTypeDim dim_t;
            ColTypeString ctStringVersion(splashVersion.str().length());
            ColTypeString ctStringFormat(splashFormat.str().length());

            // create master specific header attributes
            if (master) {
                DCAttribute::writeAttribute(SDC_ATTR_MAX_ID, ctInt32.getDataType(),
                        group_header, maxID);
            } else {
                DCAttribute::writeAttribute(SDC_ATTR_MPI_POSITION, dim_t.getDataType(),
                        group_header, mpiPosition.getPointer());
            }

            DCAttribute::writeAttribute(SDC_ATTR_COMPRESSION, ctBool.getDataType(),
                    group_header, enableCompression);

            DCAttribute::writeAttribute(SDC_ATTR_MPI_SIZE, dim_t.getDataType(),
                    group_header, mpiSize->getPointer());

            DCAttribute::writeAttribute(SDC_ATTR_VERSION, ctStringVersion.getDataType(),
                    group_header, splashVersion.str().c_str());

            DCAttribute::writeAttribute(SDC_ATTR_FORMAT, ctStringFormat.getDataType(),
                    group_header, splashFormat.str().c_str());

        } catch (DCException attr_error) {
            throw DCException(getExceptionString(
                std::string("Failed to write header attribute in reference file. Error was: ") +
                std::string(attr_error.what())));
        }

        H5Gclose(group_header);
    }

} /* namespace splash */
