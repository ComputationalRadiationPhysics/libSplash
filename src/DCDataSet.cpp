/**
 * Copyright 2013-2014 Felix Schmitt
 *
 * This file is part of libSplash. 
 * 
 * libSplash is free software: you can redistribute it and/or modify 
 * it under the terms of of either the GNU General Public License or 
 * the GNU Lesser General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version. 
 * libSplash is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License and the GNU Lesser General Public License 
 * for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * and the GNU Lesser General Public License along with libSplash. 
 * If not, see <http://www.gnu.org/licenses/>. 
 */



#include <string>
#include <sstream>
#include <cassert>

#include "splash/sdc_defines.hpp"

#include "splash/core/DCDataSet.hpp"
#include "splash/core/DCAttribute.hpp"
#include "splash/core/DCHelper.hpp"
#include "splash/core/logging.hpp"
#include "splash/DCException.hpp"
#include "splash/basetypes/ColTypeDim.hpp"

namespace splash
{

    std::string DCDataSet::getExceptionString(std::string msg)
    {
        return (std::string("Exception for DCDataSet [") + name + std::string("] ") +
                msg);
    }

    void DCDataSet::splitPath(const std::string fullName, std::string &path, std::string &name)
    {
        std::string::size_type pos = fullName.find_last_of('/');

        if (pos == std::string::npos || (pos == fullName.size() - 1))
        {
            path = "";
            name = fullName;
        } else
        {
            path.assign(fullName.c_str(), fullName.c_str() + pos);
            name.assign(fullName.c_str() + pos + 1);
        }
    }

    void DCDataSet::getFullDataPath(const std::string fullUserName,
            const std::string pathBase, uint32_t id, std::string &path, std::string &name)
    {
        splitPath(fullUserName, path, name);

        std::stringstream group_id_name;
        group_id_name << pathBase << "/" << id;
        if (path.size() > 0)
            group_id_name << "/" << path;

        path.assign(group_id_name.str());
    }

    DCDataSet::DCDataSet(const std::string name) :
    dataset(-1),
    datatype(-1),
    logicalSize(),
    ndims(0),
    name(name),
    opened(false),
    isReference(false),
    checkExistence(true),
    compression(false),
    dimType()
    {
        dsetProperties = H5Pcreate(H5P_DATASET_CREATE);
        dsetWriteProperties = H5P_DEFAULT;
        dsetReadProperties = H5P_DEFAULT;
    }

    DCDataSet::~DCDataSet()
    {
        H5Pclose(dsetProperties);
    }
    
    Dimensions DCDataSet::getSize() const
    {
        return logicalSize;
    }

    Dimensions& DCDataSet::getLogicalSize()
    {
        return logicalSize;
    }

    Dimensions DCDataSet::getPhysicalSize()
    {
        Dimensions result(logicalSize);
        result.swapDims(ndims);
        return result;
    }

