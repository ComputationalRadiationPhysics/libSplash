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

cmake_minimum_required(VERSION 2.8.5)

project (libSplash)

#set helper pathes to find libraries and packages
set(CMAKE_PREFIX_PATH "$ENV{MPI_ROOT}" "$ENV{HDF5_ROOT}")

#-------------------------------------------------------------------------------

# install prefix
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}" CACHE PATH "install prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 

#-------------------------------------------------------------------------------

# HDF5 is required package
FIND_PACKAGE(HDF5 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${HDF5_INCLUDE_DIR})
LINK_DIRECTORIES(${HDF5_LIBRARY_DIRS})

#-------------------------------------------------------------------------------

SET(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -Woverloaded-virtual")
# options
OPTION(DEBUG_VERBOSE "enable verbose debug output" OFF)
OPTION(SPLASH_RELEASE "optimize, disable all debug asserts" ON)
IF(NOT SPLASH_RELEASE)
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")
    SET(CMAKE_BUILD_TYPE Debug)

    IF(DEBUG_VERBOSE)
        ADD_DEFINITIONS(-DSDC_DEBUG_OUTPUT)
        ADD_DEFINITIONS(-DDC_DEBUG=1)
    ENDIF(DEBUG_VERBOSE)

    MESSAGE(STATUS "Building DEBUG version")
ELSE(NOT SPLASH_RELEASE)
    SET(CMAKE_BUILD_TYPE Release)
    MESSAGE(STATUS "Building RELEASE version")
ENDIF(NOT SPLASH_RELEASE)

#-------------------------------------------------------------------------------

SET(SPLASH_LIBS z ${HDF5_LIBRARIES})

# serial or parallel version of libSplash
SET(SPLASH_CLASSES DCAttribute DCDataSet DCGroup HandleMgr SerialDataCollector DomainCollector)
IF(HDF5_IS_PARALLEL)
    #parallel version 
    MESSAGE(STATUS "Parallel HDF5 found. Building parallel version")

    OPTION(COLLECTIVE_IO "use HDF5 collective IO" ON)
    IF(NOT COLLECTIVE_IO)
        ADD_DEFINITIONS(-DSPLASH_INDEPENDENT_IO=1)
        MESSAGE(STATUS "Enable HDF5 independent IO")
    ENDIF(NOT COLLECTIVE_IO)

    # MPI is required package
    FIND_PACKAGE(MPI REQUIRED)
    INCLUDE_DIRECTORIES(SYSTEM ${MPI_INCLUDE_PATH})
    SET(SPLASH_LIBS ${SPLASH_LIBS} ${MPI_LIBRARIES})

    SET(SPLASH_CLASSES ${SPLASH_CLASSES} ParallelDataCollector ParallelDomainCollector)
ELSE(HDF5_IS_PARALLEL)
    #serial version

    MESSAGE(STATUS "Parallel HDF5 NOT found. Building serial version")
ENDIF(HDF5_IS_PARALLEL)

#-------------------------------------------------------------------------------

# add includes and sources
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/src/include")
FOREACH(class ${SPLASH_CLASSES})
    SET(LIBRARY_SOURCES "${LIBRARY_SOURCES};${CMAKE_CURRENT_SOURCE_DIR}/src/${class}.cpp")
ENDFOREACH()
AUX_SOURCE_DIRECTORY("${CMAKE_CURRENT_SOURCE_DIR}/tools" TOOLS_SOURCES)

#-------------------------------------------------------------------------------

# build libsplash shared
ADD_LIBRARY(splash SHARED ${LIBRARY_SOURCES})
TARGET_LINK_LIBRARIES (splash ${SPLASH_LIBS})

#build libsplash static
ADD_LIBRARY(splash_static STATIC ${LIBRARY_SOURCES})
SET_TARGET_PROPERTIES(splash_static PROPERTIES OUTPUT_NAME splash)
TARGET_LINK_LIBRARIES (splash_static ${SPLASH_LIBS})

# install libs
INSTALL(TARGETS splash LIBRARY DESTINATION lib)
INSTALL(TARGETS splash_static ARCHIVE DESTINATION lib)

# copy include folder to install folder
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/include/."
  DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
  PATTERN ".svn" EXCLUDE
  PATTERN ".git" EXCLUDE
  PATTERN "splash_*.h" EXCLUDE
)

# install correct splash header file
INSTALL(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/PostInstall.cmake")
IF(HDF5_IS_PARALLEL)
    SET(splash_inc_file "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash_parallel.h")

    ADD_CUSTOM_COMMAND(TARGET splash POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash_parallel.h"
                "${CMAKE_CURRENT_BINARY_DIR}/splash.h"
        )
ELSE(HDF5_IS_PARALLEL)
    SET(splash_inc_file "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash_serial.h")

    ADD_CUSTOM_COMMAND(TARGET splash POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash_serial.h"
                "${CMAKE_CURRENT_BINARY_DIR}/splash.h"
        )
ENDIF(HDF5_IS_PARALLEL)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/PostInstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PostInstall.cmake @ONLY)

#-------------------------------------------------------------------------------

# build tools
OPTION(WITH_TOOLS "enable splashtools" ON)
IF(WITH_TOOLS)
    MESSAGE(STATUS "Building splashtools")

    SET(TOOLS_LIBS ${TOOLS_LIBS} splash_static)
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

    # MPI for tools
    OPTION(TOOLS_MPI "enable splashtools MPI support" OFF)
    IF(TOOLS_MPI)
        IF(NOT HDF5_IS_PARALLEL)
            # MPI is required package but not yet included
            FIND_PACKAGE(MPI REQUIRED)
            INCLUDE_DIRECTORIES(SYSTEM ${MPI_INCLUDE_PATH})
        ENDIF(NOT HDF5_IS_PARALLEL)

        ADD_DEFINITIONS(-DENABLE_MPI=1)
        SET(TOOLS_LIBS ${TOOLS_LIBS} ${MPI_LIBRARIES})
        MESSAGE(STATUS "Building splashtools MPI version")
    ELSE(TOOLS_MPI)
        IF(HDF5_IS_PARALLEL)
            SET(TOOLS_LIBS ${TOOLS_LIBS} ${MPI_LIBRARIES})
        ENDIF(HDF5_IS_PARALLEL)
    ENDIF(TOOLS_MPI)

    # general settings for splashtools w/ and w/o parallel support
    IF(HDF5_IS_PARALLEL)
        ADD_DEFINITIONS(-DENABLE_PARALLEL=1)

        MESSAGE(STATUS "Building splashtools parallel version")
    ENDIF(HDF5_IS_PARALLEL)

    ADD_EXECUTABLE(splashtools ${TOOLS_SOURCES})
    ADD_DEPENDENCIES(splashtools splash splash_static)
    TARGET_LINK_LIBRARIES(splashtools ${TOOLS_LIBS})

    #install tools
    INSTALL(TARGETS splashtools RUNTIME DESTINATION bin)
ENDIF(WITH_TOOLS)




