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
 


#include "FileAccessTest.h"

#include "SerialDataCollector.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(FileAccessTest);

#define HDF5_FILE "h5/testWriteAfterCreate"

using namespace DCollector;

FileAccessTest::FileAccessTest() :
ctInt()
{
    dataCollector = new SerialDataCollector(10);
}

FileAccessTest::~FileAccessTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }
}

void FileAccessTest::testWriteAfterCreate()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);
    attr.fileAccType = DataCollector::FAT_WRITE;
    
    // write first dataset to file (create file)
    dataCollector->open(HDF5_FILE, attr);
    int data = 1;
    
    dataCollector->write(1, ctInt, 1, Dimensions(1, 1, 1), Dimensions(1, 1, 1), 
            Dimensions(0, 0, 0), "data", &data);
    dataCollector->close();

    // write second dataset to file (write to existing file)
    dataCollector->open(HDF5_FILE, attr);
    data = 2;
    
    dataCollector->write(2, ctInt, 1, Dimensions(1, 1, 1), Dimensions(1, 1, 1), 
            Dimensions(0, 0, 0), "data", &data);
    dataCollector->close();
    
    
    // read data from file
    attr.fileAccType = DataCollector::FAT_READ;
    Dimensions data_size;
    
    data = -1;
    dataCollector->open(HDF5_FILE, attr);
    
    CPPUNIT_ASSERT(dataCollector->getMaxID() == 2);
    
    dataCollector->read(1, ctInt, "data", data_size, &data);
    
    CPPUNIT_ASSERT(data_size.getDimSize() == 1);
    CPPUNIT_ASSERT(data == 1);
    
    dataCollector->read(2, ctInt, "data", data_size, &data);
    
    CPPUNIT_ASSERT(data_size.getDimSize() == 1);
    CPPUNIT_ASSERT(data == 2);
    
    dataCollector->close();
    
    // erase file
    attr.fileAccType = DataCollector::FAT_CREATE;
    dataCollector->open(HDF5_FILE, attr);
    
    CPPUNIT_ASSERT_THROW(dataCollector->read(1, ctInt, "data", data_size, &data),
            DCException);
    
    dataCollector->close();
}

