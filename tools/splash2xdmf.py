#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2014-2015 Felix Schmitt, Conrad Schumann, Axel Huebl, 
#                     Richard Pausch
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
import numpy as np
import os.path
import glob
import argparse
from xml.dom.minidom import Document

SPLASH_CLASS_NAME = "_class"
SPLASH_CLASS_TYPE_POLY = 10
SPLASH_CLASS_TYPE_GRID = 20

verbosity_level = 0

doc = Document()
grid_doc = Document()
poly_doc = Document()
grids = dict()
polys = dict()

# String functions

def log(msg, linebreak=True, verbLevel=1):
    """
    Print log message if verbosity level is high enough.
    
    Parameters:
    ----------------
    msg:       string
               log message
    linebreak: bool
               print linebreak after message
    verbLevel: int
               required verbosity level
    """

    if verbosity_level >= verbLevel:
        if linebreak:
            print msg
        else:
            print msg,


def get_common_filename(filename):
    """
    Return common part of HDF5 filename
    
    Parameters:
    ----------------
    filename: string
              libSplash HDF5 filename
    Returns:
    ----------------
    return: string
            common filename part
    """

    index1 = filename.rfind(".h5")
    if index1 == -1:
        return filename
    else:
        index2 = filename[0:index1].rfind("_")
        if index2 == -1:
            return filename
        else:
            return filename[0:index2]
        

def get_attr_name(dset_name):
    """
    Return name part of HDF5 dataset without /data/<iteration>
    
    Parameters:
    ----------------
    dset_name: string
               fully qualified libSplash HDF5 dataset name 
    Returns:
    ----------------
    return: string
            dataset name part with /data/<iteration> stripped
    """

    index = dset_name.find("/", len("/data/"))
    if index == -1:
        return dset_name
    else:
        return dset_name[(index+1):]


# XDMF/XML output functions

def get_create_grid_node(dims, ndims):
    """
    Return an existing XML grid node for dims or create a new one
    and insert it into the map of grid nodes.
    
    Parameters:
    ----------------
    dims:  string
           space-separated dimensions
    ndims: int
           number of dimensions of dims
    Returns:
    ----------------
    return: Element
            new or existing XML Grid node 
    """

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
        geom_type = "ORIGIN_DXDYDZ"
        if ndims == 2:
            geom_type = "ORIGIN_DXDY"

        for i in range(ndims):
            dims_start += "0.0"
            dims_end += "1.0"
            if i < ndims - 1:
                dims_start += " "
                dims_end += " "
        
        geometry = doc.createElement("Geometry")
        geometry.setAttribute("Type", geom_type)

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


def create_xdmf_grid_attribute(dset, h5filename):
    """
    Create a XDMF grid attribute for an HDF5 grid-type dataset
    
    Parameters:
    ----------------
    dset:  h5py Dataset
           HDF5 dataset with grid domain information.
    h5filename: string
                HDF5 filename 
    """

    log("Creating XDMF entry for {}".format(dset.name), False)
    ndims = get_ndims(dset)
    dims = get_dims(dset)
    (dtype, prec) = get_datatype_and_prec(dset)
    if dtype == None or prec == None:
        print "Error: Unsupported data type or precision for '{}'".format(dset.name)
        return
    
    grid = get_create_grid_node(dims, ndims)
    
    attribute = doc.createElement("Attribute")
    attribute.setAttribute("Name", get_attr_name(dset.name))

    for (attr_name, attr_value) in dset.attrs.items():
	if not attr_name.startswith("_"):
	    if get_datatype_and_prec(dset.attrs.get(attr_name)) != (None, None):
		 information = doc.createElement("Information")
   		 information.setAttribute("Name", attr_name)
	         information.setAttribute("Value", "{}".format(attr_value))
   		 attribute.appendChild(information)
 
 
    data_item_attr = doc.createElement("DataItem")
    data_item_attr.setAttribute("Dimensions", "{}".format(dims))
    data_item_attr.setAttribute("NumberType", dtype)
    data_item_attr.setAttribute("Precision", prec)
    data_item_attr.setAttribute("Format", "HDF")
   
    data_item_attr_text = doc.createTextNode("{}:{}".format(h5filename, dset.name))
    data_item_attr.appendChild(data_item_attr_text)
    
    attribute.appendChild(data_item_attr)
    
    grid.appendChild(attribute)
    
    
