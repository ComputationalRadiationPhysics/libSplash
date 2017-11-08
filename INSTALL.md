libSplash Installation Instructions
===================================

Requirements
------------

**Building** libSplash from source requires to checkout the trunk via Git
using the command
`$ git clone git@github.com:ComputationalRadiationPhysics/libSplash.git`

Building libSplash **requires HDF5 in version 1.8.6** or higher.
For **MPI based parallel output** configure HDF5 with `--enable-parallel`.

To use the CMakeLists.txt file which comes with the source code, you must have
**CMake version 3.3.0** or higher installed.

The splashtools and some tests also require an **MPI 2.2** compatible MPI library,
e.g. **OpenMPI 1.5.1** or higher.

Our *tests* require the development version of the **CppUnit** library and
python support for `h5py` & `numpy`.


Compiling
---------

To **compile libSplash**, it is recommended to create a new build folder and execute
```bash
cmake -DCMAKE_INSTALL_PREFIX=<INSTALL PATH> <CMakeLists.txt-PATH>
```

Than run `$ make install` to build and install libSplash library and binaries to the chosen
installation directory. This will build libSplash as an shared object library (libsplash.so)
as well as all tools.

If you do not want to build splashtools, pass `-DWITH_TOOLS=OFF` to your cmake command.
To disable MPI parallel splashtools, pass `-DTOOLS_MPI=OFF` to your cmake command.

By default, the RELEASE version is built. To create libSplash with DEBUG symbols,
pass `-DSPLASH_RELEASE=OFF` to your cmake command.
To see verbose internal (!) HDF5 debug output, pass `-DDEBUG_VERBOSE=ON`
to your cmake command line.

Afterwards, configure your environment variables:
```bash
# helps finding the installed library, e.g. with CMake scripts
export SPLASH_ROOT=<INSTALL PATH>
# provides command line access to our tools
export PATH=$SPLASH_ROOT/bin:$PATH
# path for the linker
export LD_LIBRARY_PATH=$SPLASH_ROOT/lib:$LD_LIBRARY_PATH
# provides our python modules, e.g. for xdmf creation
export PYTHONPATH=$SPLASH_ROOT/bin:$PYTHONPATH
```


Linking to your Project
-----------------------

To use libSplash in your project, you must link against the created shared object library
libsplash.so or against the statically linked archive libsplash.a.

Because we are linking to HDF5, the following **external dependencies** must be linked:
- `-lhdf5`, `-lpthread`, `-lz`, `-lrt`, `-ldl`, `-lm`

If you are using CMake you can download our `FindSplash.cmake` module with
```bash
wget https://raw.githubusercontent.com/ComputationalRadiationPhysics/picongpu/dev/src/cmake/FindSplash.cmake
# read the documentation
cmake -DCMAKE_MODULE_PATH=. --help-module FindSplash | less
```

and use the following lines in your `CMakeLists.txt`:
```cmake
# this example will require at least CMake 3.3.0
cmake_minimum_required(VERSION 3.3.0)

# add path to FindSplash.cmake, e.g. in the directory in cmake/
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

# find libSplash installation
#   optional: prefer static libraries over shared ones (but do not force them)
set(Splash_USE_STATIC_LIBS ON)

#   optional: specifiy (minimal) version, require Splash and specific components, e.g.
#           (Splash 1.1.1 REQUIRED COMPONENTS PARALLEL)
find_package(Splash)

if(Splash_FOUND)
  # where to find headers (-I includes for compiler)
  include_directories(SYSTEM ${Splash_INCLUDE_DIRS})
  # additional compiler flags (-DFOO=bar)
  add_definitions(${Splash_DEFINITIONS})
  # libraries to link against
  set(LIBS ${LIBS} ${Splash_LIBRARIES})
endif(Splash_FOUND)

# add_executable(yourBinary ${SOURCES})
# ...
# target_link_libraries(yourBinary ${LIBS})
```

Examples
--------

To build the examples in the `examples` subdirectory, just run the following
commands in an empty build directory:

```bash
cmake -DWITH_MPI=ON <yourPathTo>/libSplash/examples/
make
```

`WITH_MPI` will enable/disable MPI based tests. Parallel libSplash examples
will require a parallel HDF5 install.

```bash
# MPI based serial (posix) writer
#   -> creates four h5_X_Y_Z.h5 files
#      depending on MPI topology;
#      iterations are appended in these files
mpiexec -n 4 ./domain_write_mpi.cpp.out h5 2 2 1

# MPI based serial (posix) reader
#    -> reads h5_X_Y_Z.h5 files
mpiexec -n 4 ./domain_read_mpi.cpp.out h5 2 2 1
# serial (posix) reader
#   -> reads h5_X_Y_Z.h5 files
./domain_read.cpp.out h5

# MPI based parallel (MPI-I/O) writer
#   -> creates ph5_10.h5 (iteration = 10);
#      each iteration creates a new file but
#      MPI parallel chunks of the same iteration
#      are aggregated to one file
mpiexec -n 4 ./parallel_domain_write.cpp.out ph5 2 2 1
```


Tests
-----

libSplash includes several tests in the `tests` subdirectory.
Tests differ for the serial and parallel version (see below) of libSplash.
You can build the tests by running cmake on the `tests/CMakeLists.txt` file.
Tests can be run using the `run_tests` and `run_parallel_tests` scripts.

`run_parallel_tests` runs tests for the parallel libSplash version.
See section *Parallel libSplash* for details.


Documentation
-------------

You can create your own version of the HTML documentation by running
`$ doxygen` on the `doc/Doxyfile`.


Parallel libSplash
------------------

libSplash has support for parallel I/O creating a single, aggregated file per iteration.
To build the parallel version, you need to build a parallel HDF5 library first.
Commonly, it should be sufficient to configure HDF5 `$ configure --enable-shared --enable-parallel ...`.
Running [cmake](#Compiling), libSplash will automatically detect the parallel HDF5
capabilities and informs you with a status message
`Parallel HDF5 found. Building parallel version`.

This builds the *ParallelDataCollector* and *ParallelDomainCollector* classes.
See
[IParallelDataCollector](https://computationalradiationphysics.github.io/libSplash/classsplash_1_1_i_parallel_data_collector.html)
and
[IParallelDomainCollector](https://computationalradiationphysics.github.io/libSplash/classsplash_1_1_i_parallel_domain_collector.html)
interfaces for further information on how to use the parallel version.
