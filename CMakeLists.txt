# Copyright 2013-2018 Felix Schmitt, Axel Huebl
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

# Preamble ####################################################################
#
cmake_minimum_required(VERSION 3.10.0)

# version
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_MAJOR_HPP
     REGEX "#define SPLASH_VERSION_MAJOR "
 )
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_MINOR_HPP
     REGEX "#define SPLASH_VERSION_MINOR "
 )
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/include/splash/version.hpp"
     SPLASH_VERSION_PATCH_HPP
     REGEX "#define SPLASH_VERSION_PATCH "
 )

string(REGEX MATCH "([0-9]+)"
    SPLASH_VERSION_MAJOR
    ${SPLASH_VERSION_MAJOR_HPP}
)
string(REGEX MATCH "([0-9]+)"
    SPLASH_VERSION_MINOR
    ${SPLASH_VERSION_MINOR_HPP}
)
string(REGEX MATCH "([0-9]+)"
    SPLASH_VERSION_PATCH
    ${SPLASH_VERSION_PATCH_HPP}
)

project(Splash
    VERSION ${SPLASH_VERSION_MAJOR}.${SPLASH_VERSION_MINOR}.${SPLASH_VERSION_PATCH}
) #LANGUAGES CXX


# Options and Variants ########################################################
#
function(splash_option name description default)
    set(Splash_USE_${name} ${default} CACHE STRING "${description}")
    set_property(CACHE Splash_USE_${name} PROPERTY
        STRINGS "ON;TRUE;AUTO;OFF;FALSE"
    )
    if(Splash_HAVE_${name})
        set(Splash_HAVE_${name} TRUE)
    else()
        set(Splash_HAVE_${name})
    endif()
    set(Splash_CONFIG_OPTIONS ${Splash_CONFIG_OPTIONS} ${name} PARENT_SCOPE)
endfunction()

splash_option(MPI "Enable MPI support (does NOT imply parallel HDF5)" AUTO)
splash_option(PARALLEL "Enable support for parallel HDF5 (implies MPI)" AUTO)
set(Splash_HAVE_COLLECTIVE ON CACHE STRING "Enable collective I/O for parallel HDF5")
list(APPEND Splash_CONFIG_OPTIONS COLLECTIVE)
set(Splash_HAVE_TOOLS ON CACHE STRING "Enable tools")
list(APPEND Splash_CONFIG_OPTIONS)
set(Splash_HAVE_TESTS OFF CACHE STRING "Enable tests")
list(APPEND Splash_CONFIG_OPTIONS TESTS)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the build type, e.g. Debug." FORCE)
endif()

# Dependencies ################################################################
#
# external library: HDF5 (mandatory)
if(Splash_USE_PARALLEL STREQUAL AUTO)
    set(HDF5_PREFER_PARALLEL TRUE)
    find_package(HDF5 1.8.6 REQUIRED COMPONENTS C)
    if(HDF5_IS_PARALLEL)
        set(Splash_HAVE_PARALLEL TRUE)
        set(Splash_USE_MPI TRUE)
    endif()
elseif(Splash_USE_PARALLEL)
    set(HDF5_PREFER_PARALLEL TRUE)
    find_package(HDF5 1.8.6 REQUIRED COMPONENTS C)
    if(NOT HDF5_IS_PARALLEL)
        message(FATAL_ERROR
            "Parallel HDF5 required but only serial version found!"
        )
    endif()
    set(Splash_HAVE_PARALLEL TRUE)
    set(Splash_USE_MPI TRUE)
else()
    set(HDF5_PREFER_PARALLEL FALSE)
    find_package(HDF5 1.8.6 REQUIRED COMPONENTS C)
    if(HDF5_IS_PARALLEL)
        message(FATAL_ERROR
            "Serial HDF5 required but only parallel version found!"
        )
    endif()
    set(Splash_HAVE_PARALLEL FALSE)
endif()

# external library: zlib (mandatory)
find_package(ZLIB REQUIRED)

if(Splash_USE_MPI STREQUAL AUTO)
    find_package(MPI)
elseif(Splash_USE_MPI)
    find_package(MPI REQUIRED)
endif()
if(MPI_FOUND)
    set(Splash_HAVE_MPI TRUE)
else()
    set(Splash_HAVE_MPI FALSE)
