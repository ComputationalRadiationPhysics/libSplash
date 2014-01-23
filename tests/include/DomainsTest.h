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
 


#ifndef DOMAINSTEST_H
#define	DOMAINSTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "splash/splash.h"

using namespace splash;

class DomainsTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(DomainsTest);

    CPPUNIT_TEST(testGridDomains);
    CPPUNIT_TEST(testPolyDomains);
    CPPUNIT_TEST(testAppendDomains);

    CPPUNIT_TEST_SUITE_END();

public:

    DomainsTest();
    virtual ~DomainsTest();

private:
    void testGridDomains();
    void subTestGridDomains(const Dimensions mpiSize, const Dimensions gridSize,
            uint32_t rank, uint32_t iteration);
    
    void testPolyDomains();
    void subTestPolyDomains(const Dimensions mpiSize, uint32_t numElements,
            uint32_t iteration);
    
    void testAppendDomains();
    
    int totalMpiSize;
    int totalMpiRank;

    ColTypeInt ctInt;
    ColTypeFloat ctFloat;
    DomainCollector *dataCollector;
};

#endif	/* DOMAINSTEST_H */

