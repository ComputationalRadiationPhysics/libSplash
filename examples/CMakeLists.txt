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

IF(VAMPIR_ENABLE)
    MESSAGE("Building examples with VampirTrace support")
    SET(CMAKE_C_COMPILER "vtcc")
    SET(CMAKE_CXX_COMPILER "vtc++")
ENDIF(VAMPIR_ENABLE)

#-------------------------------------------------------------------------------

PROJECT(splash-examples)

#-------------------------------------------------------------------------------

SET(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/" "$ENV{MPI_ROOT}" "$ENV{HDF5_ROOT}")

SET(CMAKE_CXX_FLAGS_DEBUG "-g")
SET(CMAKE_BUILD_TYPE Debug)
OPTION(WITH_MPI "build MPI examples" OFF)

SET(EXAMPLES domain_read/domain_read)
SET(MPI_EXAMPLES domain_read/domain_read_mpi domain_write/domain_write_mpi)

FOREACH(EXAMPLE_NAME ${EXAMPLES})
    SET(EXAMPLE_FILES "${EXAMPLE_FILES};${EXAMPLE_NAME}.cpp")
ENDFOREACH()

FOREACH(EXAMPLE_NAME ${MPI_EXAMPLES})
    SET(MPI_EXAMPLE_FILES "${MPI_EXAMPLE_FILES};${EXAMPLE_NAME}.cpp")
ENDFOREACH()

#-------------------------------------------------------------------------------

# HDF5 is required
FIND_PACKAGE(HDF5 1.8.6 REQUIRED)
INCLUDE_DIRECTORIES(${HDF5_INCLUDE_DIR})
SET(LIBS ${LIBS} ${HDF5_LIBRARIES})

# libSplash installation is required
find_path(SPLASH_ROOT_DIR
  NAMES include/splash/splash.h lib/libsplash.a
  PATHS ENV SPLASH_ROOT
  DOC "libSplash ROOT location"
)
INCLUDE_DIRECTORIES("${SPLASH_ROOT_DIR}/include")

# link static
ADD_LIBRARY(splash_static STATIC IMPORTED)
SET_TARGET_PROPERTIES(splash_static PROPERTIES 
	IMPORTED_LOCATION "${SPLASH_ROOT_DIR}/lib/libsplash.a")
SET(LIBS splash_static ${LIBS})

# link dynamic
#LINK_DIRECTORIES("${SPLASH_ROOT_DIR}/lib")
#SET(LIBS ${LIBS} splash)

# MPI
IF(WITH_MPI OR HDF5_IS_PARALLEL)
    FIND_PACKAGE(MPI REQUIRED)
    INCLUDE_DIRECTORIES(${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})
    SET(MPI_LIBS ${LIBS} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})

    IF(HDF5_IS_PARALLEL)
        MESSAGE(STATUS "Found parallel HDF5, linking MPI libraries")
        SET(LIBS ${MPI_LIBS})
    ENDIF(HDF5_IS_PARALLEL)
ENDIF(WITH_MPI OR HDF5_IS_PARALLEL)

#-------------------------------------------------------------------------------

# build all examples
MESSAGE(STATUS "Building serial examples")
FOREACH(EXAMPLE_FILE ${EXAMPLE_FILES})
    GET_FILENAME_COMPONENT(FILE ${EXAMPLE_FILE} NAME)
    MESSAGE(STATUS ${FILE})
    ADD_EXECUTABLE(${FILE}.out
        ${EXAMPLE_FILE}
    )

    TARGET_LINK_LIBRARIES(${FILE}.out m ${LIBS})
ENDFOREACH()

# build all MPI examples
IF(WITH_MPI)
    MESSAGE(STATUS "Building MPI examples")
    FOREACH(EXAMPLE_FILE ${MPI_EXAMPLE_FILES})
        GET_FILENAME_COMPONENT(FILE ${EXAMPLE_FILE} NAME)
        MESSAGE(STATUS ${FILE})
        ADD_EXECUTABLE(${FILE}.out
            ${EXAMPLE_FILE}
        )

        TARGET_LINK_LIBRARIES(${FILE}.out ${MPI_LIBS} )
    ENDFOREACH()
ENDIF(WITH_MPI)
