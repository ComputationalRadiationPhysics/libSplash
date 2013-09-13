libSplash Installation Instructions
===================================

Requirements
------------

**Building** libSplash from source requires to checkout the trunk via Git
using the command
`$ git clone git@github.com:ComputationalRadiationPhysics/libSplash.git`

Building libSplash **requires HDF5 in version 1.8.6** or higher with **shared library support**
(`--enable-shared`).
However, we recommend that you use HDF5 1.8.11 or newer for best support and results.

To use the CMakeLists.txt file which comes with the source code, you must have
**CMake version 2.8.5** or higher installed.

The splashtools and some tests also require an **OpenMPI** compatible MPI library.

Tests require the development version of the **CppUnit** library.


Compiling
---------

To **compile libSplash**, it is recommended to create a new build folder and execute
`$ cmake CMAKE INSTALL PREFIX=<INSTALL PATH> <CMakeLists.txt-PATH>`

Than run `$ make install` to build and install libSplash library and binaries to the chosen
installation directory. This will build libSplash as an shared object library (libsplash.so)
as well as all tools.

If you do not want to build splashtools, pass `-DWITH_TOOLS=OFF` to your cmake command.
To enable MPI parallel splashtools, pass `-DTOOLS_MPI=ON` to your cmake command.

By default, the RELEASE version is built. To create libSplash with DEBUG symbols,
pass `-DSPLASH_RELEASE=OFF` to your cmake command.
To see verbose internal (!) debug output for libSplash, pass `-DSPLASH_RELEASE=OFF -DDEBUG_VERBOSE=ON`
to your cmake command line.


Linking
-------

To use libSplash in your project, you must link against the created shared object library
libsplash.so or against the statically linked archive libsplash.a.


Tests
-----

libSplash includes several tests in the `tests` subdirectory.
Tests differ for the serial and parallel version (see below) of libSplash.
You can build the tests by running cmake on the `tests/CMakeLists.txt` file.
Tests can be run using the `run_tests` and `run_parallel_tests` scripts.

`run_parallel_tests` runs tests for the parallel libSplash version and
requires the tests to be build with `$ cmake -DPARALLEL=ON`.
See section *Parallel libSplash* for details.


Documentation
-------------

You can create your own version of the HTML documentation by running
`$ doxygen` on the `doc/Doxyfile`.


Parallel libSplash
------------------

libSplash has **experimental** support for parallel I/O.
To build the parallel version, you need to build a parallel HDF5 library first.
Commonly, it should be sufficient to configure HDF5 `$ configure --enable-shared --enable-parallel ...`.
Compile the parallel libSplash with `$ cmake -DPARALLEL=ON ...`.

This builds the *ParallelDataCollector* and *ParallelDomainCollector* classes.
*Please note that this feature is experimental and not fully tested!*
See *IParallelDataCollector* and *IParallelDomainCollector* interfaces for further
information on how to use the parallel version.

