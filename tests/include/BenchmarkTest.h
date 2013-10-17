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
 


#ifndef BENCHMARKTEST_H
#define	BENCHMARKTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <mpi.h>

#include "splash.h"

using namespace splash;

class BenchmarkTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(BenchmarkTest);

    CPPUNIT_TEST(testBenchmark);

    CPPUNIT_TEST_SUITE_END();
public:

    BenchmarkTest();
    virtual ~BenchmarkTest();
private:
    void testBenchmark();
    void runBenchmark(uint32_t cx, uint32_t cy, uint32_t cz,
            Dimensions gridSize, void* data);
    void runComparison(uint32_t cx, uint32_t cy, uint32_t cz,
        Dimensions gridSize, void* data);

    int mpi_size, mpi_rank;
    MPI_Comm comm;
    ColTypeInt ctInt;
    DataCollector *dataCollector;
};

#endif	/* BENCHMARKTEST_H */

