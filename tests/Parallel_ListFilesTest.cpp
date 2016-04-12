/**
 * Copyright 2014 Felix Schmitt
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

#include "Parallel_ListFilesTest.h"

#include <mpi.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <cppunit/TestAssert.h>
#include <vector>

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_ListFilesTest);

using namespace splash;

#define TEST_FILE "h5/listFilesParallel"
#define TEST_FILE_INVALID "h5/listFilesParallel_invalid"

#define MPI_SIZE_X 1

Parallel_ListFilesTest::Parallel_ListFilesTest()
{
    srand(time(NULL));

    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(NULL, NULL);

    int mpiSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    CPPUNIT_ASSERT(mpiSize == (MPI_SIZE_X));
}

Parallel_ListFilesTest::~Parallel_ListFilesTest()
{
    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void Parallel_ListFilesTest::testListFiles()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    IParallelDataCollector *dataCollector = new ParallelDataCollector(
            MPI_COMM_WORLD, MPI_INFO_NULL, Dimensions(MPI_SIZE_X, 1, 1), 1);

    std::stringstream fileName;
    fileName << TEST_FILE << "_" << time(NULL);

    /* write some files */
    std::vector<int32_t> ids;
    dataCollector->open(fileName.str().c_str(), attr);
    for (int32_t i = 1; i < 1000000; i = (i * 10 + rand() % i))
    {
        int32_t id = i;
        if (id == 1)
            id = 0;

        int32_t dummy_data = id;
        ids.push_back(id);
        dataCollector->write(id, ctInt, 1, Dimensions(1, 1, 1), "data", &dummy_data);
    }
    dataCollector->close();

    attr.fileAccType = DataCollector::FAT_READ;

    /* open invalid fileset */
    dataCollector->open(TEST_FILE_INVALID, attr);
    CPPUNIT_ASSERT(dataCollector->getMaxID() < 0);
    dataCollector->close();

    /* open written file set */
    dataCollector->open(fileName.str().c_str(), attr);

    /* test that ids are complete and correct */
    int32_t max_id = dataCollector->getMaxID();
    CPPUNIT_ASSERT(max_id == ids.back());

    int32_t *tmp_ids = NULL;
    size_t num_ids = 0;
    dataCollector->getEntryIDs(NULL, &num_ids);

    CPPUNIT_ASSERT(num_ids == ids.size());

    tmp_ids = new int32_t[num_ids];
    dataCollector->getEntryIDs(tmp_ids, NULL);

    for (int i = 0; i < num_ids; ++i)
    {
        int32_t tmp_data = -1;
        CPPUNIT_ASSERT(ids[i] == tmp_ids[i]);

        Dimensions size_read;
        dataCollector->read(ids[i], "data", size_read, &tmp_data);

        CPPUNIT_ASSERT(tmp_data == ids[i]);
    }

    dataCollector->close();

    dataCollector->finalize();
    delete dataCollector;
    delete[] tmp_ids;
}
