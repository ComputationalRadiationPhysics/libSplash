/**
 * Copyright 2013-2014 Felix Schmitt
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

#define NUM_TEST_LOOPS 100

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
        int64_t *nullData = NULL;

        /* clear data buffer */
        for (size_t i = 0; i < dataSize; ++i)
            data[i] = -1;
        
        /* set and write number of data elements for this round */
        size_t elements = 0;
        size_t zeroAccess = rand() % 2;
        if (!zeroAccess) {
            elements = (rand() % dataSize) + 1;
            nullData = new int64_t[elements];
        }
        
        size_t readElements = 100000;
        for (size_t i = 0; i < elements; ++i)
            data[i] = dataSize * myMpiRank + i;
        
        /* open file for writing */
        attr.fileAccType = DataCollector::FAT_CREATE;
        pdc->open(HDF5_FILE, attr);
        
        pdc->write(10, ctInt64, 1, Selection(Dimensions(dataSize, 1, 1),
                Dimensions(elements, 1, 1), Dimensions(0, 0, 0)), "data", data);
        
        pdc->write(10, ctUInt64, 1, Dimensions(1, 1, 1), "elements", &elements);
        
        pdc->close();
        
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
        size_t totalSize = 0;
        for (size_t i = 0; i < totalMpiSize; ++i)
        {
            totalSize += allNumElements[i];
            if (i < myMpiRank)
                myOffset += allNumElements[i];
        }
        
        if ( (readElements == 0) && (rand() % 2 == 0))
        {
            myOffset = totalSize;
        }
        
        /* read data for comparison */
        pdc->read(10, Dimensions(readElements, 1, 1), Dimensions(myOffset, 0, 0),
                "data", sizeRead, data);

        CPPUNIT_ASSERT(sizeRead == Dimensions(elements, 1, 1));

        /* read data, use NULL ptr if none elements to read */
        pdc->read(10, Dimensions(readElements, 1, 1), Dimensions(myOffset, 0, 0),
                "data", sizeRead, nullData);

        CPPUNIT_ASSERT(sizeRead == Dimensions(elements, 1, 1));

        if (nullData)
        {
            delete[] nullData;
        }
        
        for (size_t i = 0; i < dataSize; ++i)
        {
            if (i < readElements)
            {
                if (data[i] != dataSize * myMpiRank + i)
                {
                    std::cerr << myMpiRank << ": " << data[i] <<
                            " != " << dataSize * myMpiRank + i << std::endl;
                }
                CPPUNIT_ASSERT(data[i] == dataSize * myMpiRank + i);
            }
            else
            {
                if (data[i] != -1)
                {
                    std::cerr << myMpiRank << ": " << data[i]
                            << " != " << -1 << std::endl;
                }
                CPPUNIT_ASSERT(data[i] == -1);
            }
        }
        
        pdc->close();
        
        if (myMpiRank == 0)
            std::cout << "." << std::flush;
    }
    
    pdc->finalize();
    
    delete pdc;
    pdc = NULL;
}
