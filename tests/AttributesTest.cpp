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
 


#include "AttributesTest.h"

#include <time.h>
#include <iostream>
#include <stdlib.h>
#include <cppunit/TestAssert.h>

CPPUNIT_TEST_SUITE_REGISTRATION(AttributesTest);

using namespace splash;

#define BUF_SIZE 32
#define TEST_FILE "h5/attributes"
#define TEST_FILE2 "h5/attributes_array"

AttributesTest::AttributesTest()
{
    srand(time(NULL));
    
    dataCollector = new SerialDataCollector(10);
}


AttributesTest::~AttributesTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

void AttributesTest::testDataAttributes()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);
    
    dataCollector->open(TEST_FILE, attr);
    
    int *dummy_data = new int[BUF_SIZE * 2];
    int sum = 0, sum2 = 0;
    for (int i = 0; i < BUF_SIZE; i++)
    {
        int val_x = rand() % 10;
        int val_y = rand() % 10;
        
        sum += val_x * val_y;
        
        dummy_data[2 * i] = val_x;
        dummy_data[2 * i + 1] = val_y;
    }
    sum2 = sum;
    
    dataCollector->writeAttribute(10, ctInt, NULL, "timestep", &sum2);
    
    dataCollector->write(0, ctInt2, 1, Dimensions(BUF_SIZE, 1, 1), 
            Dimensions(BUF_SIZE, 1, 1), Dimensions(0, 0, 0), "datasets/my_dataset", dummy_data);
    
    dataCollector->writeAttribute(0, ctInt, "datasets/my_dataset", "sum", &sum);
    int neg_sum = -sum;
    dataCollector->writeAttribute(0, ctInt, "datasets/my_dataset", "neg_sum", &neg_sum);
    
    delete[] dummy_data;
    dummy_data = NULL;
    
    dataCollector->close();
    
    dummy_data = new int[BUF_SIZE * 2];
    int old_sum = sum;
    sum = 0;
    sum2 = 0;
    for (int i = 0; i < BUF_SIZE * 2; i++)
        dummy_data[i] = 0;
    
    attr.fileAccType = DataCollector::FAT_READ;
    
    dataCollector->open(TEST_FILE, attr);
    
    dataCollector->readAttribute(10, NULL, "timestep", &sum2);
    
    Dimensions src_data;
    dataCollector->read(0, "datasets/my_dataset", src_data, dummy_data);
    
    for (int i = 0; i < BUF_SIZE; i++)
    {
        sum += dummy_data[2 * i] * dummy_data[2 * i + 1];
    }
    
    delete[] dummy_data;
    dummy_data = NULL;
    
    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(sum == sum2);
    
    sum = 0;
    neg_sum = 0;
    dataCollector->readAttribute(0, "datasets/my_dataset", "sum", &sum);
    dataCollector->readAttribute(0, "datasets/my_dataset", "neg_sum", &neg_sum);
    
    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(neg_sum == -old_sum);
    
    dataCollector->close();
}


void AttributesTest::testArrayTypes()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);
    
    const int array_data_write[3] = { 17, 12, -99 };
    Dimensions dim_write(104, 0, 2);
    
    dataCollector->open(TEST_FILE2, attr);
    dataCollector->writeGlobalAttribute(ctInt3Array, "testposition", array_data_write);
    dataCollector->writeGlobalAttribute(ctDimArray, "testdim", dim_write.getPointer());
    dataCollector->close();
    
    int array_data_read[3] = {0, 0, 0};
    Dimensions dim_read;
    
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE2, attr);
    dataCollector->readGlobalAttribute("testposition", array_data_read);
    dataCollector->readGlobalAttribute("testdim", dim_read.getPointer());
    dataCollector->close();
    
    for (int i = 0; i < 3; i++)
    {
        CPPUNIT_ASSERT(array_data_read[i] == array_data_write[i]);
        CPPUNIT_ASSERT(dim_write[i] == dim_read[i]);
    }
}
