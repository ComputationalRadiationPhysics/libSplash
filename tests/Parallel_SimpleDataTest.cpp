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

#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <string>

#include "Parallel_SimpleDataTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_SimpleDataTest);

#define HDF5_FILE "h5/testWriteReadParallel"

#define MPI_CHECK(cmd) \
        { \
                int mpi_result = cmd; \
                if (mpi_result != MPI_SUCCESS) {\
                    std::cout << "MPI command " << #cmd << " failed!" << std::endl; \
                        throw DCException("MPI error"); \
                } \
        }

using namespace splash;

Parallel_SimpleDataTest::Parallel_SimpleDataTest() :
ctInt(),
parallelDataCollector(NULL)
{
    srand(10);

    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
}

Parallel_SimpleDataTest::~Parallel_SimpleDataTest()
{
    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

bool Parallel_SimpleDataTest::testData(const Dimensions mpiSize,
        const Dimensions gridSize, int *data)
{
    for (size_t z = 0; z < mpiSize[2]; ++z)
        for (size_t y = 0; y < mpiSize[1]; ++y)
            for (size_t x = 0; x < mpiSize[0]; ++x)
            {
                Dimensions mpi_pos(x, y, z);
                Dimensions offset = mpi_pos * gridSize;

                for (size_t k = 0; k < gridSize[2]; ++k)
                    for (size_t j = 0; j < gridSize[1]; ++j)
                        for (size_t i = 0; i < gridSize[0]; ++i)
                        {
                            size_t index = (offset[2] + k) * (mpiSize[1] * gridSize[1] *
                                    mpiSize[0] * gridSize[0]) + (offset[1] + j) *
                                    (mpiSize[0] * gridSize[0]) + offset[0] + i;

                            int rank_index = z * (mpiSize[1] * mpiSize[0]) +
                                    y * mpiSize[0] + x + 1;

                            if (data[index] != rank_index)
                            {
#if defined TESTS_DEBUG
                                std::cout << index << ": " << data[index] <<
                                        " != expected " << rank_index << std::endl;
#endif
                                return false;
                            }
                        }
            }

    return true;
}

bool Parallel_SimpleDataTest::subtestWriteRead(int32_t iteration,
        int currentMpiRank,
        const Dimensions mpiSize, const Dimensions mpiPos,
        const Dimensions gridSize, uint32_t dimensions, MPI_Comm mpiComm)
{
    bool results_correct = true;
    DataCollector::FileCreationAttr fileCAttr;
    std::set<std::string> datasetNames;

#if defined TESTS_DEBUG
    if (currentMpiRank == 0)
        std::cout << "iteration: " << iteration << std::endl;
#endif

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    fileCAttr.enableCompression = false;
    parallelDataCollector->open(HDF5_FILE, fileCAttr);

    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = currentMpiRank + 1;

    parallelDataCollector->write(iteration, ctInt, dimensions, gridSize,
            "deep/folder/data", dataWrite);
    datasetNames.insert("deep/folder/data");
    parallelDataCollector->write(iteration, ctInt, dimensions, gridSize,
            "deep/folder/data2", dataWrite);
    datasetNames.insert("deep/folder/data2");
    parallelDataCollector->write(iteration, ctInt, dimensions, gridSize,
            "another_dataset", dataWrite);
    datasetNames.insert("another_dataset");
    parallelDataCollector->close();

    delete[] dataWrite;
    dataWrite = NULL;

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test written data using various mechanisms
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    // need a complete filename here
    std::stringstream filename_stream;
    filename_stream << HDF5_FILE << "_" << iteration << ".h5";

    Dimensions size_read;
    Dimensions full_grid_size = gridSize * mpiSize;
    int *data_read = new int[full_grid_size.getScalarSize()];
    memset(data_read, 0, sizeof (int) * full_grid_size.getScalarSize());

    // test using SerialDataCollector
    if (currentMpiRank == 0)
    {
        DataCollector *dataCollector = new SerialDataCollector(1);
        dataCollector->open(filename_stream.str().c_str(), fileCAttr);

        dataCollector->read(iteration, "deep/folder/data", size_read, data_read);
        dataCollector->close();
        delete dataCollector;

        CPPUNIT_ASSERT(size_read == full_grid_size);
        CPPUNIT_ASSERT(testData(mpiSize, gridSize, data_read));
    }

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test using full read per process
    memset(data_read, 0, sizeof (int) * full_grid_size.getScalarSize());
    ParallelDataCollector *readCollector = new ParallelDataCollector(mpiComm,
            MPI_INFO_NULL, mpiSize, 1);

    readCollector->open(HDF5_FILE, fileCAttr);

    /* test entries listing */
    {
        DataCollector::DCEntry *entries = NULL;
        size_t numEntries = 0;

        int32_t *ids = NULL;
        size_t numIDs = 0;
        readCollector->getEntryIDs(NULL, &numIDs);
        /* there might be old files, but we are at least at the current iteration */
        CPPUNIT_ASSERT(numIDs >= iteration + 1);
        ids = new int32_t[numIDs];
        readCollector->getEntryIDs(ids, NULL);

        readCollector->getEntriesForID(iteration, NULL, &numEntries);
        CPPUNIT_ASSERT(numEntries == 3);
        entries = new DataCollector::DCEntry[numEntries];
        readCollector->getEntriesForID(iteration, entries, NULL);

        CPPUNIT_ASSERT(numEntries == datasetNames.size());
        for (uint32_t i = 0; i < numEntries; ++i)
        {
            /* test that listed datasets match expected dataset names*/
            CPPUNIT_ASSERT(datasetNames.find(entries[i].name) != datasetNames.end());
        }

        delete[] entries;
        delete[] ids;
    }

    readCollector->read(iteration, "deep/folder/data", size_read, data_read);
    readCollector->close();

    CPPUNIT_ASSERT(size_read == full_grid_size);
    CPPUNIT_ASSERT(testData(mpiSize, gridSize, data_read));
    delete[] data_read;

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test using parallel read
    data_read = new int[gridSize.getScalarSize()];
    memset(data_read, 0, sizeof (int) * gridSize.getScalarSize());

    const Dimensions globalOffset = gridSize * mpiPos;
    readCollector->open(HDF5_FILE, fileCAttr);
    readCollector->read(iteration, gridSize, globalOffset, "deep/folder/data",
            size_read, data_read);
    readCollector->close();
    delete readCollector;

    CPPUNIT_ASSERT(size_read == gridSize);

    for (size_t k = 0; k < gridSize[2]; ++k)
    {
        for (size_t j = 0; j < gridSize[1]; ++j)
        {
            for (size_t i = 0; i < gridSize[0]; ++i)
            {
                size_t index = k * gridSize[1] * gridSize[0] +
                        j * gridSize[0] + i;

                if (data_read[index] != currentMpiRank + 1)
                {
#if defined TESTS_DEBUG
                    std::cout << index << ": " << data_read[index] <<
                            " != expected " << currentMpiRank + 1 << std::endl;
#endif
                    results_correct = false;
                    break;
                }
            }
            if (!results_correct)
                break;
        }
        if (!results_correct)
            break;
    }

    delete[] data_read;

    MPI_CHECK(MPI_Barrier(mpiComm));

    return results_correct;
}

static void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
{
    mpiPos[2] = index / (mpiSize[0] * mpiSize[1]);
    mpiPos[1] = (index % (mpiSize[0] * mpiSize[1])) / mpiSize[0];
    mpiPos[0] = index % mpiSize[0];
}

void Parallel_SimpleDataTest::testWriteRead()
{
    uint32_t dimensions = 3;
    int32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getScalarSize();

                int ranks[num_ranks];
                for (uint32_t i = 0; i < num_ranks; ++i)
                    ranks[i] = i;

                MPI_Group world_group, comm_group;
                MPI_Comm mpi_current_comm;
                MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &world_group));
                MPI_CHECK(MPI_Group_incl(world_group, num_ranks, ranks, &comm_group));
                MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, comm_group, &mpi_current_comm));

                // filter mpi ranks
                if (myMpiRank < num_ranks)
                {
                    int my_current_mpi_rank;
                    MPI_Comm_rank(mpi_current_comm, &my_current_mpi_rank);

                    Dimensions current_mpi_pos;
                    indexToPos(my_current_mpi_rank, mpi_size, current_mpi_pos);

#if defined TESTS_DEBUG
                    if (my_current_mpi_rank == 0)
                    {
                        std::cout << std::endl << "** testing mpi size " <<
                                mpi_size.toString() << std::endl;
                    }
#endif

                    parallelDataCollector = new ParallelDataCollector(mpi_current_comm,
                            MPI_INFO_NULL, mpi_size, 10);

                    for (uint32_t k = 1; k < 8; k++)
                        for (uint32_t j = 5; j < 8; j++)
                            for (uint32_t i = 5; i < 8; i++)
                            {
                                Dimensions grid_size(i, j, k);

                                if (my_current_mpi_rank == 0)
                                {
#if defined TESTS_DEBUG
                                    std::cout << std::endl << "** testing grid_size = " <<
                                            grid_size.toString() << " with mpi size " <<
                                            mpi_size.toString() << std::endl;
#else
                                    std::cout << "." << std::flush;
#endif
                                }

                                MPI_CHECK(MPI_Barrier(mpi_current_comm));

                                CPPUNIT_ASSERT(subtestWriteRead(iteration,
                                        my_current_mpi_rank,
                                        mpi_size,
                                        current_mpi_pos,
                                        grid_size,
                                        dimensions,
                                        mpi_current_comm));

                                MPI_CHECK(MPI_Barrier(mpi_current_comm));

                                iteration++;
                            }

                    parallelDataCollector->finalize();
                    delete parallelDataCollector;
                    parallelDataCollector = NULL;

                    MPI_Comm_free(&mpi_current_comm);

                } else
                    iteration += (8 - 1) * (8 - 5) * (8 - 5);

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}

