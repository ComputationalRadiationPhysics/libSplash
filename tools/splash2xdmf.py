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
import glob
import argparse
from xml.dom.minidom import Document

SPLASH_CLASS_NAME = "_class"
SPLASH_CLASS_TYPE_POLY = 10
SPLASH_CLASS_TYPE_GRID = 20

verbosity_level = 0

doc = Document()
grids = dict()

# String functions

def log(msg, linebreak=True, verbLevel=1):
    if verbosity_level >= verbLevel:
        if linebreak:
            print msg
        else:
            print msg,

def get_common_filename(filename):
    index1 = filename.rfind(".h5")
    if index1 == -1:
        return filename
    else:
        index2 = filename[0:index1].rfind("_")
        if index2 == -1:
            return filename
        else:
            return filename[0:index2]
        

# strip /data/<timestep> from dset_name
def get_attr_name(dset_name):
    index = dset_name.find("/", len("/data/"))
    if index == -1:
        return dset_name
    else:
        return dset_name[(index+1):]


# XDMF/XML output functions

# return an existing xml grid node for dims or create a new one
# and insert it into the map of grid nodes
def get_create_grid_node(dims, ndims):
    if dims in grids:
        return grids[dims]
    else:
        new_grid = doc.createElement("Grid")
        new_grid.setAttribute("GridType", "Uniform")
        
        topology = doc.createElement("Topology")
        topology.setAttribute("TopologyType", "3DCoRectMesh")
        topology.setAttribute("Dimensions", "{}".format(dims))
        new_grid.appendChild(topology)
        
        dims_start = ""
        dims_end = ""

        for i in range(ndims):
            dims_start += "0.0"
            dims_end += "1.0"
            if i < ndims - 1:
                dims_start += " "
                dims_end += " "
        
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
        new_grid.appendChild(geometry)
        
        grids[dims] = new_grid
        return new_grid


# create an xdmf grid attribute for an hdf5 grid-type dataset
def create_xdmf_grid_attribute(dset, level, h5filename):
    log("Creating XDMF entry for {}".format(dset.name), False)
    ndims = get_ndims(dset)
    dims = get_dims(dset)
    
    grid = get_create_grid_node(dims, ndims)
    
    attribute = doc.createElement("Attribute")
    attribute.setAttribute("Name", get_attr_name(dset.name))
    
    data_item_attr = doc.createElement("DataItem")
    data_item_attr.setAttribute("Dimensions", "{}".format(dims))
    data_item_attr.setAttribute("NumberType", "float")
    data_item_attr.setAttribute("Precision", "4")
    data_item_attr.setAttribute("Format", "HDF")
    
    data_item_attr_text = doc.createTextNode("{}:{}".format(h5filename, dset.name))
    data_item_attr.appendChild(data_item_attr_text)
    
    attribute.appendChild(data_item_attr)
    
    grid.appendChild(attribute)
    

def create_timestep_node(time):
    time_node = doc.createElement("Time")
    time_node.setAttribute("TimeType", "Single")
    time_node.setAttribute("Value", "{}".format(time))
    return time_node


# HDF5 functions

# get the number of dimensions of an hdf5 dataset
def get_ndims(dset):
    return len(dset.dims)

# get dimensions as string for an hdf5 dataset
def get_dims(dset):
    ndims = get_ndims(dset)
    dims = ""
    for i in range(ndims):
        dims += "{}".format(dset.shape[i])
        if i < ndims - 1:
            dims += " "

    return dims

def eval_class_type(classType, dset, level, h5filename):
    if classType == SPLASH_CLASS_TYPE_GRID:
        log("GRID", False)
        create_xdmf_grid_attribute(dset, level, h5filename)
    if classType == SPLASH_CLASS_TYPE_POLY:
        log("POLY", False)


def print_dataset(dset, level, h5filename):
    for attr in dset.attrs.keys():
        if attr == SPLASH_CLASS_NAME:
            eval_class_type(dset.attrs.get(attr), dset, level, h5filename)
    log("")


def parse_hdf5_recursive(h5Group, h5filename, level):
    groups = h5Group.keys()
    for g in groups:
        log("-" * level, False)
        log(g, False)
        objClass = h5Group.get(g, None, True)
        if objClass == h5py.Group:
            log("(G)")
            parse_hdf5_recursive(h5Group[g], h5filename, level+1)
        else:
            log("(D):", False)
            print_dataset(h5Group[g], level, h5filename)
    

