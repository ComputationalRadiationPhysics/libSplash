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
 


#ifndef FILEACCESSTEST_H
#define	FILEACCESSTEST_H

#include <cppunit/extensions/HelperMacros.h>

#include "splash.h"

using namespace DCollector;

class FileAccessTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(FileAccessTest);

    CPPUNIT_TEST(testWriteAfterCreate);

    CPPUNIT_TEST_SUITE_END();

public:
    FileAccessTest();
    virtual ~FileAccessTest();
private:
    void testWriteAfterCreate();
    
    ColTypeInt ctInt;
    DataCollector *dataCollector;
};

#endif	/* FILEACCESSTEST_H */

