/**
 * Copyright 2016 Alexander Grund
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
 
 #include <cppunit/extensions/HelperMacros.h>

#include "splash/splash.h"
#include <unistd.h>
#include <sys/stat.h>

using namespace splash;

class FilenameTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(FilenameTest);

    CPPUNIT_TEST(testBaseName);
    CPPUNIT_TEST(testFullName);

    CPPUNIT_TEST_SUITE_END();

public:

    FilenameTest();
    virtual ~FilenameTest();

private:
    /**
     * Tests that file naming is correct when passing only base name
     */
    void testBaseName();

    /**
     * Tests that file naming is correct when passing full name
     */
    void testFullName();

    void runTest(const char* filename, const char* fullFilename);

    bool fileExists(const std::string& filename);

    DataCollector *dataCollector;
    ColTypeInt ctInt;
};

CPPUNIT_TEST_SUITE_REGISTRATION(FilenameTest);

#define HDF5_FILE "h5/testFilename"
#define HDF5_BASE_FILE HDF5_FILE "_0_0_0.h5"
#define HDF5_FULL_FILE HDF5_FILE ".h5"

FilenameTest::FilenameTest()
{
    dataCollector = new SerialDataCollector(10);
    srand(time(NULL));
    // Remove old files
    unlink(HDF5_BASE_FILE);
    unlink(HDF5_FULL_FILE);
}

FilenameTest::~FilenameTest()
{
    delete dataCollector;
}

bool FilenameTest::fileExists(const std::string& filename)
{
    struct stat fileInfo;
    return (stat(filename.c_str(), &fileInfo) == 0);
}

void FilenameTest::testBaseName()
{
    // MPI rank should be appended
    runTest(HDF5_FILE, HDF5_BASE_FILE);
}

void FilenameTest::testFullName()
{
    // Filename should be used unchanged
    runTest(HDF5_FULL_FILE, HDF5_FULL_FILE);
}

void FilenameTest::runTest(const char* filename, const char* fullFilename)
{
    CPPUNIT_ASSERT(!fileExists(fullFilename));

    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);
    attr.fileAccType = DataCollector::FAT_WRITE;

    // write first dataset to file (create file)
    dataCollector->open(filename, attr);
    int data1 = rand();

    dataCollector->write(1, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data", &data1);
    dataCollector->close();
    // Now file must exist
    CPPUNIT_ASSERT(fileExists(fullFilename));

    // write second dataset to file (write to existing file of same name
    dataCollector->open(filename, attr);
    int data2 = rand();

    dataCollector->write(2, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data", &data2);
    dataCollector->close();


    // read data from file
    attr.fileAccType = DataCollector::FAT_READ;
    Dimensions data_size;

    int data = -1;
    dataCollector->open(filename, attr);

    CPPUNIT_ASSERT(dataCollector->getMaxID() == 2);

    dataCollector->read(1, "data", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data1);

    dataCollector->read(2, "data", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data2);

    dataCollector->close();

    // erase file
    attr.fileAccType = DataCollector::FAT_CREATE;
    dataCollector->open(filename, attr);

    CPPUNIT_ASSERT_THROW(dataCollector->read(1, "data", data_size, &data), DCException);
    int data3 = rand();
    dataCollector->write(2, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data", &data3);
    dataCollector->close();

    // Read from created file
    attr.fileAccType = DataCollector::FAT_READ;

    data = -1;
    dataCollector->open(filename, attr);

    CPPUNIT_ASSERT(dataCollector->getMaxID() == 2);

    dataCollector->read(2, "data", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data3);
    dataCollector->close();

    // Cleanup
    unlink(fullFilename);
}