def get_create_poly_node(dims):
    """
    Return an existing XML grid node for dims or create a new one
    and insert it into the map of poly nodes.
    
    Parameters:
    ----------------
    dims:  string
           space-separated dimensions
    ndims: int
           number of dimensions of dims
    Returns:
    ----------------
    return: Element
            new or existing XML Poly node 
    """

    if dims in polys:
        return polys[dims]
    else:
        new_poly = doc.createElement("Grid")
        new_poly.setAttribute("GridType", "Uniform")
        
        topology = doc.createElement("Topology")
        topology.setAttribute("TopologyType", "Polyvertex")
        topology.setAttribute("NodesPerElement", "{}".format(dims))
        new_poly.appendChild(topology)
        
        polys[dims] = new_poly
        return new_poly


def create_xdmf_poly_attribute(dset, h5filename):
    """
    Create a XDMF grid attribute for an HDF5 poly-type dataset
    
    Parameters:
    ----------------
    dset:  h5py Dataset
           HDF5 dataset with poly domain information.
    h5filename: string
                HDF5 filename 
    """

    log("Creating XDMF entry for {}".format(dset.name), False)
    dims = get_dims(dset)
    (dtype, prec) = get_datatype_and_prec(dset)
    if dtype == None or prec == None:
        print "Error: Unsupported data type or precision for '{}'".format(dset.name)
        return
    
    poly = get_create_poly_node(dims)
    
    attribute = doc.createElement("Attribute")
    attribute.setAttribute("Name", get_attr_name(dset.name))

    for (attr_name, attr_value) in dset.attrs.items():
	if not attr_name.startswith("_"):
	    if get_datatype_and_prec(dset.attrs.get(attr_name)) != (None, None):
		 information = doc.createElement("Information")
   		 information.setAttribute("Name", attr_name)
	         information.setAttribute("Value", "{}".format(attr_value))
   		 attribute.appendChild(information)
   

    data_item_attr = doc.createElement("DataItem")
    data_item_attr.setAttribute("Dimensions", "{}".format(dims))
    data_item_attr.setAttribute("NumberType", dtype)
    data_item_attr.setAttribute("Precision", prec)
    data_item_attr.setAttribute("Format", "HDF")
   
    data_item_attr_text = doc.createTextNode("{}:{}".format(h5filename, dset.name))
    data_item_attr.appendChild(data_item_attr_text)
    
    attribute.appendChild(data_item_attr)
    
    poly.appendChild(attribute)


def create_interation_node(iteration):
    """
    Return XDMF XML iteration node 
    
    Parameters:
    ----------------
    iteration: int
               iteration step
    Returns:
    ----------------
    return: Element
            h5py XDMF iteration node
    """

    iteration_node = doc.createElement("Iteration")
    iteration_node.setAttribute("IterationType", "Single")
    iteration_node.setAttribute("Value", "{}".format(iteration))
    return iteration_node


# HDF5 functions

# get the number of dimensions of an hdf5 dataset
def get_ndims(dset):
    return len(dset.shape)


# get dimensions as string for an hdf5 dataset
def get_dims(dset):
    ndims = get_ndims(dset)
    dims = ""
    for i in range(ndims):
        dims += "{}".format(dset.shape[i])
        if i < ndims - 1:
            dims += " "

    return dims


def get_datatype_and_prec(dset):
    type = dset.dtype
    
    if type == np.int32:
        return ("Int", "4")
        
    if type == np.int64:
        return ("Int", "8")
    
    if type == np.uint32:
        return ("UInt", "4")
    
    if type == np.uint64:
        return ("UInt", "8")
    
    if type == np.float32:
        return ("Float", "4")
    
    if type == np.float64:
        return ("Float", "8")
    
    return (None, None)


def eval_class_type(classType, dset, level, h5filename):
    if classType == SPLASH_CLASS_TYPE_GRID:
        log("GRID", False)
        create_xdmf_grid_attribute(dset, h5filename)
    if classType == SPLASH_CLASS_TYPE_POLY:
        log("POLY", False)
        create_xdmf_poly_attribute(dset, h5filename)


def print_dataset(dset, level, h5filename):
    for attr in dset.attrs.keys():
        if attr == SPLASH_CLASS_NAME:
            eval_class_type(dset.attrs.get(attr), dset, level, h5filename)
    log("")


