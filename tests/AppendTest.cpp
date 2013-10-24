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
 


#include "AppendTest.h"
#include <time.h>
#include <iostream>
#include <stdlib.h>
#include <cppunit/TestAssert.h>

CPPUNIT_TEST_SUITE_REGISTRATION(AppendTest);

using namespace splash;

#define MAX_VALUE 10000
#define TEST_FILE "h5/append"

AppendTest::AppendTest()
{
    srand(time(NULL));

    dataCollector = new SerialDataCollector(10);
}

AppendTest::~AppendTest()
{
    if (dataCollector != NULL)
        delete dataCollector;
}

void AppendTest::appendData(size_t count, float *data)
{
    //std::cout << "appending " << count << " elements" << std::endl;
    dataCollector->append(0, ctFloat, count, "data", data);
}

void AppendTest::fillData(size_t count, float* data)
{
    for (size_t i = 0; i < count; i++)
        data[i] = (float) (rand() % MAX_VALUE - (MAX_VALUE / 2)) / MAX_VALUE;
}

void AppendTest::writeFile(size_t dataCount, float* data)
{
    size_t *counts = new size_t[dataCount];
    size_t c = 0;
    size_t total_counts = 0;

    while (total_counts < dataCount)
    {
        size_t count = rand() % (dataCount - total_counts) + 1;
        counts[c] = count;
        total_counts += count;
        c++;
    }

    CPPUNIT_ASSERT(total_counts == dataCount);

    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);
    
    dataCollector->open(TEST_FILE, attr);

    total_counts = 0;
    for (size_t i = 0; i < c; i++)
    {
        appendData(counts[i], data + total_counts);
        total_counts += counts[i];
    }

    dataCollector->append(0, ctFloat, 0, "data", data);

    CPPUNIT_ASSERT(total_counts == dataCount);

    dataCollector->close();
    delete[] counts;
}

void AppendTest::testAppend()
{
    for (size_t data_count = 256; data_count < 2048; data_count += 256)
    {
        float *data = new float[data_count];
        fillData(data_count, data);

        writeFile(data_count, data);

        DataCollector::FileCreationAttr attr;

        DataCollector::initFileCreationAttr(attr);
        attr.enableCompression = true;
        attr.fileAccType = DataCollector::FAT_READ;
        dataCollector->open(TEST_FILE, attr);

        float *testData = new float[data_count];

        Dimensions testDim(1, 1, 1);
        dataCollector->read(0, "data", testDim, testData);

        dataCollector->close();

        CPPUNIT_ASSERT(testDim[0] == data_count && testDim[1] == 1 && testDim[2] == 1);
       

        for (size_t i = 0; i < data_count; i++)
            CPPUNIT_ASSERT(testData[i] == data[i]);

        delete[] testData;
        delete[] data;
    }
}