endif()

# external library: CPPUnit (mandatory for tests)
if(Splash_HAVE_TESTS)
    find_path(CPPUNIT_INCLUDE_DIR cppunit/Test.h)
    find_library(CPPUNIT_LIBRARY NAMES cppunit)

    if(CPPUNIT_INCLUDE_DIR AND CPPUNIT_LIBRARY)
    else()
        message(FATAL_ERROR "CPPUnit Library NOT found! Can NOT build tests!")
    endif()
endif()

# Targets #####################################################################
#
set(SPLASH_CLASSES
    logging
    DCAttribute
    DCDataSet
    DCGroup
    HandleMgr
    SerialDataCollector
    DomainCollector
    SDCHelper
    AttributeInfo
    generateCollectionType
)
if(Splash_HAVE_PARALLEL)
    list(APPEND SPLASH_CLASSES
        ParallelDataCollector
        ParallelDomainCollector
    )
endif()

set(LIBRARY_SOURCES)
foreach(class ${SPLASH_CLASSES})
    list(APPEND LIBRARY_SOURCES ${Splash_SOURCE_DIR}/src/${class}.cpp)
endforeach()

add_library(Splash
    ${LIBRARY_SOURCES}
)
unset(LIBRARY_SOURCES)

target_link_libraries(Splash PUBLIC ${HDF5_LIBRARIES})
target_link_libraries(Splash PRIVATE ZLIB::ZLIB)
if(Splash_HAVE_MPI)
    # MPI targets: CMake 3.9+
    # note: often the PUBLIC dependency to CXX is missing in C targets...
    target_link_libraries(Splash PUBLIC MPI::MPI_C MPI::MPI_CXX)
endif()
if(NOT WIN32)
    # added automatically on Windows
    target_link_libraries(Splash PRIVATE m)
endif()

target_include_directories(Splash SYSTEM PUBLIC ${HDF5_INCLUDE_DIRS})
target_compile_definitions(Splash PUBLIC ${HDF5_DEFINITIONS})

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(Splash PRIVATE "-DSPLASH_VERBOSE_HDF5")
endif()
if(NOT Splash_HAVE_COLLECTIVE)
    # removed functionality ?
    target_compile_definitions(Splash PRIVATE "-DSPLASH_INDEPENDENT_IO=1")
endif()

