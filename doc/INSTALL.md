libSplash Installation Instructions
===================================

Requirements
------------

**Building** libSplash from source requires to checkout the trunk via Git
using the command
`$ git clone git@github.com:ComputationalRadiationPhysics/libSplash.git`
Building libSplash **requires HDF5** in version 1.8.6 or higher.
To use the CMakeLists.txt file which comes with the source code, you must have **CMake version 2.6**
or higher installed.


Compiling
---------

To **compile libSplash**, it is recommended to create a new build folder and execute
`$ cmake CMAKE INSTALL PREFIX=<INSTALL PATH> <CMakeLists.txt-PATH>`

Than run `$ make install` to build and install libSplash library and binaries to the chosen
installation directory. This will build libSplash as an shared object library (libsplash.so)
as well as all tools.

By default, the RELEASE version is built. To create libSplash with DEBUG symbols,
pass `-DSPLASH_RELEASE=OFF` to your cmake command.
To see verbose internal (!) debug output for libSplash, pass `-DSPLASH_RELEASE=OFF -DDEBUG_VERBOSE=ON`
to your cmake command line.

Linking
------

To use libSplash in your project, you must link against the created shared object library
libsplash.so or against the statically linked archive libsplash.a.

