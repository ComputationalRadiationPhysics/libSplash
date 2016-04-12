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

#include <time.h>
#include <stdio.h>
#include <sstream>

#include "BenchmarkTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(BenchmarkTest);

using namespace splash;

BenchmarkTest::BenchmarkTest()
{
    int argc;
    char** argv;
    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(&argc, &argv);

    dataCollector = new SerialDataCollector(10);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &mpi_size);
    MPI_Comm_rank(comm, &mpi_rank);
}

BenchmarkTest::~BenchmarkTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

void BenchmarkTest::runComparison(uint32_t cx, uint32_t cy, uint32_t cz,
        Dimensions gridSize, void* data)
{
    std::stringstream file_stream;
    file_stream << "/scratch/fschmitt/hdf5/bench_compare_" << cx << "_" << cy << "_" << cz << ".h5";

    FILE *file = fopen(file_stream.str().c_str(), "w");

    fwrite(data, ctInt.getSize(), gridSize.getScalarSize(), file);

    fclose(file);
}

void BenchmarkTest::runBenchmark(uint32_t cx, uint32_t cy, uint32_t cz,
        Dimensions gridSize, void* data)
{
    DataCollector::FileCreationAttr attr;

    Dimensions mpiPosition(mpi_rank % cx, (mpi_rank / cx) % cy,
            (mpi_rank / cx) / cy);

    attr.fileAccType = DataCollector::FAT_CREATE;
    attr.enableCompression = false;
    attr.mpiPosition = mpiPosition;
    attr.mpiSize = Dimensions(cx, cy, cz);
    dataCollector->open("/fastfs/fschmitt/hdf5/bench", attr);
    //dataCollector->open("/tmp/bench", attr);

    dataCollector->write(0, ctInt, 3, Selection(gridSize), "data", data);

    dataCollector->close();
}

void BenchmarkTest::testBenchmark()
{
    printf("\n");

    size_t width = 512;

    Dimensions gridSize(width, width, width);
    //Dimensions localGridSize(width / cx, width / cy, width / cz);
    /*Dimensions gridOffset(localGridSize[0] * mpiPosition[0],
            localGridSize[1] * mpiPosition[1],
            localGridSize[2] * mpiPosition[2]);*/

    size_t buffer_size = gridSize[0] * gridSize[1] * gridSize[2];
    int *data = new int[buffer_size];

    std::cout << "Buffersize/node = " << buffer_size / 1024 << " KB" << std::endl;

    for (size_t i = 0; i < buffer_size; i++)
        data[i] = buffer_size * mpi_rank + i;

    for (uint32_t z = 1; z < 5; z++)
        for (uint32_t y = 1; y < 5; y++)
            for (uint32_t x = 1; x < 5; x++)
            {
                if (x * y * z <= mpi_size)
                {
                    clock_t start = clock();
                    runBenchmark(x, y, z, gridSize, data);
                    clock_t end = clock();

                    if (mpi_rank == 0)
                        printf("hdf5: R%02d: (%2d, %2d, %2d): %4.2f sec.\n",
                            mpi_rank, x, y, z, (float) (end - start) / CLOCKS_PER_SEC);

                    start = clock();
                    runComparison(x, y, z, gridSize, data);
                    end = clock();

                    if (mpi_rank == 0)
                        printf("native: R%02d: (%2d, %2d, %2d): %4.2f sec.\n",
                            mpi_rank, x, y, z, (float) (end - start) / CLOCKS_PER_SEC);
                }

                MPI_Barrier(comm);
            }

    delete[] data;

    CPPUNIT_ASSERT(true);
}

