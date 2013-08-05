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



#ifndef PARALLEL_SIMPLEDATATEST_H
#define	PARALLEL_SIMPLEDATATEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "IParallelDataCollector.hpp"
#include "basetypes/ColTypeInt.hpp"

using namespace DCollector;

class Parallel_SimpleDataTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(Parallel_SimpleDataTest);

    CPPUNIT_TEST(testWriteRead);

    CPPUNIT_TEST_SUITE_END();

public:

    Parallel_SimpleDataTest();
    virtual ~Parallel_SimpleDataTest();

private:
    /**
     * Writes test data to a file and reads it in various ways, testing the
     * read data on conformacy.
     */
    void testWriteRead();

    /**
     * sub function for testWriteRead to allow several data/border sizes to be tested.
     */
    bool subtestWriteRead(uint32_t iteration, const Dimensions gridSize,
            const Dimensions borderSize, uint32_t dimensions);

    ColTypeInt ctInt;
    IParallelDataCollector *dataCollector;

    int totalMpiSize;
    int myMpiRank;
};

#endif	/* PARALLEL_SIMPLEDATATEST_H */

