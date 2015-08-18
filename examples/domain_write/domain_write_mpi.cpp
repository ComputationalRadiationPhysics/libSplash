/**
 * Copyright 2013-2015 Felix Schmitt, Richard Pausch
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

#include <mpi.h>

#include <iostream>
#include <string>
#include <stdlib.h>

#include "splash/splash.h"

using namespace splash;

/**
 * This libSplash example demonstrates on how to use the DomainCollector class
 * to write multiple, domain-annotated libSplash files of a single file set.
 * The output of this example can be read with the domain_read and domain_read_mpi examples.
 * 
 * The program expects the base part to a libSplash filename and the MPI process topology.
 */

void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
{
    mpiPos[2] = index % mpiSize[2];
    mpiPos[1] = (index / mpiSize[2]) % mpiSize[1];
    mpiPos[0] = index / (mpiSize[1] * mpiSize[2]);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    
    if (argc < 5)
    {
        std::cout << "Usage: " << argv[0] << " <libsplash-file-base> <x> <y> <z>" << std::endl;
        return -1;
    }
    
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    /* libSplash filename */
    std::string filename;
    filename.assign(argv[1]);
    
    Dimensions mpiTopology(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
    Dimensions mpiPosition;
    if (mpi_size != mpiTopology.getScalarSize())
    {
        std::cout << "MPI processes and topology do not match!" << std::endl;
        return -1;
    }
    
    indexToPos(mpi_rank, mpiTopology, mpiPosition);

    /* create DomainCollector */
    DomainCollector dc(100);
    DataCollector::FileCreationAttr fAttr;
    DataCollector::initFileCreationAttr(fAttr);

    fAttr.fileAccType = DataCollector::FAT_CREATE;
    fAttr.mpiPosition.set(mpiPosition);

    dc.open(filename.c_str(), fAttr);

    /* create data for writing */
    
    Dimensions localGridSize(10, 20, 5);
    
    ColTypeFloat ctFloat;
    float *data = new float[localGridSize.getScalarSize()];
    memset(data, 1, sizeof(float) * localGridSize.getScalarSize());
    
    /* where our example logically starts */
    Dimensions globalDomainOffset(100, 100, 100);
    /* where this process logically starts */
    Dimensions localDomainOffset(mpiPosition * localGridSize);
    
    /**
     * See your local Doxygen documentation or online at
     * http://computationalradiationphysics.github.io/libSplash/classsplash_1_1_domain_collector.html
     * for more information on this interface.
     **/
    dc.writeDomain(10,                    /* iteration step (timestep in case of PIConGPU) */
            ctFloat,                      /* data type */
            localGridSize.getDims(),      /* number of dimensions (here 3) */
            Selection(localGridSize),     /* data size of this process */
            "float_data",                 /* name of dataset */
            Domain(localDomainOffset,           /* logical offset of the data of this process */
            localGridSize),               /* logical size of the data of this process
                                           * must match actual data size for grids */
            Domain(globalDomainOffset,                /* logical start of the data of all processes */
            localGridSize * mpiTopology), /* logical size of the data of all processes */
            DomainCollector::GridType,    /* we are writing grid (not poly) data here */
            data);                        /* the actual data buffer (pointer) */
    
    dc.close();
    
    delete[] data;
    
    MPI_Finalize();

    return 0;
}
