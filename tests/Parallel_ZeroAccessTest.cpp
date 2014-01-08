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
#include <string.h>
#include <cppunit/TestAssert.h>
#include <iosfwd>
#include <bits/basic_string.h>

#include "Parallel_ZeroAccessTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_ZeroAccessTest);

#define HDF5_FILE "h5/testZeroAccessParallel"

#define MPI_CHECK(cmd) \
        { \
                int mpi_result = cmd; \
                if (mpi_result != MPI_SUCCESS) {\
                    std::cout << "MPI command " << #cmd << " failed!" << std::endl; \
                        throw DCException("MPI error"); \
                } \
        }

using namespace splash;

#define NUM_TEST_LOOPS 10

Parallel_ZeroAccessTest::Parallel_ZeroAccessTest()
{
    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
    
    srand(totalMpiSize + myMpiRank);
}

Parallel_ZeroAccessTest::~Parallel_ZeroAccessTest()
{
    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void Parallel_ZeroAccessTest::testZeroAccess()
{
    ColTypeInt64 ctInt64;
    ColTypeUInt64 ctUInt64;
    ParallelDomainCollector *pdc = new ParallelDomainCollector(MPI_COMM_WORLD,
            MPI_INFO_NULL, Dimensions(totalMpiSize, 1, 1), 1);
    
    const size_t dataSize = 100;
    int64_t data[dataSize];
    
    DataCollector::FileCreationAttr attr;
    attr.mpiPosition.set(myMpiRank, 0, 0);
    attr.mpiSize.set(totalMpiSize, 1, 1);
    attr.enableCompression = false;
    
    /* test loops */
    for (size_t loop = 0; loop < NUM_TEST_LOOPS; ++loop)
    {
        std::stringstream strname;
        strname << "data_" << loop;
        
        /* clear data buffer */
        for (size_t i = 0; i < dataSize; ++i)
            data[i] = -1;
        
        /* set and write number of data elements for this round */
        size_t elements = 0;
        size_t zeroAccess = rand() % 2;
        if (!zeroAccess)
         elements = (rand() % dataSize) + 1;
        
        size_t allElements[totalMpiSize];
        
        MPI_Gather(&elements, 1, MPI_UINT64_T, allElements, 1,
                MPI_UINT64_T, 0, MPI_COMM_WORLD);
        
        if (myMpiRank == 0)
        {
            std::cout << "elem table original" << std::endl;
            for (int i = 0; i < totalMpiSize; ++i)
                std::cout << "rank " << i << ": " << allElements[i] << ", ";
            std::cout << std::endl;
        }
        
        size_t readElements = 100000;
        for (size_t i = 0; i < elements; ++i)
            data[i] = dataSize * myMpiRank + i;
        
        /* open file for writing */
        attr.fileAccType = DataCollector::FAT_CREATE;
        pdc->open(HDF5_FILE, attr);
        
        pdc->write(10, ctInt64, 1, Dimensions(dataSize, 1, 1),
                Dimensions(elements, 1, 1), Dimensions(0, 0, 0), strname.str().c_str(), data);
        
        pdc->write(10, ctUInt64, 1, Dimensions(1, 1, 1), "elements", &elements);
        
        pdc->close();
        
        MPI_Barrier(MPI_COMM_WORLD);
        
        /* clear data buffer */
        for (size_t i = 0; i < dataSize; ++i)
            data[i] = -1;
        
        /* open file for reading */
        attr.fileAccType = DataCollector::FAT_READ;
        pdc->open(HDF5_FILE, attr);
        
        /* read number of own elements (just for testing) */
        Dimensions sizeRead(0, 0, 0);
        pdc->read(10, Dimensions(1, 1, 1), Dimensions(myMpiRank, 0, 0),
                "elements", sizeRead, &readElements);
        
        CPPUNIT_ASSERT(sizeRead == Dimensions(1, 1, 1));
        CPPUNIT_ASSERT(readElements == elements);
        
        /* read complete elements index table */
        size_t allNumElements[totalMpiSize];
        pdc->read(10, "elements", sizeRead, allNumElements);
        
        CPPUNIT_ASSERT(sizeRead == Dimensions(totalMpiSize, 1, 1));
        
        /* compute offset for reading */
        size_t myOffset = 0;
        size_t totalElements = 0;
        for (size_t i = 0; i < myMpiRank; ++i)
            myOffset += allNumElements[i];
        
        for (size_t i = 0; i < totalMpiSize; ++i)
            totalElements += allNumElements[i];
        
        int64_t allData[dataSize*totalMpiSize];
        pdc->read(10, Dimensions(totalElements, 1, 1), Dimensions(0, 0, 0),
                strname.str().c_str(), sizeRead, allData);
        
        if (myMpiRank == 0)
        {
            std::cout << "elem table file" << std::endl;
            for (int i = 0; i < totalMpiSize; ++i)
                std::cout << "rank " << i << ": " << allNumElements[i] << ", ";
            std::cout << std::endl;
            
            for (int i = 0; i < totalElements; ++i)
                std::cout << allData[i] << " ";
            std::cout << std::endl;
        }
        
        /* read data for comparison */
        pdc->read(10, Dimensions(readElements, 1, 1), Dimensions(myOffset, 0, 0),
                strname.str().c_str(), sizeRead, data);
        
        CPPUNIT_ASSERT(sizeRead == Dimensions(elements, 1, 1));
        
        for (size_t i = 0; i < dataSize; ++i)
        {
            if (i < readElements)
            {
                if (data[i] != dataSize * myMpiRank + i)
                {
                    printf("rank %d writes %llu elements\n", myMpiRank, (long long unsigned)elements);
                    printf("rank %d reading %llu elements\n", myMpiRank, (long long unsigned)readElements);
                    printf("rank %d offset = %llu\n", myMpiRank, (long long unsigned)myOffset);
                    printf("%lld == %lld, rank = %d, i = %llu\n",
                            (long long)(data[i]),
                            (long long)(dataSize * myMpiRank + i),
                            myMpiRank, (long long unsigned)i);
                }
                CPPUNIT_ASSERT(data[i] == dataSize * myMpiRank + i);
            }
            else
            {
                CPPUNIT_ASSERT(data[i] == -1);
            }
        }
        
        pdc->close();
        
        if (myMpiRank == 0)
            std::cout << "." << std::flush;
        
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    delete pdc;
    pdc = NULL;
}
