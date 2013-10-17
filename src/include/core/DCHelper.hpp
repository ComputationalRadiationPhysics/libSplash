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
 


#ifndef DCHELPER_H
#define	DCHELPER_H

#include <sstream>
#include <iostream>
#include <hdf5.h>

namespace splash
{

    /**
     * Provides static help functions for DataCollectors (e.g. SerialDataCollector).
     * \cond HIDDEN_SYMBOLS
     */
    class DCHelper
    {
    private:
        DCHelper();
    public:

        static void printhsizet(const char *name, const hsize_t *data, hsize_t rank)
        {
            std::cerr << name << " = (";
            for (hsize_t i = 0; i < rank - 1; i++)
                std::cerr << data[i] << ", ";
            std::cerr << data[rank - 1];
            std::cerr << ")" << std::endl;

        }

        /**
         * swaps dimensions of hsize_t vector depending on rank
         * @param hs the hsize_t vector
         * @param rank dimensions used in hs
         */
        static void swapHSize(hsize_t *hs, uint32_t rank)
        {
            if (hs == NULL)
                return;

            hsize_t tmp1;
            hsize_t tmp3[3];

            switch (rank)
            {
                case 2:
                    tmp1 = hs[0];
                    hs[0] = hs[1];
                    hs[1] = tmp1;
                    break;
                case 3:
                    tmp3[0] = hs[2];
                    tmp3[1] = hs[1];
                    tmp3[2] = hs[0];

                    for (uint32_t i = 0; i < 3; i++)
                        hs[i] = tmp3[i];
                    break;
                default:
                    return;
            }
        }

        /**
         * @param dims dimensions to get chunk dims for
         * @param rank rank of dims and chunkDims
         * @param typeSize size of each element in bytes
         * @param chunkDims pointer to array for resulting chunk dimensions
         */
        static void getOptimalChunkDims(const hsize_t *dims, uint32_t rank,
                size_t typeSize, hsize_t *chunkDims)
        {
            // some magic numbers at the moment
            // there should be some research on optimal chunk sizes
            // dims and chunkDims sizes are in elements, other sizes are in bytes

            const size_t num_thresholds = 13;
            const size_t threshold_sizes[num_thresholds] = {1024 * 1024 * 2, 1024 * 1024,
                1024 * 512, 1024 * 256, 1024 * 128, 1024 * 64, 1024 * 4,
                1024, 512, 256, 128, 64, 4};

            for (size_t max_threshold_index = 0;
                    max_threshold_index < num_thresholds;
                    ++max_threshold_index)
            {
                for (uint32_t i = 0; i < rank; i++)
                {
                    // dim_size is in bytes
                    size_t dim_size = dims[i] * typeSize;

                    if (dim_size == 0)
                        chunkDims[i] = 1;
                    else
                        chunkDims[i] = threshold_sizes[num_thresholds - 1] / typeSize;

                    for (uint32_t t = max_threshold_index; t < num_thresholds; ++t)
                        if (dim_size >= threshold_sizes[t])
                        {
                            chunkDims[i] = threshold_sizes[t] / typeSize;
                            break;
                        }
                }

                size_t total_size = 1;
                for (uint32_t i = 0; i < rank; i++)
                    total_size *= chunkDims[i] * typeSize;

                if (total_size < 1024 * 1024 * 1024)
                    break;
            }
        }

        /**
         * tests filename for common mistakes when using this library and prints a warning
         * @param filename the filename to test
         * @result true if no errors occurred, false otherwise
         */
        static bool testFilename(const std::string& filename)
        {
            if (filename.rfind(".h5.h5") == filename.length() - 6)
            {
                std::cerr << std::endl << "\tWarning: DCHelper: Do you really want to access "
                        << filename.c_str() << "?" << std::endl;
                return false;
            }

            return true;
        }

    };
    /**
     * \endcond
     */

} // namespace DataCollector

#endif	/* DCHELPER_H */
