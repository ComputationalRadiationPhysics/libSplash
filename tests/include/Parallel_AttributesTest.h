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


#ifndef PARALLEL_ATTRIBUTESTEST_H
#define	PARALLEL_ATTRIBUTESTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "IParallelDataCollector.hpp"
#include "basetypes/ColTypeInt.hpp"
#include "basetypes/ColTypeInt2.hpp"
#include "basetypes/ColTypeInt3Array.hpp"
#include "basetypes/ColTypeDimArray.hpp"

using namespace DCollector;

class Parallel_AttributesTest  : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(Parallel_AttributesTest);

    CPPUNIT_TEST(testDataAttributes);
    CPPUNIT_TEST(testArrayTypes);

    CPPUNIT_TEST_SUITE_END();
public:
    Parallel_AttributesTest();
    virtual ~Parallel_AttributesTest();
private:
    void testDataAttributes();
    void testArrayTypes();
    
    ColTypeInt ctInt;
    ColTypeInt2 ctInt2;
    ColTypeInt3Array ctInt3Array;
    ColTypeDimArray ctDimArray;
    
    int mpiRank;
};

#endif	/* PARALLEL_ATTRIBUTESTEST_H */