def parse_hdf5_recursive(h5Group, h5filename, level):
    """
    Recursively parse HDF5 file and call functions at appropriate
    groups and datasets
    
    Parameters:
    ----------------
    h5group:    h5py Group
                Current HDF5 group object
    h5filename: string
                HDF5 filename
    level:      int
                Current parse depth level
    """

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
    

def get_iterationstep_for_group(dataGroup):
    """
    Return the iteration step for a libSplash HDF5 group of type /data/<iterationstep>
    
    Parameters:
    ----------------
    dataGroup: h5py Group
               HDF5 /data group
    Returns:
    ----------------
    return: string
            <iteration step> part as string
    """

    groups = dataGroup.keys()
    if len(groups) != 1:
        print("Error: data group is expected to have a single iteration step child group")
        return -1
    
    objClass = dataGroup.get(groups[0], None, True)
    if objClass != h5py.Group:
        print("Error: data group child is expected to be of object class Group")
        return -1
        
    index = groups[0].rfind("/")
    return groups[0][(index+1):]
    

def create_xdmf_for_splash_file(base_node_grid, base_node_poly, splashFilename, args):
    """
    Insert XDMF XML structure for splashFilename under base_node
    
    Parameters:
    ----------------
    base_node:      Element
                    XML base node
    splashFilename: string
                    libSplash filename
    Returns:
    ----------------
    return: boolean
            True if successfully inserted, False otherwise
    """

    if not os.path.isfile(splashFilename):
        print "Error: '{}' does not exist.".format(splashFilename)
        sys.exit(1)
        
    # open libSplash file
    h5file = h5py.File(splashFilename, "r")
    log("Opened libSplash file '{}'".format(h5file.filename))
    
    if args.fullpath: 
    	splashFilenameXmf = splashFilename
    else:
        filename_list = splashFilename.split("/")
        splashFilenameXmf = filename_list[len(filename_list) - 1]
   
    # find /data group
    dataGroup = h5file.get("data")
    if dataGroup == None:
        print "Error: Could not find root data group in '{}'.".format(splashFilename)
        h5file.close()
        return False
    
    iteration_step = get_iterationstep_for_group(dataGroup)
    
    parse_hdf5_recursive(dataGroup, splashFilenameXmf, 1)
  
    # append all collected grids to current xml base node
    index = 0
    for grid in grids.values():
        grid.setAttribute("Name", "Grid_{}_{}".format(iteration_step, index))
        grid.appendChild(create_iteration_node(iteration_step))
        base_node_grid.appendChild(grid)
        index += 1
    grids.clear()
    
    # append all collected polys to current xml base node
    index = 0
    for poly in polys.values():
        poly.setAttribute("Name", "Poly_{}_{}".format(iteration_step, index))
        poly.appendChild(create_iteration_node(iteration_step))
        base_node_poly.appendChild(poly)
        index += 1
    polys.clear()
    
    # close libSplash file
    h5file.close()
    return True


def grid_attribute_setter (main_grid, main_poly):
    """
    Subfunction for create_xdmf_xml, which appends attributes to xml nodes

    Parameters:
    ----------------
    main_grid, main_poly: Element 
                          XML node 
    Returns:
    ----------------
    return: Element
            XML base node
    """
    main_grid.setAttribute("Name","Grids")
    main_poly.setAttribute("Name","Polys")
    
    for i in [main_grid, main_poly]:
        i.setAttribute("GridType", "Collection")
        i.setAttribute("CollectionType", "Temporal")

    return main_grid, main_poly

# program functions

