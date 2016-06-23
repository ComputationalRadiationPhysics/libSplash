/**
 * Copyright 2013-2016 Felix Schmitt, Axel Huebl, Alexander Grund
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

#include "AttributesTest.h"
#include "splash/DataCollector.hpp"

#include <time.h>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <cppunit/TestAssert.h>

std::ostream& operator<<(std::ostream& s, const splash::Dimensions& dim)
{
    return s << dim.toString();
}

CPPUNIT_TEST_SUITE_REGISTRATION(AttributesTest);

using namespace splash;

#define BUF_SIZE 32
#define TEST_FILE "h5/attributes"
#define TEST_FILE_META "h5/attributes_meta"
#define TEST_FILE2 "h5/attributes_array"
#define TEST_FILE_ARRAYMETA "h5/attributes_arraymeta"

AttributesTest::AttributesTest() :
ctString4(4)
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

    dataCollector->writeAttribute(10, ctInt, NULL, "iteration", &sum2);

    int groupNotExistsTestValue = 42;
    /* check if it is possible to add an attribute to a not existing group */
    dataCollector->writeAttribute(0, ctInt, "notExistingGroup/", "magic_number", &groupNotExistsTestValue);

    /* variable length string, '\0' terminated */
    const char *string_attr = {"My first c-string."};
    dataCollector->writeAttribute(10, ctString, NULL, "my_string", &string_attr);
    /* fixed length string, '\0' terminated */
    const char string_attr4[5] = {"ABCD"};
    dataCollector->writeAttribute(10, ctString4, NULL, "my_string4", &string_attr4);

    CPPUNIT_ASSERT_THROW(dataCollector->writeAttribute(10, ctInt, NULL, NULL, &sum2),
            DCException);
    CPPUNIT_ASSERT_THROW(dataCollector->writeAttribute(10, ctInt, NULL, "", &sum2),
            DCException);
    CPPUNIT_ASSERT_THROW(dataCollector->writeAttribute(10, ctInt, "", "", &sum2),
            DCException);

    dataCollector->write(0, ctInt2, 1, Selection(Dimensions(BUF_SIZE, 1, 1)),
            "datasets/my_dataset", dummy_data);

    dataCollector->writeAttribute(0, ctInt, "datasets/my_dataset", "sum", &sum);
    int neg_sum = -sum;
    dataCollector->writeAttribute(0, ctInt, "datasets/my_dataset", "neg_sum", &neg_sum);

    char c = 'Y';
    double d[7] = {-3.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    dataCollector->writeAttribute(0, ctInt, "datasets", "sum_at_group", &sum);
    dataCollector->writeAttribute(0, ctChar, "datasets", "my_char", &c);
    dataCollector->writeAttribute(0, ctDouble, "datasets", "unitDimension", 1u, Dimensions(7,0,0), d);

    CPPUNIT_ASSERT_THROW(dataCollector->writeAttribute(0, ctDouble, "datasets",
            "invalnDims", 4u, Dimensions(7,0,0), d), DCException);

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

    int readGroupNotExistsTestValue = 0;
    dataCollector->readAttribute(0, "notExistingGroup/", "magic_number", &readGroupNotExistsTestValue);
    CPPUNIT_ASSERT(groupNotExistsTestValue == readGroupNotExistsTestValue);

    dataCollector->readAttribute(10, NULL, "iteration", &sum2);

    char* string_read;
    dataCollector->readAttribute(10, NULL, "my_string", &string_read);
    char string_read4[5];
    dataCollector->readAttribute(10, NULL, "my_string4", &string_read4);

    CPPUNIT_ASSERT(strcmp(string_read, string_attr) == 0);
    CPPUNIT_ASSERT(strcmp(string_read4, string_attr4) == 0);

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
    c = 'A';
    dataCollector->readAttribute(0, "datasets/my_dataset", "sum", &sum);
    dataCollector->readAttribute(0, "datasets/my_dataset", "neg_sum", &neg_sum);

    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(neg_sum == -old_sum);

    double dr[7] = {0., 0., 0., 0., 0., 0., 0.};
    dataCollector->readAttribute(0, "datasets", "sum_at_group", &sum);
    dataCollector->readAttribute(0, "datasets", "my_char", &c);
    dataCollector->readAttribute(0, "datasets", "unitDimension", dr);

    CPPUNIT_ASSERT(sum == old_sum);
    CPPUNIT_ASSERT(c == 'Y');
    for (int i = 0; i < 7; i++)
        CPPUNIT_ASSERT(dr[i] == d[i]);

    dataCollector->close();
}