# Splash headers
target_include_directories(Splash PUBLIC
    $<BUILD_INTERFACE:${Splash_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${Splash_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# tools
if(Splash_HAVE_TOOLS)
    add_executable(splashtools
        tools/splashtools.cpp
    )
    target_link_libraries(splashtools PRIVATE Splash)
    if(Splash_HAVE_MPI)
       target_compile_definitions(splashtools PRIVATE "-DENABLE_MPI=1")
    endif()
    if(Splash_HAVE_PARALLEL)
      # unused option?
      target_compile_definitions(splashtools PRIVATE "-DENABLE_PARALLEL=1")
    endif()
endif()

# examples
add_executable(domain_read
    examples/domain_read/domain_read.cpp
)
target_link_libraries(domain_read PRIVATE Splash)
if(Splash_HAVE_MPI)
    add_executable(domain_read_mpi
        examples/domain_read/domain_read_mpi.cpp
    )
    target_link_libraries(domain_read_mpi PRIVATE Splash)
    add_executable(domain_write_mpi
        examples/domain_write/domain_write_mpi.cpp
    )
    target_link_libraries(domain_write_mpi PRIVATE Splash)
endif()
if(Splash_HAVE_PARALLEL)
    add_executable(parallel_domain_write
        examples/parallel_domain_write/parallel_domain_write.cpp
    )
    target_link_libraries(parallel_domain_write PRIVATE Splash)
endif()

# tests
if(Splash_HAVE_TESTS)
    set(SRCFILESOTHER dependencies/runner.cpp)
    set(TEST_NAMES
        Append
        Attributes
        FileAccess
        Filename
        References
        Remove
        SimpleData
        Striding
    )
    if(Splash_HAVE_MPI)
        list(APPEND TEST_NAMES
            Benchmark
            Domains
        )
    endif()
    if(Splash_HAVE_PARALLEL)
        list(APPEND TEST_NAMES
            Parallel_Attributes
            Parallel_Filename
            Parallel_Domains
            Parallel_ListFiles
            Parallel_References
            Parallel_Remove
            Parallel_SerialDC
            Parallel_SimpleData
            Parallel_ZeroAccess
        )
    endif()

    foreach(name ${TEST_NAMES})
        add_executable(${name}Test
            tests/${name}Test.cpp
            tests/dependencies/runner.cpp
        )
        target_link_libraries(${name}Test PRIVATE Splash)
        target_link_libraries(${name}Test PRIVATE ${CPPUNIT_LIBRARY})
        target_include_directories(${name}Test PRIVATE SYSTEM ${CPPUNIT_INCLUDE_DIR})
        target_include_directories(${name}Test PRIVATE
            $<BUILD_INTERFACE:${Splash_BINARY_DIR}/tests/include>
            $<BUILD_INTERFACE:${Splash_SOURCE_DIR}/tests/include>
        )
    endforeach()
endif()


# Generate Files with Configuration Options ###################################
#
configure_file(
    ${Splash_SOURCE_DIR}/src/include/splash/version.hpp
    ${Splash_BINARY_DIR}/include/splash/version.hpp
)

configure_file(
    ${Splash_SOURCE_DIR}/SplashConfig.cmake.in
    ${Splash_BINARY_DIR}/SplashConfig.cmake
    @ONLY
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("SplashConfigVersion.cmake"
    VERSION ${Splash_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Installs ####################################################################
#
# headers, libraries and exectuables
install(TARGETS Splash EXPORT SplashTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/include/."
  DESTINATION include
  PATTERN ".svn" EXCLUDE
  PATTERN ".git" EXCLUDE
  PATTERN "splash_*.h" EXCLUDE
  PATTERN "version.hpp" EXCLUDE
)

# CMake package file for find_package(Splash::Splash) in depending projects
install(EXPORT SplashTargets
    FILE SplashTargets.cmake
    NAMESPACE Splash::
    DESTINATION lib/cmake/Splash
)
install(
    FILES
        ${Splash_BINARY_DIR}/SplashConfig.cmake
        ${Splash_BINARY_DIR}/SplashConfigVersion.cmake
    DESTINATION lib/cmake/Splash
)

# install correct splash header file
# (will be executed after POST_BUILD)
install(FILES ${Splash_BINARY_DIR}/include/splash/splash.h
        DESTINATION include/splash)
install(FILES ${Splash_BINARY_DIR}/include/splash/version.hpp
    DESTINATION include/splash
)

add_custom_command(TARGET Splash PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${Splash_BINARY_DIR}/include/splash"
)

if(Splash_HAVE_PARALLEL)
    set(splash_inc_file "${Splash_SOURCE_DIR}/src/include/splash/splash_parallel.h")
else()
    set(splash_inc_file "${Splash_SOURCE_DIR}/src/include/splash/splash_serial.h")
endif()
add_custom_command(TARGET Splash PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${splash_inc_file}
        ${Splash_BINARY_DIR}/include/splash/splash.h
)
if(Splash_HAVE_TESTS)
    add_custom_command(TARGET Splash PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${Splash_SOURCE_DIR}/tests/readBoolChar.py
            ${Splash_BINARY_DIR}
    )
    add_custom_command(TARGET Splash PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${Splash_SOURCE_DIR}/tests/run_tests
            ${Splash_BINARY_DIR}
    )
    add_custom_command(TARGET Splash PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${Splash_SOURCE_DIR}/tests/run_parallel_tests
            ${Splash_BINARY_DIR}
    )
endif()

# install python scripts
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tools/splash2xdmf.py
    DESTINATION share/pyshared)


# install tools
if(Splash_HAVE_TOOLS)
    install(TARGETS splashtools RUNTIME DESTINATION bin)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tools/splash2xdmf.py DESTINATION bin)


# Tests #######################################################################
#
enable_testing()

# OpenMPI root guard: https://github.com/open-mpi/ompi/issues/4451
if("$ENV{USER}" STREQUAL "root")
    set(MPI_ALLOW_ROOT --allow-run-as-root)
endif()
set(MPI_TEST_EXE ${MPIEXEC_EXECUTABLE} ${MPI_ALLOW_ROOT} ${MPIEXEC_NUMPROC_FLAG})

if(Splash_HAVE_TESTS)
    add_custom_command(TARGET SimpleDataTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${Splash_BINARY_DIR}/h5
    )

    add_test(NAME Serial.SimpleData
        COMMAND ./SimpleDataTest
    )
    add_test(NAME Serial.Filename
        COMMAND FilenameTest
    )
    add_test(NAME Serial.Attributes
        COMMAND AttributesTest
    )
    add_test(NAME h5py.readBoolChar
        COMMAND readBoolChar.py
    )
    add_test(NAME Serial.Append
        COMMAND AppendTest
    )
    add_test(NAME Serial.FileAccess
        COMMAND FileAccessTest
    )
    add_test(NAME Serial.Striding
        COMMAND StridingTest
    )
    add_test(NAME Serial.Remove
        COMMAND RemoveTest
    )
    add_test(NAME Serial.References
        COMMAND ReferencesTest
    )
    if(Splash_HAVE_MPI)
        add_test(NAME MPI.Domains
            COMMAND ${MPI_TEST_EXE}
                    8 DomainsTest
        )
    endif()
    if(Splash_HAVE_PARALLEL)
        add_test(NAME Parallel.SimpleData
            COMMAND ${MPI_TEST_EXE}
                    8 Parallel_SimpleDataTest
        )
        add_test(NAME Parallel.Filename
            COMMAND ${MPI_TEST_EXE}
                    1 Parallel_FilenameTest
        )
        add_test(NAME Parallel.SerialDC
            COMMAND ${MPI_TEST_EXE}
                    8 Parallel_SerialDCTest
        )
        add_test(NAME Parallel.ListFiles
            COMMAND ${MPI_TEST_EXE}
                    1 Parallel_ListFilesTest
        )
        add_test(NAME Parallel.Domains
            COMMAND ${MPI_TEST_EXE}
                    8 Parallel_DomainsTest
        )
        add_test(NAME Parallel.Attributes
            COMMAND ${MPI_TEST_EXE}
                    4 Parallel_AttributesTest
        )
        add_test(NAME Parallel.Remove
            COMMAND ${MPI_TEST_EXE}
                    2 Parallel_RemoveTest
        )
        add_test(NAME Parallel.References
            COMMAND ${MPI_TEST_EXE}
                    2 Parallel_ReferencesTest
        )
        add_test(NAME Parallel.ZeroAccess
            COMMAND ${MPI_TEST_EXE}
                    2 Parallel_ZeroAccessTest
        )
        add_test(NAME Parallel.ZeroAccess2
            COMMAND ${MPI_TEST_EXE}
                    4 Parallel_ZeroAccessTest
        )
        add_test(NAME Parallel.ZeroAccess3
            COMMAND ${MPI_TEST_EXE}
                    7 Parallel_ZeroAccessTest
        )
    endif()
endif()

# examples
if(Splash_HAVE_MPI)
    add_test(NAME Examples.DomainWriteMPI
        COMMAND ${MPI_TEST_EXE}
                4 domain_write_mpi h5ex 2 2 1
    )
    add_test(NAME Examples.DomainReadMPI
        COMMAND ${MPI_TEST_EXE}
                4 domain_read_mpi h5ex 2 2 1
    )
    add_test(NAME Examples.DomainRead
        COMMAND domain_read h5ex
    )
endif()
if(Splash_HAVE_PARALLEL)
    add_test(NAME Examples.ParallelDomainWrite
        COMMAND ${MPI_TEST_EXE}
                4 parallel_domain_write ph5 2 2 1
    )
endif()


# Status Message for Build Options ############################################
#
message("")
message("Splash build configuration:")
message("  Splash Version: ${Splash_VERSION}")
message("  C++ Compiler : ${CMAKE_CXX_COMPILER_ID} "
                         "${CMAKE_CXX_COMPILER_VERSION} "
                         "${CMAKE_CXX_COMPILER_WRAPPER}")
message("    ${CMAKE_CXX_COMPILER}")
message("")
message("  Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message("")
message("  Build Type: ${CMAKE_BUILD_TYPE}")
message("  Build Options:")

foreach(opt IN LISTS Splash_CONFIG_OPTIONS)
  if(${Splash_HAVE_${opt}})
    message("    ${opt}: ON")
  else()
    message("    ${opt}: OFF")
  endif()
endforeach()
message("")


# Packaging ###################################################################
#
# Reference for variables: http://www.cmake.org/Wiki/CMake:CPackConfiguration

set(CPACK_GENERATOR "DEB;TGZ;TBZ2;ZIP")

set(CPACK_PACKAGE_NAME "libsplash") # lower case required (debian policy)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Simple Parallel file output Library for Accumulating Simulation data using Hdf5")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${Splash_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_README "${Splash_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${Splash_SOURCE_DIR}/COPYING")
# a valid email is required - happy spamming :)
set(CPACK_PACKAGE_CONTACT "Felix Schmitt <felix.schmitt@zih.tu-dresden.de>")
set(CPACK_PACKAGE_VENDOR "ZIH and HZDR")
# CPACK_PACKAGING_INSTALL_PREFIX : default is "/usr" for deb and rpm

set(CPACK_PACKAGE_VERSION_MAJOR "${SPLASH_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${SPLASH_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${SPLASH_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${Splash_VERSION}")

# from `dpkg --print-architecture`
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR
   CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
  set(SPLASH_ARCHITECTURE "amd64")
else()
  set(SPLASH_ARCHITECTURE "i386")
endif()

# probably only required by NSIS
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

# ship (strip) all found executables
set(CPACK_STRIP_FILES ON)
# do not ship source files (packed in a weird absolute dir)
set(CPACK_SOURCE_STRIP_FILES OFF)
#set(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable") # only required by NSIS

# try to detect used MPI flavor
set(MPI_FLAVOR "FLAVOR-NOTFOUND")
if(Splash_HAVE_MPI)
  set(MPI_FLAVOR "openmpi")
  set(MPI_FLAVOR_BIN "openmpi-bin")
  set(MPI_FLAVOR_MINVERSION 1.5.1)
  string(FIND "${MPI_C_LIBRARIES}" "libmpich" MPI_FLAVOR_POS)
  if(${MPI_FLAVOR_POS} GREATER -1)
    set(MPI_FLAVOR "mpich2")
    set(MPI_FLAVOR_BIN "mpich2")
    set(MPI_FLAVOR_MINVERSION 1.4.1)
  endif()
endif()

# package name containing all necessary information for feature identification
# inspect .deb meta data (like version) with dpkg --info <package>.deb
if(NOT Splash_HAVE_PARALLEL)
  set(SPLASH_PACKAGE_EXTRA_PREFIX "-serial")
endif()
if(MPI_FLAVOR)
  set(SPLASH_PACKAGE_EXTRA_PREFIX
      "${SPLASH_PACKAGE_EXTRA_PREFIX}-${MPI_FLAVOR}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(SPLASH_PACKAGE_DEBUG "-dbg")
endif()

set(CPACK_PACKAGE_FILE_NAME
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
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${SPLASH_ARCHITECTURE}") # default is i386

# the dev packages are a little bit too much, but since there is a lack of
# convenient named meta packages we use them to trigger the dependencies
if(Splash_HAVE_PARALLEL)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
      "libhdf5-${MPI_FLAVOR}-dev (>= 1.8.6)"
     )
  set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
      "${MPI_FLAVOR_BIN} (>= ${MPI_FLAVOR_MINVERSION})"
     )
else()
  set(CPACK_DEBIAN_PACKAGE_DEPENDS
      "libhdf5-serial-dev (>= 1.8.6)"
     )
  if(Splash_HAVE_MPI)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "${CPACK_DEBIAN_PACKAGE_DEPENDS}, lib${MPI_FLAVOR}-dev (>= ${MPI_FLAVOR_MINVERSION})"
       )
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
        "${MPI_FLAVOR_BIN} (>= ${MPI_FLAVOR_MINVERSION})"
       )
  endif()
endif()

# CPACK_DEBIAN_PACKAGE_SUGGESTS
# deb-src support
# DEBIAN_PACKAGE_BUILDS_DEPENDS : not implemented by cmake yet

# END: DEB specific -----------------------------------------------------------

# create or creates specific library names
#  libsplash.so     -> libsplash.so.1
#  libsplash.so.1   -> libsplash.so.1.1
#  libsplash.so.1.1 -> libsplash.so.1.1.0

# has to be included after all vars are set
include(CPack)

