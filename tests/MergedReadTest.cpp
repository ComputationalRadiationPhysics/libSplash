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
 


#include "MergedReadTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(MergedReadTest);

#define HDF5_FILE "h5/testMergedRead"
#define HDF5_APPEND_FILE "h5/testAppendMergedRead"

//#define TESTS_DEBUG

using namespace DCollector;

MergedReadTest::MergedReadTest() :
ctInt()
{
    throw DCException("This test is currently not supported !");

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

MergedReadTest::~MergedReadTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

void MergedReadTest::writeFiles(const char *file, int dimensions,
        Dimensions& gridSize, Dimensions& mpiSize)
{
    Dimensions smallGridSize(gridSize[0] / mpiSize[0],
            gridSize[1] / mpiSize[1],
            gridSize[2] / mpiSize[2]);

    Dimensions mpiPosition(mpi_rank % mpiSize[0], (mpi_rank / mpiSize[0]) % mpiSize[1],
            (mpi_rank / mpiSize[0]) / mpiSize[1]);

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    fileCAttr.enableCompression = false;
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    fileCAttr.mpiPosition = mpiPosition;
    fileCAttr.mpiSize = mpiSize;
    dataCollector->open(file, fileCAttr);

    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = i;

    Dimensions offset(smallGridSize[0] * mpiPosition[0], smallGridSize[1] * mpiPosition[1],
            smallGridSize[2] * mpiPosition[2]);

#if defined TESTS_DEBUG
    std::cout << mpi_rank << ": writing data..." << std::endl;
#endif
    dataCollector->write(0, ctInt, dimensions, gridSize, smallGridSize, offset,
            "data", dataWrite);

    dataCollector->close();

    delete[] dataWrite;
}

bool MergedReadTest::subTestMergedRead(uint32_t dimensions, Dimensions gridSize, Dimensions mpiSize)
{
    bool resultsCorrect = true;

    int max_rank = 1;
    for (int i = 0; i < dimensions; i++)
        max_rank *= mpiSize[i];

    if (mpi_rank < max_rank)
        writeFiles(HDF5_FILE, dimensions, gridSize, mpiSize);

    MPI_Barrier(comm);

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    if (mpi_rank == 0)
    {
        DataCollector::FileCreationAttr fileCAttr;
        fileCAttr.enableCompression = false;
        fileCAttr.fileAccType = DataCollector::FAT_READ_MERGED;
        dataCollector->open(HDF5_FILE, fileCAttr);

        int *dataRead = new int[bufferSize];
        for (uint32_t i = 0; i < bufferSize; i++)
            dataRead[i] = -1;

        Dimensions resultSize;

        dataCollector->read(0, ctInt, "data", resultSize, dataRead);
        dataCollector->close();

        for (uint32_t i = 0; i < dimensions; i++)
            CPPUNIT_ASSERT(resultSize[i] == gridSize[i]);

        for (uint32_t i = 0; i < bufferSize; i++)
        {
            if (dataRead[i] != i)
            {
                std::cout << mpi_rank << ": " << dataRead[i] << " != exptected " << i << std::endl;
                resultsCorrect = false;
                break;
            }
        }

        delete[] dataRead;
    }

    return resultsCorrect;
}

typedef int int4[4];

void MergedReadTest::testMergedRead()
{
    const int MAX_COMB = 12;
    const int4 combinations[MAX_COMB] = {
        {1, 1, 1, 1},
        {1, 2, 1, 1},
        {1, 3, 1, 1},
        {2, 1, 1, 1},
        {2, 2, 1, 1},
        {2, 1, 2, 1},
        {2, 2, 3, 1},
        {3, 1, 1, 1},
        {3, 2, 1, 1},
        {3, 2, 1, 2},
        {3, 2, 2, 1},
        {3, 2, 2, 2}
    };

    for (int c = 0; c < MAX_COMB; c++)
    {
#if defined TESTS_DEBUG
        if (mpi_rank == 0)
            std::cout << std::endl << "combination=" << c << std::endl;
#endif

        Dimensions mpiSize(1, 1, 1);
        Dimensions gridSize(1, 1, 1);

        int dimensions = combinations[c][0];
        for (int m = 0; m < 3; m++)
            mpiSize[m] = combinations[c][m + 1];

        gridSize[0] = 2 * mpiSize[0];

        if (dimensions > 1)
        {
            gridSize[1] = 2 * mpiSize[1];

            if (dimensions > 2)
            {
                gridSize[2] = 3 * mpiSize[2];
            }
        }

        int max_rank = 1;
        for (int i = 0; i < dimensions; i++)
            max_rank *= mpiSize[i];

#if defined TESTS_DEBUG
        if (mpi_rank == 0)
            std::cout << "testing: dim=" << dimensions << " with (" << gridSize[0] <<
                ", " << gridSize[1] << ", " << gridSize[2] << ")" << std::endl;
#endif

        CPPUNIT_ASSERT_MESSAGE("readMerged failed",
                subTestMergedRead(dimensions, gridSize, mpiSize));

        std::cout << std::flush;
        MPI_Barrier(comm);
    }
}

void MergedReadTest::appendData(Dimensions mpiPosition, Dimensions mpiSize)
{
    DataCollector::FileCreationAttr fileCAttr;
    fileCAttr.enableCompression = false;
    fileCAttr.fileAccType = DataCollector::FAT_CREATE;
    fileCAttr.mpiPosition = mpiPosition;
    fileCAttr.mpiSize = mpiSize;
    dataCollector->open(HDF5_APPEND_FILE, fileCAttr);

    const size_t buffer_size = 32 * (mpi_rank + 1);

    int *data_write = new int[buffer_size];

    for (uint32_t i = 0; i < buffer_size; i++)
        data_write[i] = mpi_rank;

    dataCollector->append(0, ctInt, buffer_size, "adata", data_write);
    
    dataCollector->close();

    delete[] data_write;
}

void MergedReadTest::testAppendMergedRead()
{
    Dimensions mpi_write_size(2, 2, 2);

    Dimensions mpi_position(mpi_rank % mpi_write_size[0], (mpi_rank / mpi_write_size[0]) % mpi_write_size[1],
            (mpi_rank / mpi_write_size[0]) / mpi_write_size[1]);

    //std::cout << "mpi_pos = " << mpi_position.toString() << " mpi_rank = " << mpi_rank << std::endl;

    appendData(mpi_position, mpi_write_size);

    MPI_Barrier(comm);

    if (mpi_rank == 0)
    {
        DataCollector::FileCreationAttr fileCAttr;
        fileCAttr.enableCompression = false;
        fileCAttr.fileAccType = DataCollector::FAT_READ_MERGED;
        fileCAttr.mpiPosition = mpi_position;
        fileCAttr.mpiSize = mpi_write_size;
        dataCollector->open(HDF5_APPEND_FILE, fileCAttr);

        Dimensions src_buffer;
        dataCollector->read(0, ctInt, "adata", src_buffer, NULL);

        int *data_read = new int[src_buffer[0]];

        dataCollector->read(0, ctInt, "adata", src_buffer, data_read);
        
        dataCollector->close();

        int offset = 0;
        for (int m = 0; m < mpi_write_size.getDimSize(); m++)
        {
            for (int i = 0; i < 32 * (m + 1); i++)
            {
                //std::cout << offset + i << ": " << data_read[offset + i] << " (" << m << ")" << std::endl;
                CPPUNIT_ASSERT(data_read[offset + i] == m);
            }
            
            offset += 32 * (m + 1);
        }

        delete[] data_read;

        
    } else
        CPPUNIT_ASSERT(true);
}

