#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2014 Felix Schmitt
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

import sys
import h5py
import os.path
import argparse
from xml.dom.minidom import Document

SPLASH_CLASS_NAME = "_class"
SPLASH_CLASS_TYPE_POLY = 10
SPLASH_CLASS_TYPE_GRID = 20

verbosity_level = 0

doc = Document()

# helper functions

def log(msg, linebreak=True, verbLevel=1):
    if verbosity_level >= verbLevel:
        if linebreak:
            print msg
        else:
            print msg,


# XDMF output functions

def get_ndims(dset):
    return len(dset.dims)


def get_dims(dset):
    ndims = get_ndims(dset)
    dims = ""
    for i in range(ndims):
        dims += "{}".format(dset.shape[i])
        if i < ndims - 1:
            dims += " "

    return dims


def print_xdmf_grid(dset, level, h5filename, xdmf):
    log("Creating XDMF entry for {}".format(dset.name), False)
    ndims = get_ndims(dset)
    dims = get_dims(dset)
    dims_start = ""
    dims_end = ""
    
    for i in range(ndims):
        dims_start += "0.0"
        dims_end += "1.0"
        if i < ndims - 1:
            dims_start += " "
            dims_end += " "
    
    grid = doc.createElement("Grid")
    grid.setAttribute("Name", dset.name)
    grid.setAttribute("GridType", "Uniform")
    
    topology = doc.createElement("Topology")
    topology.setAttribute("TopologyType", "3DCoRectMesh")
    topology.setAttribute("Dimensions", "{}".format(dims))
    
    geometry = doc.createElement("Geometry")
    geometry.setAttribute("Type", "ORIGIN_DXDYDZ")
    
    data_item_origin = doc.createElement("DataItem")
    data_item_origin.setAttribute("Format", "XML")
    data_item_origin.setAttribute("Dimensions", "{}".format(ndims))
    data_item_origin_text = doc.createTextNode(dims_start)
    data_item_origin.appendChild(data_item_origin_text)
    
    data_item_d = doc.createElement("DataItem")
    data_item_d.setAttribute("Format", "XML")
    data_item_d.setAttribute("Dimensions", "{}".format(ndims))
    data_item_d_text = doc.createTextNode(dims_end)
    data_item_d.appendChild(data_item_d_text)
    
    geometry.appendChild(data_item_origin)
    geometry.appendChild(data_item_d)
    
    attribute = doc.createElement("Attribute")
    attribute.setAttribute("Name", dset.name)
    
    data_item_attr = doc.createElement("DataItem")
    data_item_attr.setAttribute("Dimensions", "{}".format(dims))
    data_item_attr.setAttribute("NumberType", "float")
    data_item_attr.setAttribute("Precision", "4")
    data_item_attr.setAttribute("Format", "HDF")
    
    data_item_attr_text = doc.createTextNode("{}:{}".format(h5filename, dset.name))
    data_item_attr.appendChild(data_item_attr_text)
    
    attribute.appendChild(data_item_attr)
    
    grid.appendChild(topology)
    grid.appendChild(geometry)
    grid.appendChild(attribute)
    xdmf.appendChild(grid)
    
    
def print_xdmf_header(xdmf):
    domain = doc.createElement("Domain")
    grid = doc.createElement("Grid")
    
    grid.setAttribute("Name", "Box")
    grid.setAttribute("GridType", "Collection")
    
    domain.appendChild(grid)
    xdmf.appendChild(domain)
    return grid


# HDF5 parsing functions

def eval_class_type(classType, dset, level, h5filename, xdmf):
    if classType == SPLASH_CLASS_TYPE_GRID:
        log("GRID", False)
        print_xdmf_grid(dset, level, h5filename, xdmf)
    if classType == SPLASH_CLASS_TYPE_POLY:
        log("POLY", False)


def print_dataset(dset, level, h5filename, xdmf):
    for attr in dset.attrs.keys():
        if attr == SPLASH_CLASS_NAME:
            eval_class_type(dset.attrs.get(attr), dset, level, h5filename, xdmf)
    log("")


def print_hdf5_recursive(h5Group, h5filename, xdmf, level):
    groups = h5Group.keys()
    for g in groups:
        log("-" * level, False)
        log(g, False)
        objClass = h5Group.get(g, None, True)
        if objClass == h5py.Group:
            log("(G)")
            print_hdf5_recursive(h5Group[g], h5filename, xdmf, level+1)
        else:
            log("(D):", False)
            print_dataset(h5Group[g], level, h5filename, xdmf)


def get_args_parser():
    parser = argparse.ArgumentParser(description="Create a XDMF meta description file from a libSplash HDF5 file.")

    parser.add_argument("splashfile", metavar="<libSplash file>",
        help="libSplash HDF5 file with domain information")
        
    parser.add_argument("-v", help="Produce verbose output", action="store_true")
        
    return parser

# main

def main():
    global verbosity_level

    # get arguments from command line
    args_parser = get_args_parser()
    args = args_parser.parse_args()

    # apply arguments
    splashFilename = args.splashfile
    if args.v:
        verbosity_level = 100
    
    if not os.path.isfile(splashFilename):
        print "Error: '{}' does not exist.".format(splashFilename)
        sys.exit(1)

    # open libSplash file
    h5file = h5py.File(splashFilename, "r")
    log("Opened libSplash file '{}'".format(h5file.filename))

    # find /data group
    dataGroup = h5file.get("data")
    if dataGroup == None:
        print "Error: Could not find root data group."
    else:
        xdmf_filename = "{}.xmf".format(splashFilename)
        
        xdmf_file = open(xdmf_filename, "w")
        xdmf_root = doc.createElement("Xdmf")
        grid = print_xdmf_header(xdmf_root)
        print_hdf5_recursive(dataGroup, splashFilename, grid, 1)
        doc.appendChild(xdmf_root)
        xdmf_file.write(doc.toprettyxml())
        xdmf_file.close()
        log("Created XDMF file '{}'".format(xdmf_filename), True, 0)

    # close libSplash file
    h5file.close()
    

main()
