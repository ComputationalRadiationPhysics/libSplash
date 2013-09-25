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



#ifndef DCDATASET_HPP
#define	DCDATASET_HPP

#include <stdint.h>
#include <string>
#include <hdf5.h>

#include "DCException.hpp"
#include "Dimensions.hpp"
#include "CollectionType.hpp"
#include "basetypes/ColTypeDim.hpp"

namespace DCollector
{

    /**
     * \cond HIDDEN_SYMBOLS
     */
    class DCDataSet
    {
    public:

        enum DCDataType
        {
            DCDT_UNKNOWN,
            DCDT_FLOAT32, DCDT_FLOAT64,
            DCDT_INT32, DCDT_INT64,
            DCDT_UINT32, DCDT_UINT64
        };

        /**
         * Constructor.
         *
         * @param name name for dataset
         */
        DCDataSet(const std::string name);

        /**
         * Destructor.
         */
        virtual ~DCDataSet();

        /**
         * Opens an existing dataset.
         *
         * @param group group for this dataset
         * @return true on success, false otherwise (dataset did not exist)
         */
        bool open(hid_t group) throw (DCException);

        /**
         * Closes the dataset.
         */
        void close() throw (DCException);

        /**
         * Creates a new dataset.
         *
         * @param colType collection type of the dataset
         * @param group group for this dataset
         * @param size target size
         * @param ndims number of dimensions
         * @param compression enables transparent compression on the data
         */
        void create(const CollectionType& colType, hid_t group, const Dimensions size,
                uint32_t ndims, bool compression) throw (DCException);

        /**
         * Create an object reference
         * @param refGroup handle to group for reference
         * @param srcGroup handle to group with source dataset
         * @param srcDataSet source dataset
         */
        void createReference(hid_t refGroup, hid_t srcGroup, DCDataSet &srcDataSet)
        throw (DCException);

        /**
         * Creates a dataset region reference
         * @param refGroup handle to group for reference
         * @param srcGroup handle to group with source dataset
         * @param srcDataSet source dataset
         * @param count reference region count
         * @param offset reference region offset
         * @param stride reference region stride
         */
        void createReference(hid_t refGroup, hid_t srcGroup, DCDataSet &srcDataSet,
                Dimensions count,
                Dimensions offset,
                Dimensions stride) throw (DCException);

        /**
         * Writes data to an open dataset.
         *
         * @param srcBuffer size of source buffer to read from
         * @param data source buffer to read from
         */
        void write(Dimensions srcBuffer, const void* data) throw (DCException);

        /**
         * Writes data to an open dataset.
         *
         * @param srcBuffer size of source buffer to read from
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcOffset offset in source buffer for reading
         * @param srcData size of data in source buffer to read
         * @param data source buffer to read from
         */
        void write(Dimensions srcBuffer, Dimensions srcStride, Dimensions srcOffset,
                Dimensions srcData, const void* data) throw (DCException);

        /**
         * Writes data to an open dataset.
         *
         * @param srcBuffer size of source buffer to read from
         * @param srcStride sizeof striding in each dimension. 1 means 'no stride'
         * @param srcOffset offset in source buffer for reading
         * @param srcData size of data in source buffer to read
         * @param dstOffset offset in dataset for writing
         * @param data source buffer to read from
         */
        void write(Dimensions srcBuffer, Dimensions srcStride, Dimensions srcOffset,
                Dimensions srcData, Dimensions dstOffset, const void* data) throw (DCException);

        /**
         * Reads data from an open dataset.
         *
         * @param dstBuffer size of the buffer to read into
         * @param dstOffset offset in destination buffer to read to
         * @param sizeRead returns the size of the read dataset
         * @param srcNDims returns the dimensions of the read dataset
         * @param dst pointer to destination buffer for reading
         */
        void read(Dimensions dstBuffer,
                Dimensions dstOffset,
                Dimensions &sizeRead,
                uint32_t& srcNDims,
                void* dst) throw (DCException);

        /**
         * Reads data from an open dataset.
         *
         * @param dstBuffer size of the buffer to read into
         * @param dstOffset offset in destination buffer to read to
         * @param srcSize the size of the requested buffer
         * @param srcOffset offset in source buffer to read from
         * @param sizeRead returns the size of the read dataset
         * @param srcNDims returns the dimensions of the read dataset
         * @param dst pointer to destination buffer for reading
         */
        void read(Dimensions dstBuffer,
                Dimensions dstOffset,
                Dimensions srcSize,
                Dimensions srcOffset,
                Dimensions& sizeRead,
                uint32_t& srcNDims,
                void* dst) throw (DCException);

        /**
         * Appends data to an open 1-dimensional dataset.
         *
         * @param count number of elements to append
         * @param offset the offset to be used for reading from the data buffer.
         * @param stride size of striding to be used for reading from the data buffer. 
         * data must contain at least (striding * count) elements. 
         * 1 means 'no stride'
         * @param data data for appending
         */
        void append(size_t count,
                size_t offset,
                size_t stride,
                const void* data) throw (DCException);

        /**
         * Returns the number of dimensions of the dataset.
         *
         * @return number of dimensions
         */
        size_t getNDims();

        /**
         * Returns the DataSpace associated with this DataSet.
         * A DCException is thrown if the DataSet has not been
         * properly opened/created.
         * 
         * @return the DataSpace
         */
        hid_t getDataSpace() throw (DCException);

        /**
         * Returns the DCDataType associated with this DataSet.
         * A DCException is thrown if the DataSet has not been
         * properly opened/created or if its type is not a
         * known trivial type.
         * 
         * @return the DataType
         */
        DCDataType getDCDataType() throw (DCException);


        /**
         * Returns the size of the underlying HDF5 datatype
         * @return size of HDF5 datatype
         */
        size_t getDataTypeSize() throw (DCException);

        /**
         * Returns the name of the dataset.
         * 
         * @return dataset name
         */
        std::string getName();
        
        static void splitPath(const std::string fullName, std::string &path, std::string &name);
        
        static void getFullDataPath(const std::string fullUserName, const std::string pathBase,
            uint32_t id, std::string &path, std::string &name);

    protected:
        void setChunking(size_t typeSize) throw (DCException);
        void setCompression() throw (DCException);

        Dimensions& getLogicalSize();
        Dimensions getPhysicalSize();

        hid_t dataset;
        hid_t datatype;
        hid_t dataspace;
        hdset_reg_ref_t regionRef;
        Dimensions logicalSize;
        size_t ndims;
        std::string name;
        bool opened;
        bool isReference;
        bool checkExistence;

        // property lists
        hid_t dsetProperties;
        hid_t dsetWriteProperties;
        hid_t dsetReadProperties;

        bool compression;
    private:
        std::string getExceptionString(std::string msg);

        ColTypeDim dimType;
    };
    /**
     * \endcond
     */

}

#endif	/* DCDATASET_HPP */

