/**
 * Copyright 2013-2015 Felix Schmitt, Axel Huebl
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

#include "Parallel_AttributesTest.h"

#include <mpi.h>
#include <time.h>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <cppunit/TestAssert.h>

namespace splash {

// Declare this in namespace splash (required for ADL)
std::ostream& operator<<(std::ostream& s, const Dimensions& dim)
{
    return s << dim.toString();
}

}  // namespace splash

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_AttributesTest);

using namespace splash;

#define BUF_SIZE 32
#define TEST_FILE "h5/attributesParallel"
#define TEST_FILE_META "h5/attributesMetaParallel"
#define TEST_FILE2 "h5/attributes_arrayParallel"
#define TEST_FILE_ARRAYMETA "h5/attributes_arrayMetaParallel"

#define MPI_SIZE_X 2
#define MPI_SIZE_Y 2

Parallel_AttributesTest::Parallel_AttributesTest() :
  ctString4(4)
{
    srand(time(NULL));

    int initialized;
    MPI_Initialized(&initialized);
    if( !initialized )
        MPI_Init(NULL, NULL);

    int mpiSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

    CPPUNIT_ASSERT(mpiSize == (MPI_SIZE_X * MPI_SIZE_Y));
}

Parallel_AttributesTest::~Parallel_AttributesTest()
{
    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void Parallel_AttributesTest::testDataAttributes()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    IParallelDataCollector *dataCollector = new ParallelDataCollector(
            MPI_COMM_WORLD, MPI_INFO_NULL, Dimensions(MPI_SIZE_X, MPI_SIZE_Y, 1), 1);
    dataCollector->open(TEST_FILE, attr);

    int *dummy_data = new int[BUF_SIZE * 2];
    int sum = 0;
    for (int i = 0; i < BUF_SIZE; i++)
    {
        int val_x = rand() % 10;
        int val_y = rand() % 10;

        sum += val_x * val_y;

        dummy_data[2 * i] = val_x;
        dummy_data[2 * i + 1] = val_y;
    }

    int groupNotExistsTestValue = 42;
    /* check if it is possible to add an attribute to a not existing group */
    dataCollector->writeAttribute(0, ctInt, "notExistingGroup/", "magic_number", &groupNotExistsTestValue);

    dataCollector->write(0, ctInt2, 1, Selection(Dimensions(BUF_SIZE, 1, 1)),
            "attr/attr2/attr3/data", dummy_data);

    dataCollector->writeAttribute(0, ctInt, "attr/attr2/attr3/data", "sum", &sum);
    int neg_sum = -sum;
    dataCollector->writeAttribute(0, ctInt, "attr/attr2/attr3/data", "neg_sum", &neg_sum);

    dataCollector->writeAttribute(0, ctInt, "attr/attr2/attr3", "sum_at_group", &sum);
    char c = 'Y';
    dataCollector->writeAttribute(0, ctChar, "attr/attr2/attr3", "my_char", &c);

    /* variable length string, '\0' terminated */
    const char *string_attr = {"My first c-string."};
    dataCollector->writeAttribute(0, ctString, NULL, "my_string", &string_attr);
    /* fixed length string, '\0' terminated */
    const char string_attr4[5] = {"ABCD"};
    dataCollector->writeAttribute(0, ctString4, NULL, "my_string4", &string_attr4);

    delete[] dummy_data;
    dummy_data = NULL;

    dataCollector->close();

    dummy_data = new int[BUF_SIZE * 2];
    int old_sum = sum;
    sum = 0;
    for (int i = 0; i < BUF_SIZE * 2; i++)
        dummy_data[i] = 0;

    attr.fileAccType = DataCollector::FAT_READ;

    dataCollector->open(TEST_FILE, attr);

    int readGroupNotExistsTestValue = 0;
    dataCollector->readAttribute(0, "notExistingGroup/", "magic_number", &readGroupNotExistsTestValue);
    CPPUNIT_ASSERT(groupNotExistsTestValue == readGroupNotExistsTestValue);
    
    sum = 0;
    neg_sum = 0;
    c = 'A';
    dataCollector->readAttribute(0, "attr/attr2/attr3/data", "sum", &sum);
    dataCollector->readAttribute(0, "attr/attr2/attr3/data", "neg_sum", &neg_sum);

    CPPUNIT_ASSERT_THROW(dataCollector->readAttribute(0, "data", "sum", &sum), DCException);

    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(neg_sum == -old_sum);

    dataCollector->readAttribute(0, "attr/attr2/attr3", "sum_at_group", &sum);
    dataCollector->readAttribute(0, "attr/attr2/attr3", "my_char", &c);

    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(c == 'Y');

    char* string_read;
    dataCollector->readAttribute(0, NULL, "my_string", &string_read);
    char string_read4[5];
    dataCollector->readAttribute(0, NULL, "my_string4", &string_read4);

    CPPUNIT_ASSERT(strcmp(string_read, string_attr) == 0);
    CPPUNIT_ASSERT(strcmp(string_read4, string_attr4) == 0);

    dataCollector->close();
    dataCollector->finalize();
    delete dataCollector;
}

