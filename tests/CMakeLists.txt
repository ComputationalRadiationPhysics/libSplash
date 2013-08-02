# Projekt name
cmake_minimum_required(VERSION 2.6)

SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS_DEBUG "-g")

# HDF5 is required package
FIND_PACKAGE(HDF5 REQUIRED)
INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})
LINK_DIRECTORIES(${HDF5_LIBRARY_DIRS})

OPTION(TESTS_DEBUG "enable debug output for tests" OFF)
IF(TESTS_DEBUG)
    ADD_DEFINITIONS(-DTESTS_DEBUG)
ENDIF(TESTS_DEBUG)

INCLUDE_DIRECTORIES(include ../src/include $ENV{MPI_INC} )

FILE(GLOB SRCFILES "*.cpp")
FILE(GLOB SRCFILESOTHER "dependencies/*.cpp")

ADD_SUBDIRECTORY(.. "${CMAKE_CURRENT_BINARY_DIR}/build_libsplash")

FOREACH(SRCFILE ${SRCFILES})
    GET_FILENAME_COMPONENT(FILE ${SRCFILE} NAME)
    MESSAGE(${FILE})
    ADD_EXECUTABLE(${FILE}.out
        ${SRCFILE}
        ${SRCFILESOTHER}
    )

    IF(${vampir})
        TARGET_LINK_LIBRARIES(${FILE}.out vt-hyb splash m cppunit mpi mpi_cxx hdf5)
    ELSE()
        TARGET_LINK_LIBRARIES(${FILE}.out splash m cppunit mpi mpi_cxx hdf5)
    ENDIF()
ENDFOREACH()

