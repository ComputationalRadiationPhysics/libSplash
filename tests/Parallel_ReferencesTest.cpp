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

#include "Parallel_ReferencesTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_ReferencesTest);

#define HDF5_FILE "h5/testReferencesParallel"

using namespace DCollector;

Parallel_ReferencesTest::Parallel_ReferencesTest() :
ctInt()
{
    srand(time(NULL));

    int argc;
    char** argv;
    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    dataCollector = new ParallelDataCollector(MPI_COMM_WORLD, MPI_INFO_NULL,
            Dimensions(mpiSize, 1, 1), 10);
}

Parallel_ReferencesTest::~Parallel_ReferencesTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }

    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void Parallel_ReferencesTest::testCreateReference()
{
    Dimensions gridSize(6, 8, 2);
    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    dataCollector->open(HDF5_FILE, fileCAttr);

    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = i;

    dataCollector->write(2, ctInt, 3, gridSize, "data", dataWrite);

    CPPUNIT_ASSERT_THROW(dataCollector->createReference(0, "data", 1, "ref2",
            Dimensions(2, 2, 1), Dimensions(1, 5, 1), Dimensions(2, 1, 1)),
            DCException);

    CPPUNIT_ASSERT_THROW(dataCollector->createReference(0, "data", 2, "obj_ref"),
            DCException);

    // bound to fail as feature no supported by Parallel HDF5
    CPPUNIT_ASSERT_THROW(dataCollector->createReference(2, "data", 2, "ref",
            Dimensions(5, 7, 2), Dimensions(0, 0, 0), Dimensions(1, 1, 1)),
            DCException);

    dataCollector->createReference(2, "data", 2, "obj_ref");

    delete[] dataWrite;
    dataWrite = NULL;

    dataCollector->close();

    CPPUNIT_ASSERT(true);
}
