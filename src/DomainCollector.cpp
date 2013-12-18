/**
 * Copyright 2013 Felix Schmitt, Axel Huebl
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


#include <algorithm>
#include <cassert>
#include <math.h>

#include "splash/basetypes/basetypes.hpp"
#include "splash/DomainCollector.hpp"
#include "splash/core/DCDataSet.hpp"
#include "splash/core/DCGroup.hpp"
#include "splash/core/DCAttribute.hpp"
#include "splash/core/logging.hpp"

namespace splash
{

    DomainCollector::DomainCollector(uint32_t maxFileHandles) :
    SerialDataCollector(maxFileHandles)
    {
    }

    DomainCollector::~DomainCollector()
    {
    }

    Domain DomainCollector::getGlobalDomain(int32_t id,
            const char* name)
    throw (DCException)
    {
        if ((fileStatus != FST_MERGING) && (fileStatus != FST_READING))
            throw DCException("DomainCollector::getGlobalDomain: this access is not permitted");

        Dimensions mpi_size;
        if (fileStatus == FST_MERGING)
            mpi_size.set(mpiTopology);
        else
            mpi_size.set(1, 1, 1);

        // Since all MPI processes must write domain information as a regular grid,
        // we can just open the last MPI position to get the total size.
        Dimensions mpi_position(mpi_size[0] - 1, mpi_size[1] - 1, mpi_size[2] - 1);

        Dimensions global_domain_size;
        Dimensions global_domain_offset;

        readGlobalSizeFallback(id, name, global_domain_size.getPointer(), &mpi_position);
        readGlobalOffsetFallback(id, name, global_domain_offset.getPointer(), &mpi_position);

        return Domain(global_domain_offset, global_domain_size);
    }

    Domain DomainCollector::getLocalDomain(int32_t id,
            const char* name) throw (DCException)
    {
        if ((fileStatus != FST_MERGING) && (fileStatus != FST_READING))
            throw DCException("DomainCollector::getLocalDomain: this access is not permitted");

        // this accesses the local information, for both normal and merged read
        Dimensions mpi_position(0, 0, 0);

        Dimensions domain_size;
        Dimensions domain_offset;

        readAttribute(id, name, DOMCOL_ATTR_SIZE, domain_size.getPointer(), &mpi_position);
        readAttribute(id, name, DOMCOL_ATTR_OFFSET, domain_offset.getPointer(), &mpi_position);

        return Domain(domain_offset, domain_size);
    }

    bool DomainCollector::readDomainInfoForRank(
            Dimensions mpiPosition,
            int32_t id,
            const char* name,
            Dimensions requestOffset,
            Dimensions requestSize,
            Domain &fileDomain)
    throw (DCException)
    {
        {
            hid_t dset_handle = openDatasetHandle(id, name, &mpiPosition);

            DCAttribute::readAttribute(DOMCOL_ATTR_OFFSET, dset_handle,
                    fileDomain.getOffset().getPointer());

            DCAttribute::readAttribute(DOMCOL_ATTR_SIZE, dset_handle,
                    fileDomain.getSize().getPointer());

            closeDatasetHandle(dset_handle);
        }

        // zero request sizes will not intersect with anything
        if (requestSize == Dimensions(0,0,0))
            return false;

        Domain request_domain(requestOffset, requestSize);

        return Domain::testIntersection(request_domain, fileDomain);
    }

    void DomainCollector::readGridInternal(
            DataContainer *dataContainer,
            Dimensions mpiPosition,
            int32_t id,
            const char* name,
            Domain &clientDomain,
            Domain &requestDomain
            )
    throw (DCException)
    {
        log_msg(3, "dataclass = Grid");

        // When the first intersection is found, the whole destination 
        // buffer is allocated and added to the container.
        if (dataContainer->getNumSubdomains() == 0)
        {
            std::stringstream group_id_name;
            group_id_name << SDC_GROUP_DATA << "/" << id;
            std::string group_id_string = group_id_name.str();

            DCGroup group;
            group.open(handles.get(mpiPosition), group_id_string);

            size_t datatype_size = 0;
            DCDataType dc_datatype = DCDT_UNKNOWN;

            try
            {
                DCDataSet tmp_dataset(name);
                tmp_dataset.open(group.getHandle());

                datatype_size = tmp_dataset.getDataTypeSize();
                dc_datatype = tmp_dataset.getDCDataType();

                tmp_dataset.close();
            } catch (DCException e)
            {
                throw e;
            }

            group.close();

            DomainData *target_data = new DomainData(
                    requestDomain, requestDomain.getSize(),
                    datatype_size, dc_datatype);

            dataContainer->add(target_data);
        }

        // Compute the offsets and sizes for reading and
        // writing this intersection.
        Dimensions dst_offset(0, 0, 0);
        Dimensions src_size(1, 1, 1);
        Dimensions src_offset(0, 0, 0);

        Dimensions client_start = clientDomain.getOffset();
        Dimensions client_size = clientDomain.getSize();

        size_t ndims = getNDims(handles.get(mpiPosition), id, name);
        const Dimensions &request_offset = requestDomain.getOffset();
        const Dimensions &request_size = requestDomain.getSize();

        for (uint32_t i = 0; i < ndims; ++i)
        {
            dst_offset[i] = std::max((int64_t) clientDomain.getOffset()[i] - (int64_t) request_offset[i], (int64_t) 0);

            dst_offset[i] = std::max((int64_t) clientDomain.getOffset()[i] -
                    (int64_t) request_offset[i], (int64_t) 0);

            if (request_offset[i] <= client_start[i])
            {
                // request starts before/equal client offset
                src_offset[i] = 0;

                if (request_offset[i] + request_size[i] >= client_start[i] + client_size[i])
                    // end of request stretches beyond client limits
                    src_size[i] = client_size[i];
                else
                    // end of request within client limits
                    src_size[i] = request_offset[i] + request_size[i] - client_start[i];
            } else
            {
                // request starts after client offset
                src_offset[i] = request_offset[i] - client_start[i];

                if (request_offset[i] + request_size[i] >= client_start[i] + client_size[i])
                    // end of request stretches beyond client limits
                    src_size[i] = client_size[i] - src_offset[i];
                else
                    // end of request within client limits
                    src_size[i] = request_offset[i] + request_size[i] -
                        (client_start[i] + src_offset[i]);
            }
        }

        log_msg(3,
                "clientDomain.getSize() = %s\n"
                "dst_offset = %s "
                "src_size = %s "
                "src_offset = %s",
                clientDomain.getSize().toString().c_str(),
                dst_offset.toString().c_str(),
                src_size.toString().c_str(),
                src_offset.toString().c_str());

        assert(src_size[0] <= request_size[0]);
        assert(src_size[1] <= request_size[1]);
        assert(src_size[2] <= request_size[2]);

        // read intersecting partition into destination buffer
        Dimensions elements_read(0, 0, 0);
        uint32_t src_dims = 0;
        readDataSet(handles.get(mpiPosition), id, name,
                dataContainer->getIndex(0)->getSize(),
                dst_offset,
                src_size,
                src_offset,
                elements_read,
                src_dims,
                dataContainer->getIndex(0)->getData());

        if (!(elements_read == src_size))
            throw DCException("DomainCollector::readGridInternal: Sizes are not equal but should be (2).");
    }

    void DomainCollector::readPolyInternal(
            DataContainer *dataContainer,
            Dimensions mpiPosition,
            int32_t id,
            const char* name,
            const Dimensions &dataSize,
            Domain &clientDomain,
            bool lazyLoad
            )
    throw (DCException)
    {
        log_msg(3, "dataclass = Poly");

        if (dataSize.getScalarSize() > 0)
        {
            std::stringstream group_id_name;
            group_id_name << SDC_GROUP_DATA << "/" << id;
            std::string group_id_string = group_id_name.str();

            DCGroup group;
            group.open(handles.get(mpiPosition), group_id_string);

            size_t datatype_size = 0;
            DCDataType dc_datatype = DCDT_UNKNOWN;

            try
            {
                DCDataSet tmp_dataset(name);
                tmp_dataset.open(group.getHandle());

                datatype_size = tmp_dataset.getDataTypeSize();
                dc_datatype = tmp_dataset.getDCDataType();

                tmp_dataset.close();
            } catch (DCException e)
            {
                throw e;
            }

            group.close();

            DomainData *client_data = new DomainData(clientDomain,
                    dataSize, datatype_size, dc_datatype);

            if (lazyLoad)
            {
                client_data->setLoadingReference(PolyType,
                        handles.get(mpiPosition), id, name,
                        dataSize,
                        Dimensions(0, 0, 0),
                        Dimensions(0, 0, 0),
                        Dimensions(0, 0, 0));
            } else
            {
                Dimensions size_read;
                uint32_t src_ndims = 0;
                readCompleteDataSet(handles.get(mpiPosition), id, name,
                        dataSize,
                        Dimensions(0, 0, 0),
                        Dimensions(0, 0, 0),
                        size_read,
                        src_ndims,
                        client_data->getData());

                if (!(size_read == dataSize))
                    throw DCException("DomainCollector::readPolyInternal: Sizes are not equal but should be (1).");
            }

            dataContainer->add(client_data);
        } else
        {
            log_msg(3, "skipping entry with 0 elements");
        }
    }

    bool DomainCollector::readDomainDataForRank(
            DataContainer *dataContainer,
            DomDataClass *dataClass,
            Dimensions mpiPosition,
            int32_t id,
            const char* name,
            Dimensions requestOffset,
            Dimensions requestSize,
            bool lazyLoad)
    throw (DCException)
    {
        log_msg(3, "loading from mpi_position %s", mpiPosition.toString().c_str());

        bool readResult = false;
        Domain client_domain;
        Domain request_domain(requestOffset, requestSize);
        Dimensions data_size;
        DomDataClass tmp_data_class = UndefinedType;

        {
            hid_t dset_handle = openDatasetHandle(id, name, &mpiPosition);

            DCAttribute::readAttribute(DOMCOL_ATTR_OFFSET, dset_handle,
                    client_domain.getOffset().getPointer());

            DCAttribute::readAttribute(DOMCOL_ATTR_SIZE, dset_handle,
                    client_domain.getSize().getPointer());

            DCAttribute::readAttribute(DOMCOL_ATTR_CLASS, dset_handle,
                    &tmp_data_class);

            closeDatasetHandle(dset_handle);
        }

        readSizeInternal(handles.get(mpiPosition), id, name, data_size);

        log_msg(3,
                "clientdom. = %s "
                "requestdom.= %s "
                "data size  = %s",
                client_domain.toString().c_str(),
                request_domain.toString().c_str(),
                data_size.toString().c_str());

        const bool emptyRequest = ( data_size.getScalarSize() == 1 &&
                                    client_domain.getSize().getScalarSize() == 0 );

        if (tmp_data_class == GridType && data_size != client_domain.getSize() &&
            !emptyRequest )
        {
            std::cout << data_size.toString() << ", " << client_domain.getSize().toString() << std::endl;
            throw DCException("DomainCollector::readDomain: Size of data must match domain size for Grid data.");
        }

        if (*dataClass == UndefinedType)
        {
            *dataClass = tmp_data_class;
        } else
            if (tmp_data_class != *dataClass)
        {
            throw DCException("DomainCollector::readDomain: Data classes in files are inconsistent!");
        }

        // test on intersection and add new DomainData to the container if necessary
        if (Domain::testIntersection(request_domain, client_domain))
        {
            readResult = true;

            switch (*dataClass)
            {
                case PolyType:
                    // Poly data has no internal grid structure, 
                    // so the whole chunk has to be read and is added to the DataContainer.
                    readPolyInternal(dataContainer, mpiPosition, id, name,
                            data_size, client_domain, lazyLoad);
                    break;
                case GridType:
                    // For Grid data, only the subchunk is read into its target position
                    // in the destination buffer.
                    readGridInternal(dataContainer, mpiPosition, id, name,
                            client_domain, request_domain);
                    break;
                default:
                    return false;
            }
        } else
        {
            log_msg(3, "no loading from this MPI position required");
        }

        return readResult;
    }

    DataContainer *DomainCollector::readDomain(int32_t id,
            const char* name,
            Dimensions requestOffset,
            Dimensions requestSize,
            DomDataClass* dataClass,
            bool lazyLoad)
    throw (DCException)
    {
        if ((fileStatus != FST_MERGING) && (fileStatus != FST_READING))
            throw DCException("DomainCollector::readDomain: this access is not permitted");

        DataContainer *data_container = new DataContainer();

        log_msg(3,
                "requestOffset = %s "
                "requestSize = %s",
                requestOffset.toString().c_str(),
                requestSize.toString().c_str());

        DomDataClass data_class = UndefinedType;
        Dimensions mpi_size;

        if (fileStatus == FST_MERGING)
        {
            mpi_size.set(mpiTopology);
        } else
            mpi_size.set(1, 1, 1);

        Dimensions min_dims(0, 0, 0);
        Dimensions max_dims(mpi_size);
        max_dims = max_dims - Dimensions(1, 1, 1);

        // try to find the lowest corner (probably the file with the
        // domain offset == globalOffset) of the full domain
        // for periodically moving (e.g. moving window) simulations
        // this happens to be _not_ at mpi_pos(0,0,0)
        {
            Dimensions delta(0,0,0);
            Dimensions belowZero(min_dims);
            Dimensions aboveZero(max_dims);
            Domain minDom;
            Domain globalDomain = getGlobalDomain(id, name);
            do
            {
                log_msg(4, "find zero: belowZero = %s, aboveZero = %s",
                        belowZero.toString().c_str(),
                        aboveZero.toString().c_str());

                Domain maxDom;
                readDomainInfoForRank( belowZero, id, name,
                                       Dimensions(0,0,0), Dimensions(0,0,0), minDom);
                readDomainInfoForRank( aboveZero, id, name,
                                       Dimensions(0,0,0), Dimensions(0,0,0), maxDom);

                log_msg(4, "find zero: minDom.getOffset() = %s, maxDom.getOffset() = %s",
                        minDom.getOffset().toString().c_str(),
                        maxDom.getOffset().toString().c_str());

                for (size_t i = 0; i < 3; ++i)
                {
                    // zero still between min and max?
                    if (minDom.getOffset()[i] > maxDom.getOffset()[i])
                    {
                        delta[i] = ceil(((double) aboveZero[i] - (double) belowZero[i]) / 2.0);
                        belowZero[i] += delta[i];
                    }
                    // found zero point for this i
                    else if (minDom.getOffset()[i] == globalDomain.getOffset()[i])
                        delta[i] = 0;
                    // jumped over the zero position during last +-=delta[i]
                    else
                    {
                        belowZero[i] -= delta[i];
                        aboveZero[i] -= delta[i];
                    }
                }

            } while (delta != Dimensions(0,0,0));

            // search above or below zero point?
            //
            // the file with the largest mpi position, but not necessarily
            // the one with the largest local domain offset
            Domain lastDom;
            readDomainInfoForRank( mpi_size - Dimensions(1, 1, 1), id, name,
                                   Dimensions(0,0,0), Dimensions(0,0,0), lastDom);
            for (size_t i = 0; i < 3; ++i)
            {
                if (requestOffset[i] <= lastDom.getBack()[i])
                {
                    min_dims[i] = belowZero[i];
                    max_dims[i] = mpi_size[i] - 1;
                }
                else
                {
                    min_dims[i] = 0;
                    belowZero[i] > 1 ? max_dims[i] = belowZero[i] - 1
                                     : max_dims[i] = 0;
                }
            }
        }

        Dimensions current_mpi_pos(min_dims);
        Dimensions point_dim(1, 1, 1);

        // try to find top-left corner of requested domain
        // stop if no new file can be tested for the requested domain
        // (current_mpi_pos does not change anymore)
        Dimensions last_mpi_pos(current_mpi_pos);
        bool found_start = false;
        do
        {
            log_msg(4, "find top-left: min_dims = %s, max_dims = %s",
                    min_dims.toString().c_str(),
                    max_dims.toString().c_str());

            Domain file_domain;
            last_mpi_pos = current_mpi_pos;

            // set current_mpi_pos to be the 'center' between min_dims and max_dims
            for (size_t i = 0; i < 3; ++i)
            {
                current_mpi_pos[i] = min_dims[i] +
                        ceil(((double) max_dims[i] - (double) min_dims[i]) / 2.0);
            }

            if (readDomainInfoForRank(current_mpi_pos, id, name,
                    requestOffset, point_dim, file_domain))
            {
                found_start = true;
                break;
            }

            for (size_t i = 0; i < 3; ++i)
            {
                if (requestOffset[i] >= file_domain.getOffset()[i])
                    min_dims[i] = current_mpi_pos[i];

                if (requestOffset[i] < file_domain.getOffset()[i])
                    max_dims[i] = current_mpi_pos[i] - 1;
            }
        } while (last_mpi_pos != current_mpi_pos);

        if (!found_start)
        {
            log_msg(2, "readDomain: no data found");
            return data_container;
        }

        // found top-left corner of requested domain
        // In every file, domain attributes are read and evaluated.
        // If the file domain and the requested domain intersect,
        // the file domain is added to the DataContainer.

        // set new min_dims to top-left corner
        for (size_t i = 0; i < 3; ++i )
            max_dims[i] = (current_mpi_pos[i] + mpi_size[i] - 1) % mpi_size[i];
        min_dims = current_mpi_pos;

        log_msg(3, "readDomain: Looking for matching domain data in range "
                "min_dims = %s "
                "max_dims = %s",
                min_dims.toString().c_str(),
                max_dims.toString().c_str());

        bool found_last_entry = false;
        // ..._lin indexes can range >= mpi_size since they are helper to
        // calculate the next index (x,y,z) in the periodic modulo system
        size_t z, z_lin = min_dims[2];
        do
        {
            z = z_lin % mpi_size[2];
            size_t y, y_lin = min_dims[1];
            do
            {
                y = y_lin % mpi_size[1];
                size_t x, x_lin = min_dims[0];
                do
                {
                    x = x_lin % mpi_size[0];
                    Dimensions mpi_position(x, y, z);

                    if (!readDomainDataForRank(data_container,
                            &data_class,
                            mpi_position,
                            id,
                            name,
                            requestOffset,
                            requestSize,
                            lazyLoad))
                    {
                        // readDomainDataForRank returns false if no intersection
                        // has been found.
                        // Cut max_dims in the currently extending dimension if
                        // nothing can be found there, anymore.
                        if (z == min_dims[2])
                        {
                            if (y == min_dims[1])
                            {
                                max_dims[0] = (x + mpi_size[0] - 1) % mpi_size[0];
                                x = max_dims[0];
                            }
                            else
                            {
                                max_dims[1] = (y + mpi_size[1] - 1) % mpi_size[1];
                                y = max_dims[1];
                            }
                        } else
                        {
                            found_last_entry = true;
                            break;
                        }
                    }

                    x_lin++;
                } while (x != max_dims[0]);

                if (found_last_entry)
                    break;

                y_lin++;
            } while (y != max_dims[1]);

            if (found_last_entry)
                break;

            z_lin++;
        } while (z != max_dims[2]);

        if (dataClass != NULL)
            *dataClass = data_class;

        return data_container;
    }

    void DomainCollector::readDomainLazy(DomainData *domainData)
    throw (DCException)
    {
        if (domainData == NULL)
        {
            throw DCException("DomainCollector::readDomainLazy: Invalid parameter, DomainData must not be NULL");
        }

        DomainH5Ref *loadingRef = domainData->getLoadingReference();
        if (loadingRef == NULL)
        {
            throw DCException("DomainCollector::readDomainLazy: This DomainData does not allow lazy loading");
        }

        if (loadingRef->dataClass == UndefinedType)
        {
            throw DCException("DomainCollector::readDomainLazy: DomainData has invalid data class");
        }

        if (loadingRef->dataClass == PolyType)
        {
            Dimensions elements_read;
            uint32_t src_dims = 0;
            readDataSet(loadingRef->handle,
                    loadingRef->id,
                    loadingRef->name.c_str(),
                    loadingRef->dstBuffer,
                    loadingRef->dstOffset,
                    loadingRef->srcSize,
                    loadingRef->srcOffset,
                    elements_read,
                    src_dims,
                    domainData->getData());

            if (!(elements_read == loadingRef->dstBuffer))
                throw DCException("DomainCollector::readDomainLazy: Sizes are not equal but should be (1).");

        } else
        {

            throw DCException("DomainCollector::readDomainLazy: data class not supported");
        }
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t ndims,
            const Dimensions srcData,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const Dimensions globalDomainOffset,
            const Dimensions globalDomainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {

        writeDomain(id, type, ndims, srcData, Dimensions(1, 1, 1), srcData,
                Dimensions(0, 0, 0), name, domainOffset, domainSize,
                globalDomainOffset, globalDomainSize, dataClass, buf);
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t ndims,
            const Dimensions srcBuffer,
            const Dimensions srcData,
            const Dimensions srcOffset,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const Dimensions globalDomainOffset,
            const Dimensions globalDomainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {

        writeDomain(id, type, ndims, srcBuffer, Dimensions(1, 1, 1), srcData, srcOffset,
                name, domainOffset, domainSize, globalDomainOffset, globalDomainSize,
                dataClass, buf);
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t ndims,
            const Dimensions srcBuffer,
            const Dimensions srcStride,
            const Dimensions srcData,
            const Dimensions srcOffset,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const Dimensions globalDomainOffset,
            const Dimensions globalDomainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {
        ColTypeDim dim_t;
        ColTypeInt int_t;

        write(id, type, ndims, srcBuffer, srcStride, srcData, srcOffset, name, buf);

        {

            hid_t dset_handle = openDatasetHandle(id, name, NULL);

            DCAttribute::writeAttribute(DOMCOL_ATTR_CLASS, int_t.getDataType(),
                    dset_handle, &dataClass);
            DCAttribute::writeAttribute(DOMCOL_ATTR_SIZE, dim_t.getDataType(),
                    dset_handle, domainSize.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_OFFSET, dim_t.getDataType(),
                    dset_handle, domainOffset.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_GLOBAL_SIZE, dim_t.getDataType(),
                    dset_handle, globalDomainSize.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_GLOBAL_OFFSET, dim_t.getDataType(),
                    dset_handle, globalDomainOffset.getPointer());

            closeDatasetHandle(dset_handle);
        }
    }

    void DomainCollector::appendDomain(int32_t id,
            const CollectionType& type,
            size_t count,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const Dimensions globalDomainOffset,
            const Dimensions globalDomainSize,
            const void* buf)
    throw (DCException)
    {

        appendDomain(id, type, count, 0, 1, name, domainOffset, domainSize,
                globalDomainOffset, globalDomainSize, buf);
    }

    void DomainCollector::appendDomain(int32_t id,
            const CollectionType& type,
            size_t count,
            size_t offset,
            size_t striding,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const Dimensions globalDomainOffset,
            const Dimensions globalDomainSize,
            const void* buf)
    throw (DCException)
    {
        ColTypeDim dim_t;
        ColTypeInt int_t;
        DomDataClass data_class = PolyType;
        Dimensions elements(1, 1, 1);

        // temporarly change file access status to allow read access
        FileStatusType old_file_status = fileStatus;
        fileStatus = FST_READING;

        // try to get the number of elements already written, if any.
        try
        {
            readSizeInternal(handles.get(0), id, name, elements);
        } catch (DCException expected_exception)
        {
            // nothing to do here but to make sure elements is set correctly
            elements.set(0, 1, 1);
        }

        fileStatus = old_file_status;

        elements[0] = elements[0] + count;

        append(id, type, count, offset, striding, name, buf);

        {

            hid_t dset_handle = openDatasetHandle(id, name, NULL);

            DCAttribute::writeAttribute(DOMCOL_ATTR_CLASS, int_t.getDataType(),
                    dset_handle, &data_class);
            DCAttribute::writeAttribute(DOMCOL_ATTR_SIZE, dim_t.getDataType(),
                    dset_handle, domainSize.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_OFFSET, dim_t.getDataType(),
                    dset_handle, domainOffset.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_GLOBAL_SIZE, dim_t.getDataType(),
                    dset_handle, globalDomainSize.getPointer());
            DCAttribute::writeAttribute(DOMCOL_ATTR_GLOBAL_OFFSET, dim_t.getDataType(),
                    dset_handle, globalDomainOffset.getPointer());

            closeDatasetHandle(dset_handle);
        }
    }

    void DomainCollector::readGlobalSizeFallback(int32_t id,
            const char *dataName,
            hsize_t* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        try
        {
            readAttribute(id, dataName, DOMCOL_ATTR_GLOBAL_SIZE, data, mpiPosition);
        } catch (DCException)
        {
            hsize_t local_size[3];
            readAttribute(id, dataName, DOMCOL_ATTR_SIZE, local_size, mpiPosition);

            for (int i = 0; i < 3; ++i)
                data[i] = mpiTopology[i] * local_size[i];
        }
    }

    void DomainCollector::readGlobalOffsetFallback(int32_t id,
            const char *dataName,
            hsize_t* data,
            Dimensions *mpiPosition)
    throw (DCException)
    {
        try
        {
            readAttribute(id, dataName, DOMCOL_ATTR_GLOBAL_OFFSET, data, mpiPosition);
        } catch (DCException)
        {
            for (int i = 0; i < 3; ++i)
                data[i] = 0;
        }
    }
}
