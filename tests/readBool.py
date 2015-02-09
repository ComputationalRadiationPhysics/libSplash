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

f = h5py.File("h5/testWriteRead_0_0_0.h5", "r")
data = f["data/10/deep/folders/data_bool"]

len = data.size
data1d = data[:,:,:].reshape(len)

for i in np.arange(len):
    val = ( i%2 == 0 );
    if data1d[i] != val:
        exit(1)

exit(0)
