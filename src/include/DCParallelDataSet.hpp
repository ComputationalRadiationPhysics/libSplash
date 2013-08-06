/* 
 * File:   DCParallelDataSet.hpp
 * Author: felix
 *
 * Created on 5. August 2013, 14:02
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
            H5Pset_dxpl_mpio(dsetWriteProperties, H5FD_MPIO_COLLECTIVE);
            
            dsetReadProperties = H5Pcreate(H5P_DATASET_XFER);
            H5Pset_dxpl_mpio(dsetReadProperties, H5FD_MPIO_COLLECTIVE);
        }

        virtual ~DCParallelDataSet()
        {
            H5Pclose(dsetWriteProperties);
            H5Pclose(dsetReadProperties);
        }
    };
}

#endif	/* DCPARALLELDATASET_HPP */