bool Parallel_SimpleDataTest::subtestFill(int32_t iteration,
        int currentMpiRank,
        const Dimensions mpiSize, const Dimensions mpiPos,
        uint32_t elements, MPI_Comm mpiComm)
{
    bool results_correct = true;
    DataCollector::FileCreationAttr fileCAttr;

#if defined TESTS_DEBUG
    if (currentMpiRank == 0)
        std::cout << "iteration: " << iteration << std::endl;
#endif

    // write data to file
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    fileCAttr.enableCompression = false;
    parallelDataCollector->open(HDF5_FILE, fileCAttr);

    int dataWrite = currentMpiRank + 1;
    uint32_t num_elements = (currentMpiRank + 1) * elements;
    Dimensions grid_size(num_elements, 1, 1);

#if defined TESTS_DEBUG
    std::cout << "[" << currentMpiRank << "] " << num_elements << " elements" << std::endl;
#endif

    Dimensions globalOffset, globalSize;
    parallelDataCollector->reserve(iteration, grid_size,
            &globalSize, &globalOffset, 1, ctInt, "reserved/reserved_data");

    int attrVal = currentMpiRank;
    parallelDataCollector->writeAttribute(iteration, ctInt, "reserved/reserved_data",
            "reserved_attr", &attrVal);

    uint32_t elements_written = 0;
    uint32_t global_max_elements = mpiSize.getScalarSize() * elements;
    for (size_t i = 0; i < global_max_elements; ++i)
    {
        Dimensions write_size(1, 1, 1);
        if (i >= num_elements)
            write_size.set(0, 0, 0);

        Dimensions write_offset(globalOffset + Dimensions(elements_written, 0, 0));

        parallelDataCollector->append(iteration, write_size, 1,
                write_offset, "reserved/reserved_data", &dataWrite);

        if (i < num_elements)
            elements_written++;
    }

    MPI_CHECK(MPI_Barrier(mpiComm));

    attrVal = -1;
    parallelDataCollector->readAttribute(iteration, "reserved/reserved_data",
            "reserved_attr", &attrVal, NULL);

    CPPUNIT_ASSERT(attrVal == currentMpiRank);

    parallelDataCollector->close();

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test written data using various mechanisms
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    // need a complete filename here
    std::stringstream filename_stream;
    filename_stream << HDF5_FILE << "_" << iteration << ".h5";

    Dimensions size_read;
    Dimensions full_grid_size = globalSize;


    // test using SerialDataCollector
    if (currentMpiRank == 0)
    {
        int *data_read = new int[full_grid_size.getScalarSize()];
        memset(data_read, 0, sizeof (int) * full_grid_size.getScalarSize());

        DataCollector *dataCollector = new SerialDataCollector(1);
        dataCollector->open(filename_stream.str().c_str(), fileCAttr);

        dataCollector->read(iteration, "reserved/reserved_data",
                size_read, data_read);
        dataCollector->close();
        delete dataCollector;

        CPPUNIT_ASSERT(size_read == full_grid_size);
        CPPUNIT_ASSERT(size_read[1] == size_read[2] == 1);

        int this_rank = 0;
        uint32_t elements_this_rank = num_elements;
        for (uint32_t i = 0; i < size_read.getScalarSize(); ++i)
        {
            if (i == elements_this_rank)
            {
                this_rank++;
                elements_this_rank += num_elements * (this_rank + 1);
            }

            CPPUNIT_ASSERT(data_read[i] == this_rank + 1);
        }

        delete[] data_read;
    }

    MPI_CHECK(MPI_Barrier(mpiComm));

    return results_correct;
}

