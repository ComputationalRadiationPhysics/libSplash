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
 # libSplash is distributed in the hope that it will be useful, 
 # but WITHOUT ANY WARRANTY; without even the implied warranty of 
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 # GNU General Public License and the GNU Lesser General Public License 
 # for more details. 
 # 
 # You should have received a copy of the GNU General Public License 
 # and the GNU Lesser General Public License along with libSplash. 
 # If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 2.8)

project (domain_read)
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/" "$ENV{HDF5_ROOT}")

OPTION(RELEASE "disable all debug asserts" ON)
IF(NOT RELEASE)
    SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF(NOT RELEASE)

FIND_PACKAGE(HDF5 REQUIRED)
INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})
LINK_DIRECTORIES(${HDF5_LIBRARY_DIRS})
SET(LIBS ${LIBS} hdf5)

FIND_PACKAGE(MPI REQUIRED)
INCLUDE_DIRECTORIES(${MPI_INCLUDE_PATH})
SET(LIBS ${LIBS} ${MPI_LIBRARIES})

#LIBSPLASH
find_path(SPLASH_ROOT_DIR
  NAMES include/sdc_defines.hpp lib/libsplash.a
  PATHS ENV SPLASH_ROOT
  DOC "libSplash ROOT location"
)

INCLUDE_DIRECTORIES("${SPLASH_ROOT_DIR}/include")
LINK_DIRECTORIES("${SPLASH_ROOT_DIR}/lib")
SET(LIBS ${LIBS} splash)


ADD_EXECUTABLE(domain_read
     "domain_read.cpp"
)

TARGET_LINK_LIBRARIES(domain_read m ${LIBS})

