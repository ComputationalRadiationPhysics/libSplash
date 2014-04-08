/**
 * Copyright 2014 Felix Schmitt
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


#ifndef PARALLEL_LISTFILESTEST_H
#define	PARALLEL_LISTFILESTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "splash/splash.h"

using namespace splash;

class Parallel_ListFilesTest  : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(Parallel_ListFilesTest);

    CPPUNIT_TEST(testListFiles);

    CPPUNIT_TEST_SUITE_END();
public:
    Parallel_ListFilesTest();
    virtual ~Parallel_ListFilesTest();
private:
    void testListFiles();
    
    ColTypeInt32 ctInt;
    int mpiRank;
};

#endif	/* PARALLEL_LISTFILESTEST_H */

