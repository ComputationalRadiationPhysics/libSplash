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


#include <mpi.h>
#include <time.h>
#include <stdlib.h>

#include "Parallel_RemoveTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_RemoveTest);

#define HDF5_FILE "h5/testRemoveParallel"

#define MPI_SIZE_X 2

using namespace splash;

Parallel_RemoveTest::Parallel_RemoveTest() :
ctInt()
{
    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(NULL, NULL);

    int mpiSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    CPPUNIT_ASSERT(mpiSize == MPI_SIZE_X);

    dataCollector = new ParallelDataCollector(MPI_COMM_WORLD,
            MPI_INFO_NULL, Dimensions(MPI_SIZE_X, 1, 1), 1);
}

Parallel_RemoveTest::~Parallel_RemoveTest()
{
    if (dataCollector != NULL)
    {
        dataCollector->finalize();
        delete dataCollector;
        dataCollector = NULL;
    }

    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void Parallel_RemoveTest::testRemove()
{
    Dimensions gridSize(2, 1, 1);
    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    dataCollector->open(HDF5_FILE, fileCAttr);

    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = i;

    dataCollector->write(0, ctInt, 1, gridSize, "data", dataWrite);
    dataCollector->write(0, ctInt, 1, gridSize, "folder/data2", dataWrite);
    
    dataCollector->write(1, ctInt, 1, gridSize, "data3", dataWrite);
    
    delete[] dataWrite;
    dataWrite = NULL;

    dataCollector->close();
    
    
    // removing must not be possible in FAT_READ mode
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(HDF5_FILE, fileCAttr);
    
    CPPUNIT_ASSERT_THROW(dataCollector->remove(0), DCException);
    
    dataCollector->close();
    
    // reopen file for writing and remove datasets/groups
    fileCAttr.fileAccType = DataCollector::FAT_WRITE;
    dataCollector->open(HDF5_FILE, fileCAttr);
    
    int *dataRead = new int[bufferSize];
    
    dataCollector->read(0, "data", gridSize, dataRead);
    dataCollector->remove(0, "data");
    CPPUNIT_ASSERT_THROW(dataCollector->read(0, "data", gridSize, dataRead),
            DCException);
    
    dataCollector->read(0, "folder/data2", gridSize, dataRead);
    dataCollector->remove(0, "folder/data2");
    CPPUNIT_ASSERT_THROW(dataCollector->read(0, "data2", gridSize, dataRead),
            DCException);
    CPPUNIT_ASSERT_THROW(dataCollector->read(0, "folder/data2", gridSize, dataRead),
            DCException);
    
    dataCollector->read(1, "data3", gridSize, dataRead);
    dataCollector->remove(1);
    CPPUNIT_ASSERT_THROW(dataCollector->read(1, "data3", gridSize, dataRead),
            DCException);
    
    delete[] dataRead;
    dataRead = NULL;
    
    dataCollector->close();
    
    CPPUNIT_ASSERT(true);
}