def create_xdmf_xml(splash_files_list, args):
    """
    Return the single XDMF XML structure in the current document for all libSplash
    files in splash_files_list
    
    Parameters:
    ----------------
    splash_files_list: list of strings
                       list of libSplash filenames to create XDMF for
    args:              object
                       parsed command line user input 
    Returns:
    ----------------
    return: Element
            XML XDMF root node
    """

    iteration_series = (len(splash_files_list) > 1)
    
    # setup xml structure
    xdmf_root = doc.createElement("Xdmf")
    grid_xdmf_root = doc.createElement("Xdmf")
    poly_xdmf_root = doc.createElement("Xdmf")
    domain = doc.createElement("Domain")
    grid_domain = grid_doc.createElement("Domain")
    poly_domain = poly_doc.createElement("Domain")
    if args.no_splitgrid:
        base_node_grid = domain
        base_node_poly = domain
    else:
        base_node_grid = grid_domain
        base_node_poly = poly_domain
    
    if iteration_series:
        if args.no_splitgrid:		
            main_grid = doc.createElement("Grid")
            main_poly = doc.createElement("Grid")
        else:
            main_grid = grid_doc.createElement("Grid")
            main_poly = poly_doc.createElement("Grid")

        grid_attribute_setter(main_grid, main_poly)
        base_node_grid = main_grid
        base_node_poly = main_poly

    for current_file in splash_files_list:
        # parse this splash file and append to current xml base node
        create_xdmf_for_splash_file(base_node_grid, base_node_poly, current_file, args)

    # finalize xml structure
    if iteration_series:
	if args.no_splitgrid:
            domain.appendChild(main_grid)
            domain.appendChild(main_poly)
	else:
            grid_domain.appendChild(main_grid) 
            poly_domain.appendChild(main_poly)

    
    if args.no_splitgrid:
        xdmf_root.appendChild(domain)
        doc.appendChild(xdmf_root)
        return xdmf_root       
    else:
        grid_xdmf_root.appendChild(grid_domain)
        grid_doc.appendChild(grid_xdmf_root)
        poly_xdmf_root.appendChild(poly_domain)
        poly_doc.appendChild(poly_xdmf_root)
        return (grid_xdmf_root, poly_xdmf_root)


def write_xml_to_file(filename, document):
    """
    Write XDMF XML document to file filename
    
    Parameters:
    ----------------
    filename: string
              output filename
    document: Document
              XML Document object to store in file
    """

    xdmf_file = open(filename, "w")
    xdmf_file.write(document.toprettyxml())
    xdmf_file.close()
    log("Created XDMF file '{}'".format(filename), True, 0)
    

def handle_user_filename(filename):
    """
    Create output filename
    Parameters:
    ----------------
    filename: string
              output filename
    Returns:
    ----------------
    return: list
            output filename list 
    """
    output_filename_list = list()
    tmp = filename.rfind(".")
    for i in ["_grid","_poly"]:
        tmp_filename = filename[:tmp] + i + "." + filename[tmp+1:]	
	output_filename_list.append(tmp_filename)
    return output_filename_list

    
def get_args_parser():
    parser = argparse.ArgumentParser(description="Create a XDMF meta description file from a libSplash HDF5 file.")

    parser.add_argument("splashfile", metavar="<filename>",
        help="libSplash HDF5 file with domain information")
        
    parser.add_argument("-o", metavar="<filename>", help="Name of output XDMF file (default: append '.xmf')")
        
    parser.add_argument("-v", "--verbose", help="Produce verbose output", action="store_true")
    
    parser.add_argument("-i", "--iteration", help="Aggregate information over a iteration-series of libSplash data", action="store_true")
  
    parser.add_argument("--fullpath", help="Use absolute paths for HDF5 files", action="store_true")
    
    parser.add_argument("--no_splitgrid", help="Avoid the XML-tree to be split in grid and poly grids for separate output files", action="store_true")
    
    return parser


# main

def main():
    global verbosity_level

    # get arguments from command line
    args_parser = get_args_parser()
    args = args_parser.parse_args()

    # apply arguments
    splashFilename = args.splashfile
    iteration_series = args.iteration
    if args.verbose:
        verbosity_level = 100

    # create the list of requested splash files
    splash_files = list()
    if iteration_series:
        splashFilename = get_common_filename(splashFilename)
        for s_filename in glob.glob("{}_*.h5".format(splashFilename)):
            splash_files.append(s_filename)
    else:
        splash_files.append(splashFilename)
        tmp = splashFilename.rfind(".h5")
        splashFilename = splashFilename[:tmp]   

    create_xdmf_xml(splash_files, args)    
 
    output_filename = "{}.xmf".format(splashFilename)
    if args.o:
        if args.o.endswith(".xmf"):
            output_filename = args.o
        else:
            print "The script was stopped, because your output filename doesn't have\nan ending paraview can work with. Please use the ending '.xmf'!"
            sys.exit()

    if args.no_splitgrid:
        write_xml_to_file(output_filename, doc)
    else:
        output_filename_list = handle_user_filename(output_filename)
        for output_file in output_filename_list:
            if output_file.endswith("_grid.xmf"):
                write_xml_to_file(output_file, grid_doc)
            if output_file.endswith("_poly.xmf"):
                write_xml_to_file(output_file, poly_doc)

if __name__ == "__main__":
    main()
