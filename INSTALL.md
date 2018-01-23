libSplash Installation Instructions
===================================

Requirements
------------

**Building** libSplash from source requires to checkout the trunk via Git via the command
```bash
git clone https://github.com/ComputationalRadiationPhysics/libSplash.git
```

Building libSplash **requires HDF5 in version 1.8.6** or higher and **zlib**.
For **MPI based parallel output** configure HDF5 with `--enable-parallel`.

For building from source you must have **CMake version 3.10.0** or higher installed.

All MPI functionality requires at least a **MPI 2.2** compatible MPI library, e.g. **OpenMPI 1.5.1** or higher.

Our *tests* require the development version of the **CppUnit** library and python support for `h5py` & `numpy`.


Compiling
---------

To **compile libSplash**, it is recommended to create a new build folder and execute
```bash
cmake -DCMAKE_INSTALL_PREFIX=<INSTALL PATH> <PATH-to-dir-of-libSplash>
```

The following options can be added to the `cmake` call to control libSplash features:

| CMake Option           | Values           | Description                                       |
|------------------------|------------------|---------------------------------------------------|
| Splash_USE_MPI         | **AUTO**/ON/OFF  | Enable MPI support (does NOT imply parallel HDF5) |
| Splash_USE_PARALLEL    | **AUTO**/ON/OFF  | Enable support for parallel HDF5 (implies MPI)    |
| Splash_HAVE_COLLECTIVE | **ON**/OFF       | Enable collective I/O for parallel HDF5           |
| Splash_HAVE_TOOLS      | **ON**/OFF       | Enable tools                                      |
| Splash_HAVE_TESTS      | ON/**OFF**       | Enable tests                                      |

Than run `$ make install` to build and install libSplash library and binaries to the chosen installation directory.
By default, this will build libSplash as a static library (`libSplash.a`) and installs also its headers.
In order to build a static library, append `-DBUILD_SHARED_LIBS=ON` to the `cmake` command.
You can only build a static or a shared library at a time.

By default, the `Release` version is built.
In order to build libSplash with debug symbols, pass `-DCMAKE_BUILD_TYPE=Debug` to your `cmake` command.

Afterwards, configure your environment variables as follows if you installed libSplash in a non-system path:
```bash
# helps finding the installed library for CMake scripts
export CMAKE_PREFIX_PATH=<INSTALL PATH>:$CMAKE_PREFIX_PATH

# optional
#   provides command line access to our tools
export PATH=<INSTALL PATH>/bin:$PATH
#   path for the linker if you link a dependent projectwithout CMake
export LD_LIBRARY_PATH=$SPLASH_ROOT/lib:$LD_LIBRARY_PATH
#   provides our python modules, e.g. for xdmf creation
export PYTHONPATH=$SPLASH_ROOT/bin:$PYTHONPATH
```


Linking to your Project
-----------------------

To use libSplash in your project, you must link against the created statically linked archive `libSplash.a` (or the shared object library `libSplash.so`).

Because we are linking to HDF5, the following **external dependencies** must be linked:
- `-lhdf5`, `-lpthread`, `-lz`, `-lrt`, `-ldl`, `-lm` and with MPI `-lmpi`

If you are using CMake and did extend the `CMAKE_PREFIX_PATH`, linking against libSplash in your `CMakeLists.txt` is as easy as:
```cmake
#   optional: specifiy (minimal) version, require Splash and specific components, e.g.
#           (Splash 1.7.0 REQUIRED CONFIG COMPONENTS PARALLEL)
find_package(Splash 1.7.0 REQUIRED CONFIG)

target_link_libraries(YourTarget PUBLIC Splash::Splash)
```

Examples
--------

Examples from the `examples/` directory are automatically build with libSplash and can be found in the build directory.

`-DSplash_USE_MPI=ON` will *require* MPI based examples.
`-DSplash_USE_PARALLEL=ON` will *require* Parallel HDF5 based examples.
Parallel libSplash examples will require a parallel HDF5 install.


Run the examples via:

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

Examples are not installed on `make install`.


Tests
-----

libSplash includes several tests in the `tests/` subdirectory.
Tests differ for the serial and parallel version (see below) of libSplash.
You can build the tests by passing `-DSplash_HAVE_TESTS=ON` to CMake.

Tests require the following additional dependencies:

* CPPUnit
* Python with (serial) h5py installed

Tests can be run via:

```bash
make test
```


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
