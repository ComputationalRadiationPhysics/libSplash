/**
 * Copyright 2013 Felix Schmitt
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

#include "sdc_defines.hpp"

#include "DCDataSet.hpp"
#include "DCAttribute.hpp"
#include "DCHelper.hpp"
#include "DCException.hpp"
#include "basetypes/ColTypeDim.hpp"

namespace DCollector
{

    std::string DCDataSet::getExceptionString(std::string msg)
    {
        return (std::string("Exception for DCDataSet [") + name + std::string("] ") +
                msg);
    }

    DCDataSet::DCDataSet(const std::string name) :
    dataset(-1),
    datatype(-1),
    logicalSize(),
    rank(0),
    name(name),
    opened(false),
    isReference(false),
    compression(false),
    dimType()
    {
        dsetProperties = H5Pcreate(H5P_DATASET_CREATE);
        dsetWriteProperties = H5P_DEFAULT;
    }

    DCDataSet::~DCDataSet()
    {
        H5Pclose(dsetProperties);
    }

    Dimensions& DCDataSet::getLogicalSize()
    {
        return logicalSize;
    }

    Dimensions DCDataSet::getPhysicalSize()
    {
        Dimensions result(logicalSize);
        result.swapDims(rank);
        return result;
    }

    bool DCDataSet::open(hid_t &group)
    throw (DCException)
    {
        if (!H5Lexists(group, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
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

        rank = H5Sget_simple_extent_ndims(dataspace);
        if (rank < 0)
        {
            close();
            throw DCException(getExceptionString("open: Failed to get dimensions"));
        }

        getLogicalSize().set(1, 1, 1);
        if (H5Sget_simple_extent_dims(dataspace, getLogicalSize().getPointer(), NULL) < 0)
        {
            close();
            throw DCException(getExceptionString("open: Failed to get sizes"));
        }

        getLogicalSize().swapDims(rank);

        opened = true;

        return true;
    }

    void DCDataSet::setChunking(size_t typeSize)
    throw (DCException)
    {
        if (getPhysicalSize().getDimSize() != 0)
        {
            // get chunking dimensions
            hsize_t chunk_dims[rank];
            DCHelper::getOptimalChunkDims(getPhysicalSize().getPointer(), rank,
                    typeSize, chunk_dims);

            if (H5Pset_chunk(this->dsetProperties, rank, chunk_dims) < 0)
                throw DCException(getExceptionString("setChunking: Failed to set chunking"));

#if defined SDC_DEBUG_OUTPUT
            DCHelper::printhsizet("chunk_dims", chunk_dims, rank);
#endif
        }
    }

    void DCDataSet::setCompression()
    throw (DCException)
    {
        if (this->compression && getPhysicalSize().getDimSize() != 0)
        {
            // shuffling reorders bytes for better compression
            // set gzip compression level (1=lowest - 9=highest)
            if (H5Pset_shuffle(this->dsetProperties) < 0 ||
                    H5Pset_deflate(this->dsetProperties, 1) < 0)
                throw DCException(getExceptionString("setCompression: Failed to set compression"));
        }
    }

    void DCDataSet::create(const CollectionType& colType,
            hid_t &group, const Dimensions size, uint32_t rank, bool compression)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# DCDataSet::create #" << std::endl;
        std::cerr << std::endl << "creating: " << name << " with size " << size.toString() << std::endl;
#endif
        if (opened)
            throw DCException(getExceptionString("create: dataset is already open"));

        // if the dataset already exists, remove/unlink it
        // note that this won't free the memory occupied by this
        // dataset, however, there currently is no function to delete
        // a dataset
        if (H5Lexists(group, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
            H5Gunlink(group, name.c_str());

        this->rank = rank;
        this->compression = compression;
        this->datatype = colType.getDataType();

        getLogicalSize().set(size);

        setChunking(colType.getSize());
        setCompression();

        if (getPhysicalSize().getDimSize() != 0)
        {
            hsize_t *max_dims = new hsize_t[rank];
            for (size_t i = 0; i < rank; ++i)
                max_dims[i] = H5F_UNLIMITED;

            dataspace = H5Screate_simple(rank, getPhysicalSize().getPointer(), max_dims);

            delete[] max_dims;
            max_dims = NULL;
        }
        else
            dataspace = H5Screate(H5S_NULL);



        if (dataspace < 0)
            throw DCException(getExceptionString("create: Failed to create dataspace"));

        // create the new dataset
        dataset = H5Dcreate(group, this->name.c_str(), this->datatype, dataspace, H5P_LINK_CREATE_DEFAULT,
                this->dsetProperties, H5P_DATASET_ACCESS_DEFAULT);

        if (dataset < 0)
            throw DCException(getExceptionString("create: Failed to create dataset"));

        isReference = false;
        opened = true;
    }

    void DCDataSet::createReference(hid_t& refGroup,
            hid_t& srcGroup,
            DCDataSet &srcDataSet,
            Dimensions count,
            Dimensions offset,
            Dimensions stride)
    throw (DCException)
    {
        if (opened)
            throw DCException(getExceptionString("createReference: dataset is already open"));

        if (H5Lexists(refGroup, name.c_str(), H5P_LINK_ACCESS_DEFAULT))
            throw DCException(getExceptionString("createReference: this reference already exists"));

        getLogicalSize().set(count);
        this->rank = srcDataSet.getRank();

        count.swapDims(this->rank);
        offset.swapDims(this->rank);
        stride.swapDims(this->rank);

        // select region hyperslab in source dataset
        H5Sselect_hyperslab(srcDataSet.getDataSpace(), H5S_SELECT_SET,
                offset.getPointer(), stride.getPointer(),
                count.getPointer(), NULL);

        H5Rcreate(&regionRef, srcGroup, srcDataSet.getName().c_str(), H5R_DATASET_REGION,
                srcDataSet.getDataSpace());

        hsize_t dims = 1;
        hsize_t max_dim = H5S_UNLIMITED;
        dataspace = H5Screate_simple(1, &dims, &max_dim);

        dataset = H5Dcreate(refGroup, name.c_str(), H5T_STD_REF_DSETREG,
                dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

        H5Dwrite(dataset, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL,
                dsetWriteProperties, &regionRef);

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

    size_t DCDataSet::getRank()
    {
        return rank;
    }

    hid_t DCDataSet::getDataSpace()
    throw (DCException)
    {
        if (!opened)
            throw DCException(getExceptionString("getDataSpace: dataset is not opened"));

        return dataspace;
    }

    DCDataSet::DCDataType DCDataSet::getDCDataType() throw (DCException)
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
            }
            else
            {
                if (type_size == sizeof (int64_t))
                    result = DCDT_INT64;
                else
                    result = DCDT_INT32;
            }
        }
        else
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
            uint32_t& srcRank,
            void* dst)
    throw (DCException)
    {
        read(dstBuffer, dstOffset, Dimensions(0, 0, 0), Dimensions(0, 0, 0),
                sizeRead, srcRank, dst);
    }

    void DCDataSet::read(Dimensions dstBuffer,
            Dimensions dstOffset,
            Dimensions srcSize,
            Dimensions srcOffset,
            Dimensions& sizeRead,
            uint32_t& srcRank,
            void* dst)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# DCDataSet::read #" << std::endl;
        std::cerr << std::endl << "reading: " << name << std::endl;
#endif
        if (!opened)
            throw DCException(getExceptionString("read: Dataset has not been opened/created"));

        if (dstBuffer.getDimSize() == 0)
            dstBuffer.set(getLogicalSize());

        if (srcSize.getDimSize() == 0)
            srcSize.set(getLogicalSize() - srcOffset);

        // dst buffer is allowed to be NULL
        // in this case, only the size of the dataset is returned
        // if the dataset is empty, return just its size as there is nothing to read
        if (dst != NULL && getRank() > 0)
        {
#if defined SDC_DEBUG_OUTPUT
            std::cerr << "rank = " << rank << std::endl;
            std::cerr << "logical_size = " << getLogicalSize().toString() << std::endl;
            std::cerr << "physical_size = " << getPhysicalSize().toString() << std::endl;
            std::cerr << "[1] dstBuffer = " << dstBuffer.toString() << std::endl;
            std::cerr << "[1] dstOffset = " << dstOffset.toString() << std::endl;
            std::cerr << "[1] srcSize = " << srcSize.toString() << std::endl;
            std::cerr << "[1] srcOffset = " << srcOffset.toString() << std::endl;
#endif

            dstBuffer.swapDims(rank);
            dstOffset.swapDims(rank);
            srcSize.swapDims(rank);
            srcOffset.swapDims(rank);

#if defined SDC_DEBUG_OUTPUT
            std::cerr << "[2] dstBuffer = " << dstBuffer.toString() << std::endl;
            std::cerr << "[2] dstOffset = " << dstOffset.toString() << std::endl;
            std::cerr << "[2] srcSize = " << srcSize.toString() << std::endl;
            std::cerr << "[2] srcOffset = " << srcOffset.toString() << std::endl;
#endif

            hid_t dst_dataspace = H5Screate_simple(rank, dstBuffer.getPointer(), NULL);
            if (dst_dataspace < 0)
                throw DCException(getExceptionString("read: Failed to create target dataspace"));

            if (rank == 1)
            {
                // read data
                if (H5Sselect_hyperslab(dst_dataspace, H5S_SELECT_SET, dstOffset.getPointer(), NULL,
                        srcSize.getPointer(), NULL) < 0 ||
                        H5Sselect_valid(dst_dataspace) <= 0)
                    throw DCException(getExceptionString("read: Target dataspace hyperslab selection is not valid!"));

                if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, srcOffset.getPointer(), NULL,
                        srcSize.getPointer(), NULL) < 0 ||
                        H5Sselect_valid(dataspace) <= 0)
                    throw DCException(getExceptionString("read: Source dataspace hyperslab selection is not valid!"));

                if (H5Dread(dataset, this->datatype, dst_dataspace, dataspace, H5P_DEFAULT, dst) < 0)
                    throw DCException(getExceptionString("read: Failed to read dataset"));

            }
            else
            {
                Dimensions client_size(1, 1, 1);
                Dimensions local_mpi_size(1, 1, 1);

                DCAttribute::readAttribute(SDC_ATTR_SIZE, dataset, client_size.getPointer());
                DCAttribute::readAttribute(SDC_ATTR_MPI_SIZE, dataset, local_mpi_size.getPointer());

                client_size.swapDims(rank);
                local_mpi_size.swapDims(rank);

                if (local_mpi_size.getDimSize() != 1)
                {
                    if (srcOffset.getDimSize() != 0 || srcSize != getPhysicalSize())
                        throw DCException(
                            getExceptionString("read: reading composite datasets with source offset is not supported !"));

                    // read data in parts (each part is for one mpi position)
                    for (uint32_t x = 0; x < local_mpi_size[0]; x++)
                        for (uint32_t y = 0; y < local_mpi_size[1]; y++)
                            for (uint32_t z = 0; z < local_mpi_size[2]; z++)
                            {
                                Dimensions offset(x * client_size[0], y * client_size[1], z * client_size[2]);
                                Dimensions dst_offset(offset);

                                for (int i = 0; i < 3; i++)
                                    dst_offset[i] += dstOffset[i];

                                if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset.getPointer(), NULL,
                                        client_size.getPointer(), NULL) < 0 ||
                                        H5Sselect_valid(dataspace) <= 0)
                                    throw DCException(getExceptionString("read: Source hyperslab selection is not valid !"));

                                if (H5Sselect_hyperslab(dst_dataspace, H5S_SELECT_SET, dst_offset.getPointer(), NULL,
                                        client_size.getPointer(), NULL) < 0 ||
                                        H5Sselect_valid(dst_dataspace) <= 0)
                                    throw DCException(getExceptionString("read: Target dataspace hyperslab selection is not valid!"));


                                if (H5Dread(dataset, this->datatype, dst_dataspace, dataspace, H5P_DEFAULT, dst) < 0)
                                    throw DCException(getExceptionString("read: Failed to read dataset"));
                            }
                }
                else
                {
                    if (H5Sselect_hyperslab(dst_dataspace, H5S_SELECT_SET, dstOffset.getPointer(), NULL,
                            srcSize.getPointer(), NULL) < 0 ||
                            H5Sselect_valid(dst_dataspace) <= 0)
                        throw DCException(getExceptionString("read: Target dataspace hyperslab selection is not valid!"));

                    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, srcOffset.getPointer(), NULL,
                            srcSize.getPointer(), NULL) < 0 ||
                            H5Sselect_valid(dataspace) <= 0)
                        throw DCException(getExceptionString("read: Source dataspace hyperslab selection is not valid!"));

                    if (H5Dread(dataset, this->datatype, dst_dataspace, dataspace, H5P_DEFAULT, dst) < 0)
                        throw DCException(getExceptionString("read: Failed to read dataset"));
                }
            }

            H5Sclose(dst_dataspace);
            
            srcSize.swapDims(rank);
        }

        // swap dimensions if necessary
        sizeRead.set(srcSize);
        srcRank = this->rank;

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "returns rank = " << rank << std::endl;
        std::cerr << "returns sizeRead = " << sizeRead.toString() << std::endl;
#endif
    }

    void DCDataSet::write(Dimensions dim, const void* data)
    throw (DCException)
    {
        write(dim, Dimensions(1, 1, 1), Dimensions(0, 0, 0), dim, Dimensions(0, 0, 0), data);
    }

    void DCDataSet::write(Dimensions srcBuffer, Dimensions srcStride, Dimensions srcOffset, Dimensions srcData,
            const void* data)
    throw (DCException)
    {
        write(srcBuffer, srcStride, srcOffset, srcData, Dimensions(0, 0, 0), data);
    }

    void DCDataSet::write(Dimensions srcBuffer, Dimensions srcStride,
            Dimensions srcOffset, Dimensions srcData,
            Dimensions dstOffset, const void* data)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# DCDataSet::write #" << std::endl;
        std::cerr << std::endl << "writing: " << name << std::endl;
#endif

        if (!opened)
            throw DCException(getExceptionString("write: Dataset has not been opened/created"));

        if (data == NULL)
            throw DCException(getExceptionString("write: Cannot write NULL data"));

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "rank = " << rank << std::endl;
        std::cerr << "logical_size = " << getLogicalSize().toString() << std::endl;
        std::cerr << "physical_size = " << getPhysicalSize().toString() << std::endl;

        std::cerr << "[1] src_buffer = " << srcBuffer.toString() << std::endl;
        std::cerr << "[1] src_stride = " << srcStride.toString() << std::endl;
        std::cerr << "[1] src_data = " << srcData.toString() << std::endl;
        std::cerr << "[1] src_offset = " << srcOffset.toString() << std::endl;
        std::cerr << "[1] dst_offset = " << dstOffset.toString() << std::endl;
#endif
        // swap dimensions if necessary
        srcBuffer.swapDims(rank);
        srcStride.swapDims(rank);
        srcData.swapDims(rank);
        srcOffset.swapDims(rank);
        dstOffset.swapDims(rank);

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "[2] src_buffer = " << srcBuffer.toString() << std::endl;
        std::cerr << "[2] src_stride = " << srcStride.toString() << std::endl;
        std::cerr << "[2] src_data = " << srcData.toString() << std::endl;
        std::cerr << "[2] src_offset = " << srcOffset.toString() << std::endl;
        std::cerr << "[2] dst_offset = " << dstOffset.toString() << std::endl;
#endif

        // dataspace to read from
        hid_t dsp_src;

        if (getLogicalSize().getDimSize() != 0)
        {
            dsp_src = H5Screate_simple(rank, srcBuffer.getPointer(), NULL);
            if (dsp_src < 0)
                throw DCException(getExceptionString("write: Failed to create source dataspace"));

            if (H5Sselect_hyperslab(dsp_src, H5S_SELECT_SET, srcOffset.getPointer(),
                    srcStride.getPointer(), srcData.getPointer(), NULL) < 0 ||
                    H5Sselect_valid(dsp_src) <= 0)
                throw DCException(getExceptionString("write: Invalid source hyperslap selection"));

            // dataspace to write to
            if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, dstOffset.getPointer(),
                    NULL, srcData.getPointer(), NULL) < 0 ||
                    H5Sselect_valid(dataspace) <= 0)
                throw DCException(getExceptionString("write: Invalid target hyperslap selection"));

            // write data to the dataset
            if (H5Dwrite(dataset, this->datatype, dsp_src, dataspace, dsetWriteProperties, data) < 0)
                throw DCException(getExceptionString("write: Failed to write dataset"));

            if (rank > 1)
            {
                // write (client) size of this dataset
                DCAttribute::writeAttribute(SDC_ATTR_SIZE, dimType.getDataType(),
                        dataset, getLogicalSize().getPointer());

                // mark this dataset as not combined
                DCAttribute::writeAttribute(SDC_ATTR_MPI_SIZE, dimType.getDataType(),
                        dataset, Dimensions(1, 1, 1).getPointer());
            }

            H5Sclose(dsp_src);
        }
    }

    void DCDataSet::append(uint32_t count, uint32_t offset, uint32_t stride, const void* data)
    throw (DCException)
    {
#if defined SDC_DEBUG_OUTPUT
        std::cerr << "# DCDataSet::append #" << std::endl;
#endif

        if (!opened)
            throw DCException(getExceptionString("append: Dataset has not been opened/created."));

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "logical_size = " << getLogicalSize().toString() << std::endl;
#endif

        Dimensions target_offset(getLogicalSize());
        // extend size (dataspace) of existing dataset with count elements
        getLogicalSize()[0] += count;

        hsize_t *max_dims = new hsize_t[rank];
        for (size_t i = 0; i < rank; ++i)
            max_dims[i] = H5F_UNLIMITED;

        if (H5Sset_extent_simple(dataspace, 1, getLogicalSize().getPointer(), max_dims) < 0)
            throw DCException(getExceptionString("append: Failed to set new extent"));

        delete[] max_dims;
        max_dims = NULL;

#if defined SDC_DEBUG_OUTPUT
        std::cerr << "logical_size = " << getLogicalSize().toString() << std::endl;
#endif

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


        if (H5Dwrite(dataset, this->datatype, dsp_src, dataspace, dsetWriteProperties, data) < 0)
            throw DCException(getExceptionString("append: Failed to append dataset"));

        H5Sclose(dsp_src);
    }

}