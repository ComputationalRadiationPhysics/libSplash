Change Log for libSplash
================================================================

Release 1.2.0
-------------
**Date:** 2014-04-30

**New Features**

 - splash2xdmf python script to create XDMF descriptions for libSplash files
   with domain information
 - allow to read/write attributes at groups


**Interface Changes**

 - major interface change: simplified interfaces as most read/write routines
   now use a Selection class for describe the source layout
 - local domain offsets are now exclusive, i.e. do not include the
   global domain offset
 - changed file format version to 2.0
 - add a finalize() call to ParallelDataCollector to free MPI resources


**Bug Fixes**

 - fix automatic chunking algorithm
 - only use HDF5 hyperslaps if necessary for improved performance
 - fix bug in getMaxID and getEntryIDs for ParallelDataCollector



Release 1.1.1
-------------
**Date:** 2014-02-20

**Bug Fixes**

 - Allow to write/read attributes at groups #99


Release 1.1.0
-------------
**Date:** 2014-01-23

**New Features**

 - Added support for various new collection types.
 - Added macros to generate array and compound collection types.
 - Enabled support for using static HDF5 library.
 - Added patch level and file format version information.
 - Updated user manual for parallel I/O.
 - New tests and examples.


**Interface Changes**

 - Header files moved to splash subfolder (include/splash/splash.h). 


**Bug Fixes**

 - Create duplicate of user-provided MPI communicator.
 - Allow empty write/read calls (no data).
 - Improved HDF5 detection in cmake file.
 - Fix usage of MPI data types.


**Misc:**

 - Compiling with OpenMPI now requires version 1.5.1 or higher
