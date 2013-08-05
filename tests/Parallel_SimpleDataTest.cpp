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



#include <time.h>
#include <stdlib.h>
#include <mpi/mpi.h>

#include "Parallel_SimpleDataTest.h"
#include "ParallelDataCollector.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_SimpleDataTest);

#define HDF5_FILE "h5/testWriteReadParallel"

//#define TESTS_DEBUG

using namespace DCollector;

Parallel_SimpleDataTest::Parallel_SimpleDataTest() :
ctInt()
{
    srand(time(NULL));

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
}

Parallel_SimpleDataTest::~Parallel_SimpleDataTest()
{

}

bool Parallel_SimpleDataTest::subtestWriteRead(uint32_t iteration,
        const Dimensions gridSize, const Dimensions borderSize, uint32_t dimensions)
{
    /*Dimensions smallGridSize(gridSize[0] - 2 * borderSize[0],
            gridSize[1] - 2 * borderSize[1],
            gridSize[2] - 2 * borderSize[2]);*/

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    dataCollector->open(HDF5_FILE, fileCAttr);

    // initial part of the test: data is written to the file,
    // once with and once without borders
    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = myMpiRank + 1;

    dataCollector->write(iteration, ctInt, dimensions, gridSize, "data", dataWrite);

    //dataCollector->write(10, ctInt, dimensions, gridSize, smallGridSize,
    //      borderSize, "data_without_borders", dataWrite);

    dataCollector->close();

    return true;
}

void Parallel_SimpleDataTest::testWriteRead()
{
    uint32_t dimensions = 3;
    uint32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 2; ++mpi_z)
        for (uint32_t mpi_y = 2; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 2; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);

                int max_rank = 1;
                for (int i = 0; i < 3; i++)
                    max_rank *= mpi_size[i];
                
                if (myMpiRank < max_rank)
                {
                    dataCollector = new ParallelDataCollector(MPI_COMM_WORLD,
                            MPI_INFO_NULL, mpi_size, myMpiRank, 10);

                    Dimensions gridSize(4, 2, 1);
                    Dimensions borderSize(1, 1, 1);

                    CPPUNIT_ASSERT(subtestWriteRead(iteration,
                            gridSize, borderSize, dimensions));

                    delete dataCollector;
                    dataCollector = NULL;
                }
                
                iteration++;
            }

    MPI_Barrier(MPI_COMM_WORLD);
}
