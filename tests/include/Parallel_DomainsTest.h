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



#ifndef PARALLEL_DOMAINSTEST_H
#define	PARALLEL_DOMAINSTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "splash.h"

using namespace DCollector;

class Parallel_DomainsTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(Parallel_DomainsTest);

    CPPUNIT_TEST(testGridDomains);
    CPPUNIT_TEST(testPolyDomains);
    CPPUNIT_TEST(testAppendDomains);

    CPPUNIT_TEST_SUITE_END();

public:

    Parallel_DomainsTest();
    virtual ~Parallel_DomainsTest();

private:
    void testGridDomains();
    void testPolyDomains();
    void testAppendDomains();
    
    void subTestGridDomains(int32_t iteration,
            int currentMpiRank,
            const Dimensions mpiSize, const Dimensions mpiPos,
            const Dimensions gridSize, uint32_t dimensions, MPI_Comm mpiComm);
    
    void subTestPolyDomains(int32_t iteration,
            int currentMpiRank,
            const Dimensions mpiSize, const Dimensions mpiPos,
            const uint32_t numElements, uint32_t dimensions, MPI_Comm mpiComm);

    int totalMpiSize;
    int myMpiRank;

    ColTypeInt ctInt;
    ColTypeFloat ctFloat;
    ParallelDomainCollector *parallelDomainCollector;
};

#endif	/* PARALLEL_DOMAINSTEST_H */

