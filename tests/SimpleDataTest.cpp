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

#include "SimpleDataTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(SimpleDataTest);

#define HDF5_FILE "h5/testWriteRead"

//#define TESTS_DEBUG

using namespace DCollector;

SimpleDataTest::SimpleDataTest() :
ctInt()
{
    dataCollector = new SerialDataCollector(10);
    srand(time(NULL));
}

SimpleDataTest::~SimpleDataTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

bool SimpleDataTest::subtestWriteRead(Dimensions gridSize, Dimensions borderSize, uint32_t dimensions)
{
    bool resultsCorrect = true;

    Dimensions smallGridSize(gridSize[0] - 2 * borderSize[0],
            gridSize[1] - 2 * borderSize[1],
            gridSize[2] - 2 * borderSize[2]);

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    dataCollector->open(HDF5_FILE, fileCAttr);

    // initial part of the test: data is written to the file, once with and once
    // without borders
    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = i;

    dataCollector->write(10, ctInt, dimensions, gridSize, "deep/folders/data", dataWrite);

    dataCollector->write(20, ctInt, dimensions, gridSize, smallGridSize,
            borderSize, "deep/folders/data_without_borders", dataWrite);

    dataCollector->close();

    // first part of the test: read data with borders to a cleared
    // array (-1) and test results

    // read data from file
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(HDF5_FILE, fileCAttr);

#if defined TESTS_DEBUG
    printf("printing file entries:\n");
#endif
    DataCollector::DCEntry *entries = NULL;
    size_t numEntries = 0;

    int32_t *ids = NULL;
    size_t numIDs = 0;
    dataCollector->getEntryIDs(NULL, &numIDs);
    CPPUNIT_ASSERT(numIDs == 2);
    ids = new int32_t[numIDs];
    dataCollector->getEntryIDs(ids, NULL);

    for (uint32_t j = 0; j < numIDs; ++j)
    {
        dataCollector->getEntriesForID(ids[j], NULL, &numEntries);
        CPPUNIT_ASSERT(numEntries == 1);
        entries = new DataCollector::DCEntry[numEntries];
        dataCollector->getEntriesForID(ids[j], entries, NULL);

#if defined TESTS_DEBUG
        for (uint32_t i = 0; i < numEntries; ++i)
            printf("id=%d name=%s\n", ids[j], entries[i].name.c_str());
#endif

        delete[] entries;
    }

    delete[] ids;

    int *dataRead = new int[bufferSize];
    for (uint32_t i = 0; i < bufferSize; i++)
        dataRead[i] = -1;

    Dimensions resultSize;
    dataCollector->read(10, "deep/folders/data", resultSize, dataRead);

    for (uint32_t i = 0; i < 3; i++)
        CPPUNIT_ASSERT(resultSize[i] == gridSize[i]);

    for (uint32_t i = 0; i < bufferSize; i++)
        if (dataRead[i] != dataWrite[i])
        {
#if defined TESTS_DEBUG
            std::cout << i << ": " << dataRead[i] << " != exptected " << dataWrite[i] << std::endl;
#endif
            resultsCorrect = false;
            break;
        }

    delete[] dataRead;

    CPPUNIT_ASSERT_MESSAGE("simple write/read failed", resultsCorrect);


    // second part of this test: data without borders is read to its original
    // locations in a cleared array (-1)

    dataRead = new int[bufferSize];
    for (uint32_t i = 0; i < bufferSize; i++)
        dataRead[i] = -1;

    dataCollector->read(20, "deep/folders/data_without_borders", gridSize, borderSize,
            resultSize, NULL);

    for (uint32_t i = 0; i < 3; i++)
        CPPUNIT_ASSERT(resultSize[i] == smallGridSize[i]);

    dataCollector->read(20, "deep/folders/data_without_borders", gridSize, borderSize,
            resultSize, dataRead);

    // print out read and written data for debugging purposes
#if defined TESTS_DEBUG
    for (size_t k = 0; k < gridSize[2]; k++)
    {
        std::cout << "k = " << k << std::endl;

        for (size_t j = 0; j < gridSize[1]; j++)
        {
            for (size_t i = 0; i < gridSize[0]; i++)
            {
                uint32_t index = k * (gridSize[0] * gridSize[1]) + j * gridSize[0] + i;
                std::cout << dataRead[index] << " ";
            }
            std::cout << std::endl;
        }
    }

    for (size_t k = 0; k < gridSize[2]; k++)
    {
        std::cout << "k = " << k << std::endl;

        for (size_t j = 0; j < gridSize[1]; j++)
        {
            for (size_t i = 0; i < gridSize[0]; i++)
            {
                uint32_t index = k * (gridSize[0] * gridSize[1]) + j * gridSize[0] + i;
                std::cout << dataWrite[index] << " ";
            }
            std::cout << std::endl;
        }
    }
#endif

    for (size_t i = 0; i < gridSize[0]; i++)
        for (size_t j = 0; j < gridSize[1]; j++)
            for (size_t k = 0; k < gridSize[2]; k++)
            {
                uint32_t index = k * (gridSize[0] * gridSize[1]) + j * gridSize[0] + i;

                int value = dataRead[index];
                int check_value = -1;

                if ((i >= borderSize[0] && i < gridSize[0] - borderSize[0]) &&
                        (j >= borderSize[1] && j < gridSize[1] - borderSize[1]) &&
                        (k >= borderSize[2] && k < gridSize[2] - borderSize[2]))
                    check_value = dataWrite[index];

                if (value != check_value)
                {
#if defined TESTS_DEBUG
                    std::cout << "(i, j, k) == index == " << i << ", " << j << ", " << k << std::endl;
                    std::cout << value << " != exptected " << check_value << std::endl;
#endif
                    resultsCorrect = false;
                    break;
                }
            }

    delete[] dataRead;
    delete[] dataWrite;

    dataCollector->close();

    CPPUNIT_ASSERT_MESSAGE("write/read with offsets failed", resultsCorrect);

    return resultsCorrect;
}

void SimpleDataTest::testWriteRead()
{
    uint32_t dimensions = 3;

    for (uint32_t k = 5; k < 8; k++)
        for (uint32_t j = 5; j < 8; j++)
            for (uint32_t i = 5; i < 8; i++)
            {
                Dimensions gridSize(i, j, k);
                Dimensions borderSize(rand() % 2 + 1, rand() % 2 + 1, rand() % 2 + 1);

                CPPUNIT_ASSERT(subtestWriteRead(gridSize, borderSize, dimensions));
            }

}

void SimpleDataTest::testNullWrite()
{
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    dataCollector->open(HDF5_FILE, fileCAttr);

    Dimensions size(100, 20, 17);

    dataCollector->write(10, ctInt, 3, size, "deep/folders/null", NULL);
    
    dataCollector->write(10, ctInt, 3, Dimensions(0, 0, 0), "deep/folders/null_2", NULL);

    dataCollector->close();

    // first part of the test: read data with borders to a cleared
    // array (-1) and test results

    // read data from file
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(HDF5_FILE, fileCAttr);
    
    int *buffer = new int[size.getScalarSize()];
    
    Dimensions size_read(0, 0, 0);
    dataCollector->read(10, "deep/folders/null", size_read, NULL);
    
    CPPUNIT_ASSERT(size_read == size);
    
    dataCollector->read(10, "deep/folders/null", size_read, buffer);
    
    dataCollector->read(10, "deep/folders/null_2", size_read, NULL);
    
    // empty datasets have size 1 in HDF5
    CPPUNIT_ASSERT(size_read == Dimensions(1, 1, 1));
    
    dataCollector->close();
    
    delete[] buffer;
}

