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
 


#ifndef MERGEDREADTEST_H
#define	MERGEDREADTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <mpi.h>

#include "splash.h"

using namespace DCollector;

class MergedReadTest: public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(MergedReadTest);

    CPPUNIT_TEST(testMergedRead);
    CPPUNIT_TEST(testAppendMergedRead);

    CPPUNIT_TEST_SUITE_END();
    
public:
    MergedReadTest();
    virtual ~MergedReadTest();
private:
    bool subTestMergedRead(uint32_t dimensions, Dimensions gridSize, Dimensions mpiSize);
    void testMergedRead();
    void testAppendMergedRead();

    void appendData(Dimensions mpiPosition, Dimensions mpiSize);

    void writeFiles(const char *file, int dimensions,
        Dimensions &gridSize, Dimensions &mpiSize);

    int mpi_size, mpi_rank;
    MPI_Comm comm;
    ColTypeInt ctInt;
    DataCollector *dataCollector;
};

#endif	/* MERGEDREADTEST_H */

