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
#include "SerialDataCollector.hpp"

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

using namespace DCollector;

Parallel_SimpleDataTest::Parallel_SimpleDataTest() :
ctInt(),
parallelDataCollector(NULL),
dataCollector(NULL)
{
    srand(10);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
}

Parallel_SimpleDataTest::~Parallel_SimpleDataTest()
{

}

bool Parallel_SimpleDataTest::subtestWriteRead(uint32_t iteration,
        int currentMpiRank,
        const Dimensions mpiSize,
        const Dimensions gridSize, uint32_t dimensions, MPI_Comm mpiComm)
{
    bool results_correct = true;
    DataCollector::FileCreationAttr fileCAttr;

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

    // initial part of the test: data is written to the file,
    // once with and once without borders
    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = currentMpiRank + 1;

    parallelDataCollector->write(iteration, ctInt, dimensions, gridSize, "data", dataWrite);
    parallelDataCollector->close();

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test written data using serial read
    if (currentMpiRank == 0)
    {
        fileCAttr.fileAccType = DataCollector::FAT_READ;
        // need a complete filename here
        std::stringstream filename_stream;
        filename_stream << HDF5_FILE << "_" << iteration << ".h5";
        dataCollector->open(filename_stream.str().c_str(), fileCAttr);

        Dimensions full_grid_size = gridSize * mpiSize;
        Dimensions size_read;

        int *data_read = new int[full_grid_size.getDimSize()];
        memset(data_read, 0, sizeof (int) * full_grid_size.getDimSize());

        dataCollector->read(iteration, ctInt, "data", size_read, data_read);
        dataCollector->close();

        CPPUNIT_ASSERT(size_read == full_grid_size);
        for (size_t z = 0; z < mpiSize[2]; ++z)
            for (size_t y = 0; y < mpiSize[1]; ++y)
                for (size_t x = 0; x < mpiSize[0]; ++x)
                {
                    Dimensions mpi_pos(x, y, z);

                    for (size_t k = 0; k < gridSize[2]; ++k)
                        for (size_t j = 0; j < gridSize[1]; ++j)
                            for (size_t i = 0; i < gridSize[0]; ++i)
                            {
                                Dimensions offset = mpi_pos * gridSize;

                                size_t index = (offset[2] + k) * (mpiSize[1] * gridSize[1] *
                                        mpiSize[0] * gridSize[0]) + (offset[1] + j) *
                                        (mpiSize[0] * gridSize[0]) + offset[0] + i;

                                size_t rank_index = z * (mpiSize[1] * mpiSize[0]) +
                                        y * mpiSize[0] + x + 1;

                                if (data_read[index] != rank_index)
                                {
#if defined TESTS_DEBUG
                                    std::cout << index << ": " << data_read[index] <<
                                            " != expected " << rank_index << std::endl;
#endif
                                    results_correct = false;
                                    break;
                                }
                            }
                }

        delete[] data_read;

        CPPUNIT_ASSERT_MESSAGE("simple parallel write/read failed", results_correct);
    }

    MPI_CHECK(MPI_Barrier(mpiComm));
    return results_correct;
}

void Parallel_SimpleDataTest::testWriteRead()
{
    uint32_t dimensions = 3;
    uint32_t iteration = 0;

    dataCollector = new SerialDataCollector(1);

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getDimSize();

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
                    
#if defined TESTS_DEBUG
                    if (my_current_mpi_rank == 0)
                    {
                        std::cout << std::endl << "** testing mpi size " <<
                                mpi_size.toString() << std::endl;
                    }
#endif

                    parallelDataCollector = new ParallelDataCollector(mpi_current_comm,
                            MPI_INFO_NULL, mpi_size, my_current_mpi_rank, 10);

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

                                CPPUNIT_ASSERT(subtestWriteRead(iteration, my_current_mpi_rank,
                                        mpi_size, grid_size, dimensions, mpi_current_comm));

                                MPI_CHECK(MPI_Barrier(mpi_current_comm));

                                iteration++;
                            }

                    delete parallelDataCollector;
                    parallelDataCollector = NULL;

                    MPI_Comm_free(&mpi_current_comm);

                } else
                    iteration += (8 - 1) * (8 - 5) * (8 - 5);

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    delete dataCollector;
    dataCollector = NULL;

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}
