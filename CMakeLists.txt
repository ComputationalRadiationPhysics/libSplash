 #
 # Copyright 2013 Felix Schmitt, Axel Huebl
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

IF(VAMPIR_ENABLE)
    MESSAGE("Building libSplash with VampirTrace support")
    SET(CMAKE_C_COMPILER "vtcc")
    SET(CMAKE_CXX_COMPILER "vtc++")
ENDIF(VAMPIR_ENABLE)

#-------------------------------------------------------------------------------

PROJECT(libSplash)

#-------------------------------------------------------------------------------

#set helper pathes to find libraries and packages
#- you need the multiarch dir x86_64-linux-gnu on 64bit Ubuntu's with
#  self-compiled compilers
SET(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/" "$ENV{MPI_ROOT}"
                      "$ENV{HDF5_ROOT}")

#-------------------------------------------------------------------------------

# install prefix
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}" CACHE PATH "install prefix"
        FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 

#-------------------------------------------------------------------------------

# whats my version?
FILE(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_MAJOR_HPP REGEX "#define SPLASH_VERSION_MAJOR ")
FILE(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_MINOR_HPP REGEX "#define SPLASH_VERSION_MINOR ")
FILE(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_PATCH_HPP REGEX "#define SPLASH_VERSION_PATCH ")

STRING(REGEX MATCH "([0-9]+)" SPLASH_VERSION_MAJOR
                            ${SPLASH_VERSION_MAJOR_HPP})
STRING(REGEX MATCH "([0-9]+)" SPLASH_VERSION_MINOR
                            ${SPLASH_VERSION_MINOR_HPP})
STRING(REGEX MATCH "([0-9]+)" SPLASH_VERSION_PATCH
                            ${SPLASH_VERSION_PATCH_HPP})

SET(SPLASH_VERSION "${SPLASH_VERSION_MAJOR}.${SPLASH_VERSION_MINOR}.${SPLASH_VERSION_PATCH}")
MESSAGE(STATUS "Building libSplash version ${SPLASH_VERSION}")

#-------------------------------------------------------------------------------

# HDF5 is required package
FIND_PACKAGE(HDF5 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${HDF5_INCLUDE_DIR})

# check if static version of HDF5 is used
#   CMake prefers .so/shared libraries                 
#   if we find a static HDF5 lib in the ${HDF5_LIBRARIES}
#   it means there is only a static version installed
SET(HDF5_IS_STATIC ON)
STRING(FIND "${HDF5_LIBRARIES}" "hdf5.a" HDF5_IS_STATIC_POS)
IF(${HDF5_IS_STATIC_POS} EQUAL -1)
    SET(HDF5_IS_STATIC OFF)
ENDIF(${HDF5_IS_STATIC_POS} EQUAL -1)

#-------------------------------------------------------------------------------

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Woverloaded-virtual")

# options
OPTION(DEBUG_VERBOSE "Enable verbose HDF5 debug output" OFF)
IF(DEBUG_VERBOSE)
    ADD_DEFINITIONS(-DSPLASH_VERBOSE_HDF5)
ENDIF(DEBUG_VERBOSE)

OPTION(SPLASH_RELEASE "Release version, disable all debug asserts" ON)
IF(NOT SPLASH_RELEASE)
    SET(CMAKE_CXX_FLAGS_DEBUG "-g")
    SET(CMAKE_BUILD_TYPE Debug)

    MESSAGE(STATUS "Building DEBUG version")
ELSE(NOT SPLASH_RELEASE)
    SET(CMAKE_BUILD_TYPE Release)
    MESSAGE(STATUS "Building RELEASE version")
ENDIF(NOT SPLASH_RELEASE)

#-------------------------------------------------------------------------------

SET(SPLASH_LIBS z ${HDF5_LIBRARIES})