void Parallel_AttributesTest::testAttributesMeta()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    IParallelDataCollector* dataCollector = new ParallelDataCollector(
            MPI_COMM_WORLD, MPI_INFO_NULL, Dimensions(MPI_SIZE_X, MPI_SIZE_Y, 1), 1);

    dataCollector->open(TEST_FILE_META, attr);

    const int intValGlob = rand();
    const int intVal = rand();
    dataCollector->writeGlobalAttribute(10, ctInt, "intValGlob", &intValGlob);
    dataCollector->writeAttribute(10, ctInt, NULL, "intVal", &intVal);

    /* variable length string, '\0' terminated */
    const char *varLenStr = "My first c-string.";
    dataCollector->writeAttribute(10, ctString, NULL, "varLenStr", &varLenStr);
    /* fixed length string, '\0' terminated */
    const char fixedLenStr[5] = {"ABCD"};
    dataCollector->writeAttribute(10, ctString4, NULL, "fixedLenStr", &fixedLenStr);

    // Create a group
    dataCollector->write(10, ctInt, 1, Selection(Dimensions()), "group", &intVal);

    const int intValGroup = rand();
    const char charVal = 'Y';
    dataCollector->writeAttribute(10, ctInt, "group", "intValGroup", &intValGroup);
    dataCollector->writeAttribute(10, ctChar, "group", "charVal", &charVal);

    // Reopen in read mode
    dataCollector->close();
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE_META, attr);

    AttributeInfo info = dataCollector->readGlobalAttributeInfo(10, "intValGlob");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());
    int intValRead = 0;
    info->read(ctInt, &intValRead);
    CPPUNIT_ASSERT_EQUAL(intValGlob, intValRead);

    info = dataCollector->readAttributeInfo(10, NULL, "intVal");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());
    intValRead = 0;
    info->read(ctInt, &intValRead);
    CPPUNIT_ASSERT_EQUAL(intVal, intValRead);

    info = dataCollector->readAttributeInfo(10, NULL, "varLenStr");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(varLenStr), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctString));
    CPPUNIT_ASSERT(info->isVarSize());
    // Note: API returns a pointer to the string!
    const char* varLenStrRead = NULL;
    info->read(ctString, &varLenStrRead);
    CPPUNIT_ASSERT(strcmp(varLenStr, varLenStrRead) == 0);

    info = dataCollector->readAttributeInfo(10, NULL, "fixedLenStr");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(fixedLenStr), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctString4));
    CPPUNIT_ASSERT(!info->isVarSize());
    // Note: API returns string data
    std::vector<char> fixedLenStrRead(info->getMemSize());
    info->read(ctString4, &fixedLenStrRead[0]);
    CPPUNIT_ASSERT(strcmp(fixedLenStr, &fixedLenStrRead[0]) == 0);

    info = dataCollector->readAttributeInfo(10, "group", "intValGroup");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());
    int intValGroupRead = 0;
    info->read(ctInt, &intValGroupRead);
    CPPUNIT_ASSERT_EQUAL(intValGroup, intValGroupRead);

    info = dataCollector->readAttributeInfo(10, "group", "charVal");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(charVal), info->getMemSize());
    // Note: Native char resolves to either Int8 or UInt8
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt8) || typeid(info->getType()) == typeid(ColTypeUInt8));
    CPPUNIT_ASSERT(!info->isVarSize());
    char charValRead = 0;
    info->read(ctChar, &charValRead);
    CPPUNIT_ASSERT_EQUAL(charVal, charValRead);

    delete info;
    dataCollector->close();
    dataCollector->finalize();
    delete dataCollector;
}

void Parallel_AttributesTest::testArrayTypes()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    const int array_data_write[3] = {17, 12, -99};
    Dimensions dim_write(104, 0, 2);

    IParallelDataCollector *dataCollector = new ParallelDataCollector(
            MPI_COMM_WORLD, MPI_INFO_NULL, Dimensions(MPI_SIZE_X, MPI_SIZE_Y, 1), 1);

    dataCollector->open(TEST_FILE2, attr);
    dataCollector->writeGlobalAttribute(10, ctInt3Array, "testpositionArray", array_data_write);
    dataCollector->writeGlobalAttribute(10, ctInt, "testposition", 1u, Dimensions(3, 1, 1), array_data_write);
    dataCollector->writeGlobalAttribute(20, ctDimArray, "testdim", dim_write.getPointer());
    dataCollector->close();

    int array_data_read[3] = {0, 0, 0};
    int data_read[3] = {0, 0, 0};
    Dimensions dim_read;

    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE2, attr);
    dataCollector->readGlobalAttribute(10, "testpositionArray", array_data_read);
    dataCollector->readGlobalAttribute(10, "testposition", data_read);
    dataCollector->readGlobalAttribute(20, "testdim", dim_read.getPointer());
    dataCollector->close();

    for (int i = 0; i < 3; i++)
    {
        CPPUNIT_ASSERT(array_data_read[i] == array_data_write[i]);
        CPPUNIT_ASSERT(data_read[i] == array_data_write[i]);
        CPPUNIT_ASSERT(dim_write[i] == dim_read[i]);
    }

    dataCollector->close();
    dataCollector->finalize();
    delete dataCollector;
}

