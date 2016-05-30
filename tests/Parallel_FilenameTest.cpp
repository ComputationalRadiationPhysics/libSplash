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
#include <mpi.h>
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

    IParallelDataCollector *dataCollector;
    ColTypeInt ctInt;
    int mpiRank;
    static int initCt;
};

int FilenameTest::initCt = 0;

CPPUNIT_TEST_SUITE_REGISTRATION(FilenameTest);

#define HDF5_FILE "h5/testParallelFilename"
#define HDF5_BASE_FILE HDF5_FILE "_2.h5"
#define HDF5_FULL_FILE HDF5_FILE "Full.h5"

#define MPI_SIZE_X 1
#define MPI_SIZE_Y 1

FilenameTest::FilenameTest()
{
    if(!initCt)
    {
        MPI_Init(NULL, NULL);
        srand(time(NULL));
        // Remove old files
        unlink(HDF5_BASE_FILE);
        unlink(HDF5_FULL_FILE);
    }
    initCt++;

    int mpiSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    CPPUNIT_ASSERT(mpiSize == (MPI_SIZE_X * MPI_SIZE_Y));

    dataCollector = new ParallelDataCollector(MPI_COMM_WORLD, MPI_INFO_NULL,
            Dimensions(MPI_SIZE_X, MPI_SIZE_Y, 1), 1);
}

FilenameTest::~FilenameTest()
{
    if (dataCollector)
    {
        dataCollector->finalize();
        delete dataCollector;
        dataCollector = NULL;
    }
    if(!--initCt)
        MPI_Finalize();
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
    attr.fileAccType = DataCollector::FAT_CREATE;

    // write first dataset to file (create file)
    dataCollector->open(filename, attr);
    int data1 = rand();

    dataCollector->write(2, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data1", &data1);
    dataCollector->close();

    // Now file must exist
    CPPUNIT_ASSERT(fileExists(fullFilename));

    // write second dataset to file (write to existing file of same name
    attr.fileAccType = DataCollector::FAT_WRITE;
    dataCollector->open(filename, attr);
    int data2 = rand();

    dataCollector->write(2, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data2", &data2);
    dataCollector->close();


    // read data from file
    attr.fileAccType = DataCollector::FAT_READ;
    Dimensions data_size;

    int data = -1;
    dataCollector->open(filename, attr);

    dataCollector->read(2, "data1", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data1);

    dataCollector->read(2, "data2", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data2);

    dataCollector->close();

    // erase file
    attr.fileAccType = DataCollector::FAT_CREATE;
    dataCollector->open(filename, attr);

    CPPUNIT_ASSERT_THROW(dataCollector->read(2, "data1", data_size, &data), DCException);
    int data3 = rand();
    dataCollector->write(2, ctInt, 1, Selection(Dimensions(1, 1, 1)), "data1", &data3);
    dataCollector->close();


    data = -1;
    // Read from created file
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(filename, attr);

    dataCollector->read(2, "data1", data_size, &data);

    CPPUNIT_ASSERT(data_size.getScalarSize() == 1);
    CPPUNIT_ASSERT(data == data3);
    dataCollector->close();
}
