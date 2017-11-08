#
# Copyright 2013 Felix Schmitt
#
# This file is part of libSplash.
#
# libSplash is free software: you can redistribute it and/or modify
# it under the terms of of either the GNU General Public License or
# the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libSplash is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License and the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# and the GNU Lesser General Public License along with libSplash.
# If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.3.0)

#-------------------------------------------------------------------------------

PROJECT(splash-tests)

#-------------------------------------------------------------------------------

SET(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/" "$ENV{MPI_ROOT}" "$ENV{HDF5_ROOT}")

SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS_DEBUG "-g")
OPTION(WITH_MPI "use MPI-based tests" OFF)

#-------------------------------------------------------------------------------

# HDF5 is required package
FIND_PACKAGE(HDF5 1.8.6 REQUIRED)
INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})

IF(WITH_MPI)
    # MPI is required package
    FIND_PACKAGE(MPI REQUIRED)
    INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
ENDIF(WITH_MPI)

# CPPUNIT is required package (not available yet)
#FIND_PACKAGE(CppUnit REQUIRED)
#INCLUDE_DIRECTORIES(${CPPUNIT_INCLUDE_DIR})

#-------------------------------------------------------------------------------

OPTION(TESTS_DEBUG "enable debug output for tests" OFF)
IF(TESTS_DEBUG)
    ADD_DEFINITIONS(-DTESTS_DEBUG)
ENDIF(TESTS_DEBUG)

INCLUDE_DIRECTORIES(include ../src/include $ENV{MPI_INC} )

#-------------------------------------------------------------------------------

FILE(GLOB SRCFILESOTHER "dependencies/*.cpp")
SET(TESTS Append Attributes FileAccess Filename References Remove SimpleData Striding)

IF(WITH_MPI)
    SET(TESTS ${TESTS} Benchmark Domains)
ENDIF(WITH_MPI)

OPTION(PARALLEL "enable tests for parallel libSplash" ${HDF5_IS_PARALLEL})
IF(PARALLEL)
    IF(NOT WITH_MPI)
        # MPI is required package
        FIND_PACKAGE(MPI REQUIRED)
        INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
    ENDIF(NOT WITH_MPI)

    SET(TESTS ${TESTS} Parallel_Attributes Parallel_Filename Parallel_Domains Parallel_ListFiles Parallel_References Parallel_Remove Parallel_SerialDC Parallel_SimpleData Parallel_ZeroAccess)
ELSE(PARALLEL)
    SET(TESTS ${TESTS})
ENDIF(PARALLEL)

FOREACH(TEST_NAME ${TESTS})
    SET(TEST_FILES "${TEST_FILES};${TEST_NAME}Test.cpp")
ENDFOREACH()

#-------------------------------------------------------------------------------

SET(SPLASH_RELEASE CACHE BOOL OFF)
SET(WITH_TOOLS CACHE BOOL OFF)
ADD_SUBDIRECTORY(.. "${CMAKE_CURRENT_BINARY_DIR}/build_libsplash")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/build_libsplash")

#-------------------------------------------------------------------------------

add_definitions(-Wno-deprecated-declarations)

SET(LIBS splash_static m cppunit ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES} ${HDF5_LIBRARIES})
IF(${vampir})
    SET(LIBS vt-hyb ${LIBS})
ENDIF(${vampir})

FOREACH(TEST_FILE ${TEST_FILES})
    GET_FILENAME_COMPONENT(FILE ${TEST_FILE} NAME)
    MESSAGE(STATUS ${FILE})
    ADD_EXECUTABLE(${FILE}.out
        ${TEST_FILE}
        ${SRCFILESOTHER}
    )
    ADD_DEPENDENCIES(${FILE}.out splash_static)

    TARGET_LINK_LIBRARIES(${FILE}.out ${LIBS})
ENDFOREACH()

# copy scripts to "build" dir for easy testing
FILE(COPY readBoolChar.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY run_tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
FILE(COPY run_parallel_tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