# serial or parallel version of libSplash
SET(SPLASH_CLASSES logging DCAttribute DCDataSet DCGroup HandleMgr SerialDataCollector DomainCollector)
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
    INCLUDE_DIRECTORIES(SYSTEM ${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
    SET(SPLASH_LIBS ${SPLASH_LIBS} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})

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

# build libsplash shared (if shared HDF5 is used)
IF(HDF5_IS_STATIC)
    MESSAGE(WARNING "Skipping dynamic libSplash because HDF5 library is static")
ELSE(HDF5_IS_STATIC)
    ADD_LIBRARY(splash SHARED ${LIBRARY_SOURCES})
    TARGET_LINK_LIBRARIES(splash ${SPLASH_LIBS})
ENDIF(HDF5_IS_STATIC)

# build libsplash static
ADD_LIBRARY(splash_static STATIC ${LIBRARY_SOURCES})
SET_TARGET_PROPERTIES(splash_static PROPERTIES OUTPUT_NAME splash)
TARGET_LINK_LIBRARIES (splash_static ${SPLASH_LIBS})

# install libs
IF(NOT HDF5_IS_STATIC)
    INSTALL(TARGETS splash LIBRARY DESTINATION lib)
ENDIF(NOT HDF5_IS_STATIC)

INSTALL(TARGETS splash_static ARCHIVE DESTINATION lib)

# copy include folder to install folder
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/include/."
  DESTINATION include
  PATTERN ".svn" EXCLUDE
  PATTERN ".git" EXCLUDE
  PATTERN "splash_*.h" EXCLUDE
)

# install correct splash header file
# (will be executed after the POST_BUILD copies of splash.h)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/splash/splash.h
        DESTINATION include/splash)

ADD_CUSTOM_COMMAND(TARGET splash_static POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/splash"
)

IF(HDF5_IS_PARALLEL)
    SET(splash_inc_file "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/splash_parallel.h")

    ADD_CUSTOM_COMMAND(TARGET splash_static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/splash_parallel.h"
                "${CMAKE_CURRENT_BINARY_DIR}/splash/splash.h"
        )
ELSE(HDF5_IS_PARALLEL)
    SET(splash_inc_file "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/splash_serial.h")

    ADD_CUSTOM_COMMAND(TARGET splash_static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/splash_serial.h"
                "${CMAKE_CURRENT_BINARY_DIR}/splash/splash.h"
        )