def get_timestep_for_group(dataGroup):
    groups = dataGroup.keys()
    if len(groups) != 1:
        print("Error: data group is expected to have a single timestep child group")
        return -1
    
    objClass = dataGroup.get(groups[0], None, True)
    if objClass != h5py.Group:
        print("Error: data group child is expected to be of object class Group")
        return -1
        
    index = groups[0].rfind("/")
    return groups[0][(index+1):]
    

def create_xdmf_for_splash_file(base_node, splashFilename):
    if not os.path.isfile(splashFilename):
        print "Error: '{}' does not exist.".format(splashFilename)
        sys.exit(1)
        
    # open libSplash file
    h5file = h5py.File(splashFilename, "r")
    log("Opened libSplash file '{}'".format(h5file.filename))
    
    # find /data group
    dataGroup = h5file.get("data")
    if dataGroup == None:
        print "Error: Could not find root data group in '{}'.".format(splashFilename)
        h5file.close()
        return False
    
    time_step = get_timestep_for_group(dataGroup)
    
    parse_hdf5_recursive(dataGroup, splashFilename, 1)
    # append all collected grids to current xml base node
    index = 0
    for grid in grids.values():
        grid.setAttribute("Name", "Grid_{}_{}".format(time_step, index))
        grid.appendChild(create_timestep_node(time_step))
        base_node.appendChild(grid)
        index += 1
 
    grids.clear()
    
    # close libSplash file
    h5file.close()
    return True


# program functions

# creates the xdmf xml structure in the current document for all libSplash
# files in splash_files_list
def create_xdmf_xml(splash_files_list):
    time_series = (len(splash_files_list) > 1)
    
    # setup xml structure
    xdmf_root = doc.createElement("Xdmf")
    domain = doc.createElement("Domain")
    base_node = domain
    
    if time_series:
        main_grid = doc.createElement("Grid")
        main_grid.setAttribute("Name", "Grids")
        main_grid.setAttribute("GridType", "Collection")
        main_grid.setAttribute("CollectionType", "Temporal")
        base_node = main_grid
    
    for current_file in splash_files_list:
        # parse this splash file and append to current xml base node
        create_xdmf_for_splash_file(base_node, current_file)

    # finalize xml structure
    if time_series:
        domain.appendChild(main_grid)

    xdmf_root.appendChild(domain)
    doc.appendChild(xdmf_root)
    return xdmf_root


def write_xml_to_file(filename, document):
    # write xml to xdmf file
    xdmf_file = open(filename, "w")
    xdmf_file.write(document.toprettyxml())
    xdmf_file.close()
    log("Created XDMF file '{}'".format(filename), True, 0)
    
    
def get_args_parser():
    parser = argparse.ArgumentParser(description="Create a XDMF meta description file from a libSplash HDF5 file.")

    parser.add_argument("splashfile", metavar="<filename>",
        help="libSplash HDF5 file with domain information")
        
    parser.add_argument("-o", metavar="<filename>", help="Name of output XDMF file (default: append '.xmf')")
        
    parser.add_argument("-v", "--verbose", help="Produce verbose output", action="store_true")
    
    parser.add_argument("-t", "--time", help="Aggregate information over a time-series of libSplash data", action="store_true")
        
    return parser


# main

def main():
    global verbosity_level

    # get arguments from command line
    args_parser = get_args_parser()
    args = args_parser.parse_args()

    # apply arguments
    splashFilename = args.splashfile
    time_series = args.time
    if args.verbose:
        verbosity_level = 100

    # create the list of requested splash files
    splash_files = list()
    if time_series:
        splashFilename = get_common_filename(splashFilename)
        for s_filename in glob.glob("{}_*.h5".format(splashFilename)):
            splash_files.append(s_filename)
    else:
        splash_files.append(splashFilename)
        
    create_xdmf_xml(splash_files)
    
    output_filename = "{}.xmf".format(splashFilename)
    if args.o:
        output_filename = args.o
    write_xml_to_file(output_filename, doc)

if __name__ == "__main__":
    main()