// Delete old info, read new info and check validity (reduce noise in test code)
#define READ_ATTR_META(name, group) delete info; info = NULL;                           \
                             info = dataCollector->readAttributeMeta(10, group, #name); \
                             CPPUNIT_ASSERT(info);

void AttributesTest::testAttributesMeta()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    dataCollector->open(TEST_FILE_META, attr);

    int intVal = rand();
    dataCollector->writeGlobalAttribute(ctInt, "intVal", &intVal);
    dataCollector->writeAttribute(10, ctInt, NULL, "intVal", &intVal);

    /* variable length string, '\0' terminated */
    const char *varLenStr = "My first c-string.";
    dataCollector->writeAttribute(10, ctString, NULL, "varLenStr", &varLenStr);
    /* fixed length string, '\0' terminated */
    const char fixedLenStr[5] = {"ABCD"};
    dataCollector->writeAttribute(10, ctString4, NULL, "fixedLenStr", &fixedLenStr);

    // Create a group
    dataCollector->write(10, ctInt, 1, Selection(Dimensions()), "group", &intVal);

    char charVal = 'Y';
    dataCollector->writeAttribute(10, ctInt, "group", "intVal", &intVal);
    dataCollector->writeAttribute(10, ctChar, "group", "charVal", &charVal);

    // Reopen in read mode
    dataCollector->close();
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE_META, attr);

    DCAttributeInfo* info = NULL;
    info = dataCollector->readGlobalAttributeMeta(10, "intVal");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(intVal, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(varLenStr, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(varLenStr), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctString));
    CPPUNIT_ASSERT(info->isVarSize());

    READ_ATTR_META(fixedLenStr, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(fixedLenStr), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctString4));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(intVal, "group");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(charVal, "group");
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(charVal), info->getMemSize());
    // Note: Native char resolves to either Int8 or UInt8
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt8) || typeid(info->getType()) == typeid(ColTypeUInt8));
    CPPUNIT_ASSERT(!info->isVarSize());

    delete info;
    dataCollector->close();
}


void AttributesTest::testArrayTypes()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    const int array_data_write[3] = { 17, 12, -99 };
    Dimensions dim_write(104, 0, 2);

    dataCollector->open(TEST_FILE2, attr);
    dataCollector->writeGlobalAttribute(ctInt3Array, "testpositionArray", array_data_write);
    dataCollector->writeGlobalAttribute(ctInt, "testposition", 1u, Dimensions(3, 1, 1), array_data_write);
    dataCollector->writeGlobalAttribute(ctDimArray, "testdim", dim_write.getPointer());
    dataCollector->close();

    int array_data_read[3] = {0, 0, 0};
    int data_read[3] = {0, 0, 0};
    Dimensions dim_read;

    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE2, attr);
    dataCollector->readGlobalAttribute("testpositionArray", array_data_read);
    dataCollector->readGlobalAttribute("testposition", data_read);
    dataCollector->readGlobalAttribute("testdim", dim_read.getPointer());
    dataCollector->close();

    for (int i = 0; i < 3; i++)
    {
        CPPUNIT_ASSERT(array_data_read[i] == array_data_write[i]);
        CPPUNIT_ASSERT(data_read[i] == array_data_write[i]);
        CPPUNIT_ASSERT(dim_write[i] == dim_read[i]);
    }
}

void AttributesTest::testArrayAttributesMeta()
{
    DataCollector::FileCreationAttr attr;
    DataCollector::initFileCreationAttr(attr);

    dataCollector->open(TEST_FILE_ARRAYMETA, attr);

    const int int3Array[3] = { 17, 12, -99 };
    double doubleArray[7] = {-3.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    Dimensions dim(104, 0, 2);
    const int intVal= 42;
    const char strArray[6] = {"x\0y\0z"};

    dataCollector->writeAttribute(10, ctInt, NULL, "intVal", 1u, Dimensions(1, 1, 1), &intVal);
    dataCollector->writeAttribute(10, ctInt, NULL, "intVal2", 3u, Dimensions(1, 1, 1), &intVal);
    dataCollector->writeAttribute(10, ctInt3Array, NULL, "int3Array1", int3Array);
    dataCollector->writeAttribute(10, ctInt, NULL, "int3Array2", 1u, Dimensions(3, 1, 1), int3Array);
    dataCollector->writeAttribute(10, ctInt, NULL, "int3Array3", 3u, Dimensions(1, 1, 3), int3Array);
    dataCollector->writeAttribute(10, ctDimArray, NULL, "dim", dim.getPointer());
    dataCollector->writeAttribute(10, ctDouble, NULL, "doubleArray", 1u, Dimensions(7,1,1), doubleArray);
    dataCollector->writeAttribute(10, ColTypeString(1), NULL, "strArray", 1u, Dimensions(3,1,1), strArray);

    // Reopen in read mode
    dataCollector->close();
    attr.fileAccType = DataCollector::FAT_READ;
    dataCollector->open(TEST_FILE_ARRAYMETA, attr);

    DCAttributeInfo* info = NULL;

    READ_ATTR_META(intVal, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(intVal2, NULL);
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(intVal), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(3u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(int3Array1, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctInt3Array));
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(int3Array2, NULL);
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(3, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(int3Array3, NULL);
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(int3Array), info->getMemSize());
    // Note: This is the file saved type. ColTypeInt will resolve to the generic variant
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeInt32));
    CPPUNIT_ASSERT_EQUAL(3u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 3), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(dim, NULL);
    CPPUNIT_ASSERT(info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(dim), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctDimArray));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(1, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(doubleArray, NULL);
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(doubleArray), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ctDouble));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(7, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    READ_ATTR_META(strArray, NULL);
    CPPUNIT_ASSERT(!info->isScalar());
    CPPUNIT_ASSERT_EQUAL(sizeof(strArray), info->getMemSize());
    CPPUNIT_ASSERT(typeid(info->getType()) == typeid(ColTypeString));
    CPPUNIT_ASSERT_EQUAL(1u, info->getNDims());
    CPPUNIT_ASSERT_EQUAL(Dimensions(3, 1, 1), info->getDims());
    CPPUNIT_ASSERT(!info->isVarSize());

    delete info;
    dataCollector->close();
}
