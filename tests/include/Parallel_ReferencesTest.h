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
 


#ifndef PARALLEL_REFERENCESTEST_H
#define	PARALLEL_REFERENCESTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "splash/splash.h"

using namespace splash;

class Parallel_ReferencesTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(Parallel_ReferencesTest);

    CPPUNIT_TEST(testCreateReference);

    CPPUNIT_TEST_SUITE_END();

public:

    Parallel_ReferencesTest();
    virtual ~Parallel_ReferencesTest();

private:
    void testCreateReference();

    ColTypeInt ctInt;
    IParallelDataCollector *dataCollector;
    
    int mpiRank, mpiSize;
};

#endif	/* PARALLEL_REFERENCESTEST_H */