void Parallel_SimpleDataTest::testFill()
{
    int32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getScalarSize();

                int ranks[num_ranks];
                for (uint32_t i = 0; i < num_ranks; ++i)
                    ranks[i] = i;

                MPI_Group world_group, comm_group;
                MPI_Comm mpi_current_comm;
                MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &world_group));
                MPI_CHECK(MPI_Group_incl(world_group, num_ranks, ranks, &comm_group));
                MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, comm_group, &mpi_current_comm));

                // filter mpi ranks
                if (myMpiRank < num_ranks)
                {
                    int my_current_mpi_rank;
                    MPI_Comm_rank(mpi_current_comm, &my_current_mpi_rank);

                    Dimensions current_mpi_pos;
                    indexToPos(my_current_mpi_rank, mpi_size, current_mpi_pos);

#if defined TESTS_DEBUG
                    if (my_current_mpi_rank == 0)
                    {
                        std::cout << std::endl << "** testing mpi size " <<
                                mpi_size.toString() << std::endl;
                    }
#endif

                    parallelDataCollector = new ParallelDataCollector(mpi_current_comm,
                            MPI_INFO_NULL, mpi_size, 10);

                    for (uint32_t elements = 1; elements <= 8; elements++)
                    {
                        if (my_current_mpi_rank == 0)
                        {
#if defined TESTS_DEBUG
                            std::cout << std::endl << "** testing elements = " <<
                                    elements << " with mpi size " <<
                                    mpi_size.toString() << std::endl;
#else
                            std::cout << "." << std::flush;
#endif
                        }

                        MPI_CHECK(MPI_Barrier(mpi_current_comm));

                        CPPUNIT_ASSERT(subtestFill(iteration,
                                my_current_mpi_rank,
                                mpi_size,
                                current_mpi_pos,
                                elements,
                                mpi_current_comm));

                        MPI_CHECK(MPI_Barrier(mpi_current_comm));

                        iteration++;
                    }

                    parallelDataCollector->finalize();
                    delete parallelDataCollector;
                    parallelDataCollector = NULL;

                    MPI_Comm_free(&mpi_current_comm);

                } else
                    iteration += 8;

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}
