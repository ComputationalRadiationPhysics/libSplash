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
#include <stdexcept>
#include <hdf5.h>

#include "DCException.hpp"
#include "Dimensions.hpp"
#include "CollectionType.hpp"
#include "basetypes/ColTypeDim.hpp"

namespace DCollector
{

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
        bool open(hid_t &group) throw (DCException);

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
         * @param rank number of dimensions
         * @param compression enables transparent compression on the data
         */
        void create(const CollectionType& colType, hid_t &group, const Dimensions size,
                uint32_t rank, bool compression) throw (DCException);

        void createReference(hid_t& refGroup, hid_t& srcGroup, DCDataSet &srcDataSet,
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
         * @param srcRank returns the dimensions of the read dataset
         * @param dst pointer to destination buffer for reading
         */
        void read(Dimensions dstBuffer,
                Dimensions dstOffset,
                Dimensions &sizeRead,
                uint32_t& srcRank,
                void* dst) throw (DCException);
        
        /**
         * Reads data from an open dataset.
         *
         * @param dstBuffer size of the buffer to read into
         * @param dstOffset offset in destination buffer to read to
         * @param srcSize the size of the requested buffer
         * @param srcOffset offset in source buffer to read from
         * @param sizeRead returns the size of the read dataset
         * @param srcRank returns the dimensions of the read dataset
         * @param dst pointer to destination buffer for reading
         */
        void read(Dimensions dstBuffer,
                Dimensions dstOffset,
                Dimensions srcSize,
                Dimensions srcOffset,
                Dimensions& sizeRead,
                uint32_t& srcRank,
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
        void append(uint32_t count,
                uint32_t offset,
                uint32_t stride,
                const void* data) throw (DCException);

        /**
         * Returns the number of dimensions of the dataset.
         *
         * @return number of dimensions
         */
        size_t getRank();

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
        
        
        size_t getDataTypeSize() throw (DCException);

        /**
         * Returns the name of the dataset.
         * 
         * @return dataset name
         */
        std::string getName();
        
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
        size_t rank;
        std::string name;
        bool opened;
        bool isReference;

        // property lists
        hid_t dsetProperties;
        hid_t dsetWriteProperties;
        hid_t dsetReadProperties;

        bool compression;
    private:
        std::string getExceptionString(std::string msg);

        ColTypeDim dimType;
    };

}

#endif	/* DCDATASET_HPP */

