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

#include "StridingTest.h"

#include <time.h>
#include <stdlib.h>
#include <math.h>

CPPUNIT_TEST_SUITE_REGISTRATION(StridingTest);

#define HDF5_FILE "h5/testStriding"

//#define TESTS_DEBUG

using namespace splash;

StridingTest::StridingTest() :
ctInt()
{
    dataCollector = new SerialDataCollector(10);
    srand(time(NULL));
}

StridingTest::~StridingTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

bool StridingTest::subtestStriding(Dimensions gridSize, Dimensions striding, uint32_t dimensions)
{
    bool resultsCorrect = true;

    Dimensions dstGridSize(ceil((float) gridSize[0] / (float) (striding[0])),
            ceil((float) gridSize[1] / (float) (striding[1])),
            ceil((float) gridSize[2] / (float) (striding[2])));

    size_t bufferSize = gridSize[0] * gridSize[1] * gridSize[2];

    size_t dstBufferSize = dstGridSize[0] * dstGridSize[1] * dstGridSize[2];

    // write data to file
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    dataCollector->open(HDF5_FILE, fileCAttr);

    // initial part of the test: data is written to the file, once with and once
    // without borders
    int *dataWrite = new int[bufferSize];

    for (uint32_t i = 0; i < bufferSize; i++)
        dataWrite[i] = i;

    dataCollector->write(0, ctInt, dimensions,
            Selection(gridSize,
            dstGridSize,
            Dimensions(0, 0, 0),
            striding),
            "data_strided", dataWrite);

    dataCollector->close();

    // first part of the test: read data with borders to a cleared
    // array (-1) and test results

    // read data from file
    fileCAttr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(HDF5_FILE, fileCAttr);

    int *dataRead = new int[dstBufferSize];
    for (uint32_t i = 0; i < dstBufferSize; i++)
        dataRead[i] = -1;

    Dimensions resultSize;
    dataCollector->read(0, "data_strided", resultSize, dataRead);

    dataCollector->close();

    // print out read and written data for debugging purposes
#if defined TESTS_DEBUG
    std::cout << "read:" << std::endl;
    for (size_t k = 0; k < dstGridSize[2]; k++)
    {
        std::cout << "k = " << k << std::endl;

        for (size_t j = 0; j < dstGridSize[1]; j++)
        {
            for (size_t i = 0; i < dstGridSize[0]; i++)
            {
                uint32_t index = k * (dstGridSize[0] * dstGridSize[1]) + j * dstGridSize[0] + i;
                std::cout << dataRead[index] << " ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "written:" << std::endl;
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

    for (uint32_t i = 0; i < dimensions; i++)
        CPPUNIT_ASSERT(resultSize[i] == dstGridSize[i]);

    for (size_t i = 0; i < dstGridSize[0]; i++)
    {
        for (size_t j = 0; j < dstGridSize[1]; j++)
        {
            for (size_t k = 0; k < dstGridSize[2]; k++)
            {
                int index_read = k * (dstGridSize[1] * dstGridSize[0]) +
                        j * dstGridSize[0] + i;
                int index_write = k * (striding[2]) * (gridSize[1] * gridSize[0]) +
                        j * (striding[1]) * gridSize[0] + i * (striding[0]);

                int value_read = dataRead[index_read];
                int check_value = dataWrite[index_write];

                if (value_read != check_value)
                {
#if defined TESTS_DEBUG
                    std::cout << "(i, j, k) == index == " << i << ", " << j << ", " << k << std::endl;
                    std::cout << value_read << " != exptected " << check_value << std::endl;
#endif
                    resultsCorrect = false;
                    break;
                }
            }
        }
    }

    delete[] dataRead;
    delete[] dataWrite;

    CPPUNIT_ASSERT_MESSAGE("strided write failed", resultsCorrect);

    return resultsCorrect;
}

void StridingTest::testStriding()
{
    uint32_t dimensions = 3;

    for (uint32_t k = 5; k < 8; k++)
        for (uint32_t j = 5; j < 8; j++)
            for (uint32_t i = 5; i < 8; i++)
            {
                Dimensions gridSize(i, j, k);
                Dimensions striding(rand() % 2 + 1, rand() % 2 + 1, rand() % 2 + 1);

                CPPUNIT_ASSERT(subtestStriding(gridSize, striding, dimensions));
            }
}

