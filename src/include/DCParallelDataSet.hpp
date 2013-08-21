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

#ifndef DCPARALLELDATASET_HPP
#define	DCPARALLELDATASET_HPP

#include "DCDataSet.hpp"


namespace DCollector
{

    class DCParallelDataSet : public DCDataSet
    {
    public:

        DCParallelDataSet(const std::string name) :
        DCDataSet(name)
        {
            dsetWriteProperties = H5Pcreate(H5P_DATASET_XFER);
#if defined (SPLASH_INDEPENDENT_IO)
            H5Pset_dxpl_mpio(dsetWriteProperties, H5FD_MPIO_INDEPENDENT);
#else
            H5Pset_dxpl_mpio(dsetWriteProperties, H5FD_MPIO_COLLECTIVE);
#endif

            dsetReadProperties = H5Pcreate(H5P_DATASET_XFER);
            H5Pset_dxpl_mpio(dsetReadProperties, H5FD_MPIO_COLLECTIVE);

            checkExistence = false;
        }

        virtual ~DCParallelDataSet()
        {
            H5Pclose(dsetWriteProperties);
            H5Pclose(dsetReadProperties);
        }
    };
}

#endif	/* DCPARALLELDATASET_HPP */

