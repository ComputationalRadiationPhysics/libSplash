Change Log for libSplash
================================================================

Release 1.7.0
-------------
**Date:** 2018-01-22

This release adds modernized CMake scripts including CMake config
packages on install with targets. A new `readAttributeInfo` method
was added to the `DataCollector` for save reading of types and
extents of attributes. On reads, the `offset` in `DataContainer` for
non-zero offsets in domains was fixed. Due to a bug in upsteam CMake,
at least version 3.10.0 of CMake is required now.

**Interface Changes**

 - new read method: attribute meta info #248 #268
 - keep info on H5 datatypes in collection types #259

**Bug Fixes**

 - `DataContainer`: fix offset read #263
 - `FindHDF5.cmake`: use upstream CMake version #267

**Misc**

 - modern CMake3 scripts #269
 - fix GitHub badges #254 #264
 
Thanks to Axel Huebl and Alexander Grund for contributions to this release!


Release 1.6.0
-------------
**Date:** 2016-10-28

`SerialDataCollector` file name interface change: this feature
was accidentally not included in the `1.5.0` release.

**Interface Changes**

 - `SerialDataCollector` file name interface changed #242

**Bug Fixes**

 - compile with GCC 6.2 was broken: ambiguous `abs` #255

**Misc**

 - tests: remove warning on deprecated CMake usage in 3.1+ #242


Release 1.5.0
-------------
**Date:** 2016-10-26

Written attributes do now generate their according group path
recursively if it does not exist and the `SerialDataCollector`
interface for file names has been changed to be more consistent.

**Interface Changes**

 - `SerialDataCollector` file name interface changed #242
 - `writeAttribute()` now creates missing groups recursively #231 #250

**Bug Fixes**

 - throw more exceptions on wrong usage of `ParallelDataCollector` #247

**Misc**

 - compiling via `-Werror` is not shipped any more #251
 - python tools: indentiation cleanup #249
 - `generateCollectionType` refactored, docs fixed #243 #246


Release 1.4.0
-------------
**Date:** 2016-04-12

The `SerialDataCollector` now also writes global attributes
to `/`, allowing serial files to fulfill the openPMD standard.

**Interface Changes**

 - `SerialDataCollector::writeGlobalAttribute` now writes to `/`
   instead of `/custom` (follow-up to #182) #229


Release 1.3.1
-------------
**Date:** 2016-04-12

This release contains bug fixes and serveral internal code
clean-ups.

**Bug Fixes**

 - `readMeta` now returns the correct `CollectionType` if more then
   one entry was used per iteration, detection for `ColTypeBool` was
   broken #224
 - exception parameters are now catched consequently by const
   reference #213


**Misc**

 - remove tabs and EOL white spaces #216 #217 #218
 - remove `_` prefixes from a few include guards #214
 - add example for the `ParallelDataCollector` class #14
 - remove beta notice for parallel HDF5 support #209
 - update travis-ci script #212


Release 1.3.0
-------------
**Date:** 2015-11-12

This release adds functionality for users to fulfill formatting for
`openPMD`. Support for array attributes, strings/chars, bools and
further `h5py` compatibility have been added. Additionally, read
functionality has improved by new meta calls to determine type
and size of a data set before reading.
The internal format was increased to version 3.3

**New Features**

 - bool types are now h5py compatible #153 #198
 - new interface `readMeta` in `DataCollector`s to determine type 
   and size before reading #203
 - new interfaces for `DataCollector`s `writeGlobalAttribute` to support
   arrays via HDF5 simple data spaces (h5py compatible) #170 #171
 - `splashVersion` and `splashFormat` are now written to `/header` #183
 - header define `SPLASH_HDF5_VERSION` remembers HDF5 version of build #177
 - `char`s, fixed and variable length `string` support added #167


**Interface Changes**

 - `ParallelDataCollector::writeGlobalAttribute` now writes to `/` instead of
   `/custom` (`SerialDataCollector` unchanged) #182


**Misc**

 - term "iteration" is now consequently preferred over
   "time step" #157 #187 #188 #204
 - public includes do not throw on `-Wshadow` any more #201
 - `/header/compression` attribute in new bool representation #199
 - CMake:
   - shared library detection refactored #192
   - FindHDF5 shipped with version support (pre CMake 3.3.0) #176 #169
 - doxygen project name updated #178
 - `DataSpace` refactored: prefer `DSP_DIM_MAX` over magic numbers #175
 - test scripts
   - CI (travis) false-positive fixed #172
   - output improved #166
   - warn on missing dir argument #185
 - `INSTALL.md` now in project's root directory #158


Release 1.2.4
-------------
**Date:** 2015-01-25

This release fixes a bug with parallel NULL reads.

**Bug Fixes**

 - Fix Null-Access Parallel Read Hang #148
 - Fix compile error on Red Hat #147


Release 1.2.3
-------------
**Date:** 2014-10-14

This release contains a minor fix for listing entries and groups.

**Misc**

 - Remove / from datasets when listing entries #143


**Bug Fixes**

 - Fix bug when querying DCGroup entries #142


Release 1.2.2
-------------
**Date:** 2014-06-21

That release was necessary since we forgot to bump the version
number in <splash/version.hpp> with the last release.


Release 1.2.1
-------------
**Date:** 2014-06-20

This release contains a minor improvement in the data format.
The internal format was increased to version 2.1 (which is
backwards compatible to all 2.x formated files).

**Misc**

 - parallel simple data test re-enabled #130
 - data sets: use fixed size (non-extensible) by default,
   use `H5F_UNLIMTED` only in append mode #135
 - update install notes #134


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