ENDIF(HDF5_IS_PARALLEL)

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
            INCLUDE_DIRECTORIES(SYSTEM ${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
        ENDIF(NOT HDF5_IS_PARALLEL)

        ADD_DEFINITIONS(-DENABLE_MPI=1)
        SET(TOOLS_LIBS ${TOOLS_LIBS} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
        MESSAGE(STATUS "Building splashtools MPI version")
    ELSE(TOOLS_MPI)
        IF(HDF5_IS_PARALLEL)
            SET(TOOLS_LIBS ${TOOLS_LIBS} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
        ENDIF(HDF5_IS_PARALLEL)
    ENDIF(TOOLS_MPI)

    # general settings for splashtools w/ and w/o parallel support
    IF(HDF5_IS_PARALLEL)
        ADD_DEFINITIONS(-DENABLE_PARALLEL=1)

        MESSAGE(STATUS "Building splashtools parallel version")
    ENDIF(HDF5_IS_PARALLEL)

    ADD_EXECUTABLE(splashtools ${TOOLS_SOURCES})
    ADD_DEPENDENCIES(splashtools splash_static)
    TARGET_LINK_LIBRARIES(splashtools ${TOOLS_LIBS})

    #install tools
    INSTALL(TARGETS splashtools RUNTIME DESTINATION bin)
    INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tools/splash2xdmf.py DESTINATION bin)
ENDIF(WITH_TOOLS)

#-------------------------------------------------------------------------------

# Packaging
# Reference for variables: http://www.cmake.org/Wiki/CMake:CPackConfiguration

SET(CPACK_GENERATOR "DEB;TGZ;TBZ2;ZIP")

SET(CPACK_PACKAGE_NAME "libsplash") # lower case required (debian policy)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Simple Parallel file output Library for Accumulating Simulation data using Hdf5")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
# a valid email is required - happy spamming :)
SET(CPACK_PACKAGE_CONTACT "Felix Schmitt <felix.schmitt@zih.tu-dresden.de>")
SET(CPACK_PACKAGE_VENDOR "ZIH and HZDR")
# CPACK_PACKAGING_INSTALL_PREFIX : default is "/usr" for deb and rpm

SET(CPACK_PACKAGE_VERSION_MAJOR "${SPLASH_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${SPLASH_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${SPLASH_VERSION_PATCH}")
SET(CPACK_PACKAGE_VERSION
    "${SPLASH_VERSION_MAJOR}.${SPLASH_VERSION_MINOR}.${SPLASH_VERSION_PATCH}")

# from `dpkg --print-architecture`
IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR
   CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
  SET(SPLASH_ARCHITECTURE "amd64")
ELSE()
  SET(SPLASH_ARCHITECTURE "i386")
ENDIF()

# probably only required by NSIS
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

# ship (strip) all found executables
SET(CPACK_STRIP_FILES ON)
# do not ship source files (packed in a weird absolute dir)
SET(CPACK_SOURCE_STRIP_FILES OFF)
#SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable") # only required by NSIS

# try to detect used MPI flavor
SET(MPI_FLAVOR "FLAVOR-NOTFOUND")
IF(HDF5_IS_PARALLEL OR TOOLS_MPI)
  SET(MPI_FLAVOR "openmpi")
  SET(MPI_FLAVOR_BIN "openmpi-bin")
  SET(MPI_FLAVOR_MINVERSION 1.5.1)
  STRING(FIND "${MPI_C_LIBRARIES}" "libmpich" MPI_FLAVOR_POS)
  IF(${MPI_FLAVOR_POS} GREATER -1)
    SET(MPI_FLAVOR "mpich2")
    SET(MPI_FLAVOR_BIN "mpich2")
    SET(MPI_FLAVOR_MINVERSION 1.4.1)
  ENDIF(${MPI_FLAVOR_POS} GREATER -1)
ENDIF(HDF5_IS_PARALLEL OR TOOLS_MPI)

# package name containing all necessary information for feature identification
# inspect .deb meta data (like version) with dpkg --info <package>.deb
IF(NOT HDF5_IS_PARALLEL)
  SET(SPLASH_PACKAGE_EXTRA_PREFIX "-serial")
ENDIF(NOT HDF5_IS_PARALLEL)
IF(MPI_FLAVOR)
  SET(SPLASH_PACKAGE_EXTRA_PREFIX
      "${SPLASH_PACKAGE_EXTRA_PREFIX}-${MPI_FLAVOR}")
ENDIF(MPI_FLAVOR)
IF(NOT SPLASH_RELEASE)
  SET(SPLASH_PACKAGE_DEBUG "-dbg")
ENDIF(NOT SPLASH_RELEASE)

SET(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}${SPLASH_PACKAGE_EXTRA_PREFIX}-${SPLASH_ARCHITECTURE}${SPLASH_PACKAGE_DEBUG}"
   )

# DEB specific ----------------------------------------------------------------
#   package relation policy at
#   http://www.debian.org/doc/debian-policy/ch-relationships.html

# default values that are already set in a fine and noble manner
# CPACK_DEBIAN_PACKAGE_MAINTAINER <- CPACK_PACKAGE_CONTACT
# CPACK_DEBIAN_PACKAGE_NAME <- CPACK_PACKAGE_NAME
# CPACK_DEBIAN_PACKAGE_VERSION <- CPACK_PACKAGE_VERSION
# CPACK_DEBIAN_PACKAGE_DESCRIPTION <- CPACK_PACKAGE_DESCRIPTION_SUMMARY
# CPACK_DEBIAN_PACKAGE_SECTION : default is "devel"
# CPACK_DEBIAN_PACKAGE_PRIORITY : default is "optional"
# CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA : pos/pre inst/rm scripts

# from `dpkg --print-architecture`
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${SPLASH_ARCHITECTURE}") # default is i386

# the dev packages are a little bit too much, but since there is a lack of
# convenient named meta packages we use them to trigger the dependencies
IF(HDF5_IS_PARALLEL)
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS
      "libhdf5-${MPI_FLAVOR}-dev (>= 1.8.6)"
     )
  SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS
      "${MPI_FLAVOR_BIN} (>= ${MPI_FLAVOR_MINVERSION})"
     )
ELSE(HDF5_IS_PARALLEL)
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS
      "libhdf5-serial-dev (>= 1.8.6)"
     )
  IF(TOOLS_MPI)	
    SET(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS}, lib${MPI_FLAVOR}-dev (>= ${MPI_FLAVOR_MINVERSION})"
       )
    SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS
        "${MPI_FLAVOR_BIN} (>= ${MPI_FLAVOR_MINVERSION})"
       )
  ENDIF(TOOLS_MPI)
ENDIF(HDF5_IS_PARALLEL)

# CPACK_DEBIAN_PACKAGE_SUGGESTS
# deb-src support
# DEBIAN_PACKAGE_BUILDS_DEPENDS : not implemented by cmake yet?

# END: DEB specific -----------------------------------------------------------

# create or creates specific library names like ???
#  libsplash.so     -> libsplash.so.1
#  libsplash.so.1   -> libsplash.so.1.1
#  libsplash.so.1.1 -> libsplash.so.1.1.0

# has to be included after all vars are set
INCLUDE(CPack)

