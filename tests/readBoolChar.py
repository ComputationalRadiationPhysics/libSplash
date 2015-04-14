#!/usr/bin/env python
#
# Copyright 2015 Axel Huebl
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
#

import h5py
import numpy as np

# bool compatible data sets
f = h5py.File("h5/testWriteRead_0_0_0.h5", "r")
data = f["data/10/deep/folders/data_bool"]

len = data.size
data1d = data[:,:,:].reshape(len)

for i in np.arange(len):
    val = ( i%2 == 0 );
    if data1d[i] != val:
        exit(1)

f.close()

# single char compatible attributes
f = h5py.File("h5/attributes_0_0_0.h5", "r")
c = f["data/0/datasets"].attrs["my_char"]

# h5py, as of 2.5.0, does not know char and
# identifies H5T_NATIVE_CHAR as INT8
print(c, type(c))

# is...
if type(c) is np.int8:
    if not c == 89:
        exit(1)

# should be...
if type(c) is np.char:
    if not c == "Y":
        exit(1)

# variable length string compatible attributes
s = f["/data/10"].attrs["my_string"]
print(s, type(s))

if not s == "My first c-string.":
    exit(1)

if not type(s) is str:
    exit(1)

# fixed length string compatible attributes
s5 = f["/data/10"].attrs["my_string5"]
print(s5, type(s5), s5.dtype)

if not s5 == "ABCD":
    exit(1)

if not type(s5) is np.string_:
    exit(1)


f.close()

exit(0)