    bool DCDataSet::open(hid_t group)
    throw (DCException)
    {
        if (checkExistence && !H5Lexists(group, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
            return false;

        dataset = H5Dopen(group, name.c_str(), H5P_DATASET_ACCESS_DEFAULT);

        if (dataset < 0)
            throw DCException(getExceptionString("open: Failed to open dataset"));

        datatype = H5Dget_type(dataset);
        if (datatype < 0)
        {
            H5Dclose(dataset);
            throw DCException(getExceptionString("open: Failed to get type of dataset"));
        }

        dataspace = H5Dget_space(dataset);
        if (dataspace < 0)
        {
            H5Dclose(dataset);
            throw DCException(getExceptionString("open: Failed to open dataspace"));
        }

        int dims_result = H5Sget_simple_extent_ndims(dataspace);
        if (dims_result < 0)
        {
            close();
            throw DCException(getExceptionString("open: Failed to get dimensions"));
        }

        ndims = dims_result;

        getLogicalSize().set(1, 1, 1);
        if (H5Sget_simple_extent_dims(dataspace, getLogicalSize().getPointer(), NULL) < 0)
        {
            close();
            throw DCException(getExceptionString("open: Failed to get sizes"));
        }

        getLogicalSize().swapDims(ndims);

        opened = true;

        return true;
    }

    void DCDataSet::setChunking(size_t typeSize)
    throw (DCException)
    {
        if (getPhysicalSize().getScalarSize() != 0)
        {
            // get chunking dimensions
            hsize_t chunk_dims[ndims];
            DCHelper::getOptimalChunkDims(getPhysicalSize().getPointer(), ndims,
                    typeSize, chunk_dims);

            if (H5Pset_chunk(this->dsetProperties, ndims, chunk_dims) < 0)
            {
                for (size_t i = 0; i < ndims; ++i)
                {
                    log_msg(1, "chunk_dims[%llu] = %llu",
                            (long long unsigned) i, (long long unsigned) (chunk_dims[i]));
                }
                throw DCException(getExceptionString("setChunking: Failed to set chunking"));
            }
        }
    }

    void DCDataSet::setCompression()
    throw (DCException)
    {
        if (this->compression && getPhysicalSize().getScalarSize() != 0)
        {
            // shuffling reorders bytes for better compression
            // set gzip compression level (1=lowest - 9=highest)
            if (H5Pset_shuffle(this->dsetProperties) < 0 ||
                    H5Pset_deflate(this->dsetProperties, 1) < 0)
                throw DCException(getExceptionString("setCompression: Failed to set compression"));
        }
    }

    void DCDataSet::create(const CollectionType& colType,
            hid_t group, const Dimensions size, uint32_t ndims, bool compression)
    throw (DCException)
    {
        log_msg(2, "DCDataSet::create (%s, size %s)", name.c_str(), size.toString().c_str());

        if (opened)
            throw DCException(getExceptionString("create: dataset is already open"));

        // if the dataset already exists, remove/unlink it
        // note that this won't free the memory occupied by this
        // dataset, however, there currently is no function to delete
        // a dataset
        if (!checkExistence || (checkExistence && H5Lexists(group, name.c_str(), H5P_LINK_ACCESS_DEFAULT)))
            H5Ldelete(group, name.c_str(), H5P_LINK_ACCESS_DEFAULT);

        this->ndims = ndims;
        this->compression = compression;
        this->datatype = colType.getDataType();

        getLogicalSize().set(size);

        setChunking(colType.getSize());
        setCompression();

        if (getPhysicalSize().getScalarSize() != 0)
        {
            hsize_t *max_dims = new hsize_t[ndims];
            for (size_t i = 0; i < ndims; ++i)
                max_dims[i] = H5F_UNLIMITED;

            dataspace = H5Screate_simple(ndims, getPhysicalSize().getPointer(), max_dims);

            delete[] max_dims;
            max_dims = NULL;
        } else
            dataspace = H5Screate(H5S_NULL);



        if (dataspace < 0)
            throw DCException(getExceptionString("create: Failed to create dataspace"));

        // create the new dataset
        dataset = H5Dcreate(group, this->name.c_str(), this->datatype, dataspace,
                H5P_DEFAULT, dsetProperties, H5P_DEFAULT);

        if (dataset < 0)
            throw DCException(getExceptionString("create: Failed to create dataset"));

        isReference = false;
        opened = true;
    }

    void DCDataSet::createReference(hid_t refGroup,
            hid_t srcGroup,
            DCDataSet &srcDataSet)
    throw (DCException)
    {
        if (opened)
            throw DCException(getExceptionString("createReference: dataset is already open"));

        if (checkExistence && H5Lexists(refGroup, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
            throw DCException(getExceptionString("createReference: this reference already exists"));

        getLogicalSize().set(srcDataSet.getLogicalSize());
        this->ndims = srcDataSet.getNDims();

        if (H5Rcreate(&regionRef, srcGroup, srcDataSet.getName().c_str(), H5R_OBJECT, -1) < 0)
            throw DCException(getExceptionString("createReference: failed to create region reference"));

        hsize_t ndims = 1;
        dataspace = H5Screate_simple(1, &ndims, NULL);
        if (dataspace < 0)
            throw DCException(getExceptionString("createReference: failed to create dataspace for reference"));

        dataset = H5Dcreate(refGroup, name.c_str(), H5T_STD_REF_OBJ,
                dataspace, H5P_DEFAULT, dsetProperties, H5P_DEFAULT);

        if (dataset < 0)
            throw DCException(getExceptionString("createReference: failed to create dataset for reference"));

        if (H5Dwrite(dataset, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL,
                dsetWriteProperties, &regionRef) < 0)
            throw DCException(getExceptionString("createReference: failed to write reference"));

        isReference = true;
        opened = true;
    }

    void DCDataSet::createReference(hid_t refGroup,
            hid_t srcGroup,
            DCDataSet &srcDataSet,
            Dimensions count,
            Dimensions offset,
            Dimensions stride)
    throw (DCException)
    {
        if (opened)
            throw DCException(getExceptionString("createReference: dataset is already open"));

        if (checkExistence && H5Lexists(refGroup, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
            throw DCException(getExceptionString("createReference: this reference already exists"));

        getLogicalSize().set(count);
        this->ndims = srcDataSet.getNDims();

        count.swapDims(this->ndims);
        offset.swapDims(this->ndims);
        stride.swapDims(this->ndims);

        // select region hyperslab in source dataset
        if (H5Sselect_hyperslab(srcDataSet.getDataSpace(), H5S_SELECT_SET,
                offset.getPointer(), stride.getPointer(),
                count.getPointer(), NULL) < 0 ||
                H5Sselect_valid(srcDataSet.getDataSpace()) <= 0)
            throw DCException(getExceptionString("createReference: failed to select hyperslap for reference"));

        if (H5Rcreate(&regionRef, srcGroup, srcDataSet.getName().c_str(), H5R_DATASET_REGION,
                srcDataSet.getDataSpace()) < 0)
            throw DCException(getExceptionString("createReference: failed to create region reference"));

        hsize_t ndims = 1;
        dataspace = H5Screate_simple(1, &ndims, NULL);
        if (dataspace < 0)
            throw DCException(getExceptionString("createReference: failed to create dataspace for reference"));

        dataset = H5Dcreate(refGroup, name.c_str(), H5T_STD_REF_DSETREG,
                dataspace, H5P_DEFAULT, dsetProperties, H5P_DEFAULT);

        if (dataset < 0)
            throw DCException(getExceptionString("createReference: failed to create dataset for reference"));

        if (H5Dwrite(dataset, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL,
                dsetWriteProperties, &regionRef) < 0)
            throw DCException(getExceptionString("createReference: failed to write reference"));

        isReference = true;
        opened = true;
    }

    void DCDataSet::close()
    throw (DCException)
    {
        opened = false;
        isReference = false;

        if (H5Dclose(dataset) < 0 || H5Sclose(dataspace) < 0)
            throw DCException(getExceptionString("close: Failed to close dataset"));
    }

    size_t DCDataSet::getNDims()
    {
        return ndims;
    }

    hid_t DCDataSet::getDataSpace()
    throw (DCException)
    {
        if (!opened)
            throw DCException(getExceptionString("getDataSpace: dataset is not opened"));

        return dataspace;
    }

    DCDataType DCDataSet::getDCDataType() throw (DCException)
    {
        if (!opened)
            throw DCException(getExceptionString("getDCDataType: dataset is not opened"));

        DCDataType result = DCDT_UNKNOWN;

        H5T_class_t type_class = H5Tget_class(datatype);
        size_t type_size = H5Tget_size(datatype);
        H5T_sign_t type_signed = H5Tget_sign(datatype);

        if (type_class == H5T_INTEGER)
        {
            if (type_signed == H5T_SGN_NONE)
            {
                if (type_size == sizeof (uint64_t))
                    result = DCDT_UINT64;
                else
                    result = DCDT_UINT32;
            } else
            {
                if (type_size == sizeof (int64_t))
                    result = DCDT_INT64;
                else
                    result = DCDT_INT32;
            }
        } else
            if (type_class == H5T_FLOAT)
        {
            // float or double
            if (type_size == sizeof (float))
                result = DCDT_FLOAT32;
            else
                if (type_size == sizeof (double))
                result = DCDT_FLOAT64;
        }

        return result;
    }

    size_t DCDataSet::getDataTypeSize()
    throw (DCException)
    {
        if (!opened)
            throw DCException(getExceptionString("getDataTypeSize: dataset is not opened"));

        size_t size = H5Tget_size(this->datatype);
        if (size == 0)
            throw DCException(getExceptionString("getDataTypeSize: could not get size of datatype"));

        return size;
    }

    std::string DCDataSet::getName()
    {
        return name;
    }

    void DCDataSet::read(Dimensions dstBuffer,
            Dimensions dstOffset,
            Dimensions &sizeRead,
            uint32_t& srcNDims,
            void* dst)
    throw (DCException)
    {
        read(dstBuffer, dstOffset, getLogicalSize(), Dimensions(0, 0, 0),
                sizeRead, srcNDims, dst);
    }

    void DCDataSet::read(Dimensions dstBuffer,
            Dimensions dstOffset,
            Dimensions srcSize,
            Dimensions srcOffset,
            Dimensions& sizeRead,
            uint32_t& srcNDims,
            void* dst)
    throw (DCException)
    {
        log_msg(2, "DCDataSet::read (%s)", name.c_str());

        if (!opened)
            throw DCException(getExceptionString("read: Dataset has not been opened/created"));

        if (dstBuffer.getScalarSize() == 0)
            dstBuffer.set(srcSize);

        // dst buffer is allowed to be NULL
        // in this case, only the size of the dataset is returned
        // if the dataset is empty, return just its size as there is nothing to read
        if ((dst != NULL) && (getNDims() > 0))
        {
            log_msg(3,
                    "\n ndims         = %llu\n"
                    " logical_size  = %s\n"
                    " physical_size = %s\n"
                    " dstBuffer     = %s\n"
                    " dstOffset     = %s\n"
                    " srcSize       = %s\n"
                    " srcOffset     = %s\n",
                    (long long unsigned) ndims,
                    getLogicalSize().toString().c_str(),
                    getPhysicalSize().toString().c_str(),
                    dstBuffer.toString().c_str(),
                    dstOffset.toString().c_str(),
                    srcSize.toString().c_str(),
                    srcOffset.toString().c_str());

            dstBuffer.swapDims(ndims);
            dstOffset.swapDims(ndims);
            srcSize.swapDims(ndims);
            srcOffset.swapDims(ndims);

            hid_t dst_dataspace = H5Screate_simple(ndims, dstBuffer.getPointer(), NULL);
            if (dst_dataspace < 0)
                throw DCException(getExceptionString("read: Failed to create target dataspace"));

            if (H5Sselect_hyperslab(dst_dataspace, H5S_SELECT_SET, dstOffset.getPointer(), NULL,
                    srcSize.getPointer(), NULL) < 0 ||
                    H5Sselect_valid(dst_dataspace) <= 0)
                throw DCException(getExceptionString("read: Target dataspace hyperslab selection is not valid!"));

            if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, srcOffset.getPointer(), NULL,
                    srcSize.getPointer(), NULL) < 0 ||
                    H5Sselect_valid(dataspace) <= 0)
                throw DCException(getExceptionString("read: Source dataspace hyperslab selection is not valid!"));

            if (srcSize.getScalarSize() == 0)
                H5Sselect_none(dataspace);
            
            if (H5Dread(dataset, this->datatype, dst_dataspace, dataspace, dsetReadProperties, dst) < 0)
                throw DCException(getExceptionString("read: Failed to read dataset"));

            H5Sclose(dst_dataspace);

            srcSize.swapDims(ndims);
        }

        // swap dimensions if necessary
        sizeRead.set(srcSize);
        srcNDims = this->ndims;

        log_msg(3, " returns ndims = %llu", (long long unsigned) ndims);
        log_msg(3, " returns sizeRead = %s", sizeRead.toString().c_str());
    }

    void DCDataSet::write(
            Selection srcSelect,
            Dimensions dstOffset,
            const void* data)
    throw (DCException)
    {
        log_msg(2, "DCDataSet::write (%s)", name.c_str());

        if (!opened)
            throw DCException(getExceptionString("write: Dataset has not been opened/created"));

        log_msg(3,
                "\n ndims         = %llu\n"
                " logical_size  = %s\n"
                " physical_size = %s\n"
                " src_select    = %s\n"
                " dst_offset    = %s\n",
                (long long unsigned) ndims,
                getLogicalSize().toString().c_str(),
                getPhysicalSize().toString().c_str(),
                srcSelect.toString().c_str(),
                dstOffset.toString().c_str());

        // swap dimensions if necessary
        srcSelect.swapDims(ndims);
        dstOffset.swapDims(ndims);

        // dataspace to read from
        hid_t dsp_src;

        if (getLogicalSize().getScalarSize() != 0)
        {
            dsp_src = H5Screate_simple(ndims, srcSelect.size.getPointer(), NULL);
            if (dsp_src < 0)
                throw DCException(getExceptionString("write: Failed to create source dataspace"));

            // select hyperslap only if necessary
            if ((srcSelect.offset.getScalarSize() != 0) || (srcSelect.count != srcSelect.size) || 
                    (srcSelect.stride.getScalarSize() != 1))
            {
                if (H5Sselect_hyperslab(dsp_src, H5S_SELECT_SET, srcSelect.offset.getPointer(),
                        srcSelect.stride.getPointer(), srcSelect.count.getPointer(), NULL) < 0 ||
                        H5Sselect_valid(dsp_src) <= 0)
                    throw DCException(getExceptionString("write: Invalid source hyperslap selection"));
            }

            if (srcSelect.count.getScalarSize() == 0)
                H5Sselect_none(dsp_src);

            // dataspace to write to
            // select hyperslap only if necessary
            if ((dstOffset.getScalarSize() != 0) || (srcSelect.count != getPhysicalSize()))
            {
                if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, dstOffset.getPointer(),
                        NULL, srcSelect.count.getPointer(), NULL) < 0 ||
                        H5Sselect_valid(dataspace) <= 0)
                    throw DCException(getExceptionString("write: Invalid target hyperslap selection"));
            }

            if (!data || (srcSelect.count.getScalarSize() == 0))
            {
                H5Sselect_none(dataspace);
                data = NULL;
            }

            // write data to the dataset

            if (H5Dwrite(dataset, this->datatype, dsp_src, dataspace, dsetWriteProperties, data) < 0)
                throw DCException(getExceptionString("write: Failed to write dataset"));

            H5Sclose(dsp_src);
        }
    }

    void DCDataSet::append(size_t count, size_t offset, size_t stride, const void* data)
    throw (DCException)
    {
        log_msg(2, "DCDataSet::append");

        if (!opened)
            throw DCException(getExceptionString("append: Dataset has not been opened/created."));

        log_msg(3, "logical_size = %s", getLogicalSize().toString().c_str());

        Dimensions target_offset(getLogicalSize());
        // extend size (dataspace) of existing dataset with count elements
        getLogicalSize()[0] += count;

        hsize_t * max_dims = new hsize_t[ndims];
        for (size_t i = 0; i < ndims; ++i)
            max_dims[i] = H5F_UNLIMITED;

        if (H5Sset_extent_simple(dataspace, 1, getLogicalSize().getPointer(), max_dims) < 0)
            throw DCException(getExceptionString("append: Failed to set new extent"));

        delete[] max_dims;
        max_dims = NULL;

        log_msg(3, "logical_size = %s", getLogicalSize().toString().c_str());

        if (H5Dset_extent(dataset, getLogicalSize().getPointer()) < 0)
            throw DCException(getExceptionString("append: Failed to extend dataset"));

        // select the region in the target DataSpace to write to
        Dimensions dim_data(count, 1, 1);
        if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, target_offset.getPointer(),
                NULL, dim_data.getPointer(), NULL) < 0 ||
                H5Sselect_valid(dataspace) < 0)
            throw DCException(getExceptionString("append: Invalid target hyperslap selection"));

        // append data to the dataset.
        // select the region in the source DataSpace to read from
        Dimensions dim_src(offset + count * stride, 1, 1);
        hid_t dsp_src = H5Screate_simple(1, dim_src.getPointer(), NULL);
        if (dsp_src < 0)
            throw DCException(getExceptionString("append: Failed to create src dataspace while appending"));

        if (H5Sselect_hyperslab(dsp_src, H5S_SELECT_SET, Dimensions(offset, 0, 0).getPointer(),
                Dimensions(stride, 1, 1).getPointer(), dim_data.getPointer(), NULL) < 0 ||
                H5Sselect_valid(dsp_src) < 0)
            throw DCException(getExceptionString("append: Invalid source hyperslap selection"));

        if (!data || (count == 0))
        {
            H5Sselect_none(dataspace);
            data = NULL;
        }

        if (H5Dwrite(dataset, this->datatype, dsp_src, dataspace, dsetWriteProperties, data) < 0)
            throw DCException(getExceptionString("append: Failed to append dataset"));

        H5Sclose(dsp_src);
    }

}