void Parallel_AttributesTest::testArrayAttributesMeta()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    IParallelDataCollector* dataCollector = new ParallelDataCollector(
            MPI_COMM_WORLD, MPI_INFO_NULL, Dimensions(MPI_SIZE_X, MPI_SIZE_Y, 1), 1);

    dataCollector->open(TEST_FILE_ARRAYMETA, attr);

    const int intVal  = rand();
    const int intVal2 = rand();
    const int int3Array[3] = { rand(), rand(), -rand() };
    const int int3Array2[3] = { rand(), rand(), -rand() };
    const int int3Array3[3] = { rand(), rand(), -rand() };
    const double doubleArray[7] = {-(rand() * 10.)/RAND_MAX, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    const Dimensions dim(rand(), rand(), rand());
    const char strArray[6] = {"x\0y\0z"};

    dataCollector->writeAttribute(10, ctInt, NULL, "intVal", 1u, Dimensions(1, 1, 1), &intVal);
    dataCollector->writeAttribute(10, ctInt, NULL, "intVal2", 3u, Dimensions(1, 1, 1), &intVal2);
    dataCollector->writeAttribute(10, ctInt3Array, NULL, "int3Array1", int3Array);
    dataCollector->writeAttribute(10, ctInt, NULL, "int3Array2", 1u, Dimensions(3, 1, 1), int3Array2);
    dataCollector->writeAttribute(10, ctInt, NULL, "int3Array3", 3u, Dimensions(1, 1, 3), int3Array3);
    dataCollector->writeAttribute(10, ctDimArray, NULL, "dim", dim.getPointer());
    dataCollector->writeAttribute(10, ctDouble, NULL, "doubleArray", 1u, Dimensions(7,1,1), doubleArray);
    dataCollector->writeAttribute(10, ColTypeString(1), NULL, "strArray", 1u, Dimensions(3,1,1), strArray);

    // Reopen in read mode
    dataCollector->close();
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE_ARRAYMETA, attr);

    int intValRead  = 0;
    int int3ArrayRead[3] = { 0,0,0 };
    double doubleArrayRead[7] = {0, 0, 0, 0, 0, 0, 0};
    Dimensions dimRead(0,0,0);
    char strArrayRead[6] = "\0\0\0\0\0";

    AttributeInfo info = dataCollector->readAttributeInfo(10, NULL, "intVal");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&intValRead, sizeof(intValRead));
    CPPUNIT_ASSERT_EQUAL(intVal, intValRead);

    info = dataCollector->readAttributeInfo(10, NULL, "intVal2");
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(3u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&intValRead, sizeof(intValRead));
    CPPUNIT_ASSERT_EQUAL(intVal2, intValRead);

    info = dataCollector->readAttributeInfo(10, NULL, "int3Array1");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctInt3Array));
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&int3ArrayRead, sizeof(int3ArrayRead));
    CPPUNIT_ASSERT(memcmp(int3Array, int3ArrayRead, sizeof(int3ArrayRead)) == 0);

    info = dataCollector->readAttributeInfo(10, NULL, "int3Array2");
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(3, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&int3ArrayRead, sizeof(int3ArrayRead));
    CPPUNIT_ASSERT(memcmp(int3Array2, int3ArrayRead, sizeof(int3ArrayRead)) == 0);

    info = dataCollector->readAttributeInfo(10, NULL, "int3Array3");
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(3u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 3), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&int3ArrayRead, sizeof(int3ArrayRead));
    CPPUNIT_ASSERT(memcmp(int3Array3, int3ArrayRead, sizeof(int3ArrayRead)) == 0);

    info = dataCollector->readAttributeInfo(10, NULL, "dim");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(dim), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctDimArray));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&dimRead, dimRead.getSize());
    CPPUNIT_ASSERT_EQUAL(dim, dimRead);

    info = dataCollector->readAttributeInfo(10, NULL, "doubleArray");
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(doubleArray), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctDouble));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(7, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&doubleArrayRead, sizeof(doubleArrayRead));
    CPPUNIT_ASSERT(memcmp(doubleArray, doubleArrayRead, sizeof(doubleArrayRead)) == 0);

    info = dataCollector->readAttributeInfo(10, NULL, "strArray");
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(strArray), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeString));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(3, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());
    info->read(&strArrayRead, sizeof(strArrayRead));
    CPPUNIT_ASSERT(memcmp(strArray, strArrayRead, sizeof(strArrayRead)) == 0);

    delete info;
    dataCollector->close();
    dataCollector->finalize();
    delete dataCollector;
}
