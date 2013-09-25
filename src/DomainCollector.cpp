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



#include <algorithm>
#include <cassert>
#include <math.h>

#include "basetypes/ColTypeDim.hpp"
#include "basetypes/ColTypeInt.hpp"
#include "DomainCollector.hpp"
#include "core/DCDataSet.hpp"

namespace DCollector
{

    DomainCollector::DomainCollector(uint32_t maxFileHandles) :
    SerialDataCollector(maxFileHandles)
    {
    }

    DomainCollector::~DomainCollector()
    {
    }

    /*size_t DomainCollector::getGlobalElements(int32_t id,
            const char* name)
    throw (DCException)
    {
        if ((fileStatus != FST_MERGING) && (fileStatus != FST_READING))
            throw DCException("DomainCollector::getTotalElements: this access is not permitted");

        size_t total_elements = 0;
        Dimensions mpi_position(0, 0, 0);
        Dimensions access_mpi_size;

        if (fileStatus == FST_MERGING)
            access_mpi_size.set(mpiTopology);
        else
            access_mpi_size.set(1, 1, 1);

        DomDataClass data_class = UndefinedType;
        readAttribute(id, name, DOMCOL_ATTR_CLASS, &data_class, &mpi_position);

        if (data_class == DomainCollector::GridType)
        {
            // For Grid data, we can just read from the last MPI position since
            // all processes need to write as a regular grid.
            Dimensions subdomain_size;
            Dimensions subdomain_offset;

            mpi_position.set(access_mpi_size[0] - 1, access_mpi_size[1] - 1, access_mpi_size[2] - 1);

            readGlobalSizeFallback(id, name, subdomain_size.getPointer(), &mpi_position);

            total_elements = subdomain_size.getScalarSize();
        } else
        {
            // For Poly data, we currently need to open every file (slow!)
            Dimensions subdomain_elements;
            for (size_t z = 0; z < access_mpi_size[2]; z++)
            {
                for (size_t y = 0; y < access_mpi_size[1]; y++)
                {
                    for (size_t x = 0; x < access_mpi_size[0]; x++)
                    {
                        mpi_position.set(x, y, z);

                        readAttribute(id, name, DOMCOL_ATTR_ELEMENTS,
                                subdomain_elements.getPointer(), &mpi_position);

                        total_elements += subdomain_elements.getScalarSize();
                    }
                }
            }
        }

        return total_elements;
    }*/

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

        Dimensions domain_size;
        Dimensions domain_offset;

        readGlobalSizeFallback(id, name, domain_size.getPointer(), &mpi_position);
        readGlobalSizeFallback(id, name, domain_offset.getPointer(), &mpi_position);

        return Domain(domain_offset, domain_size);
    }

    /*size_t DomainCollector::getLocalElements(int32_t id,
            const char* name) throw (DCException)
    {
        if ((fileStatus != FST_MERGING) && (fileStatus != FST_READING))
            throw DCException("DomainCollector::getLocalDomain: this access is not permitted");
        
        // this accesses the local information, for both normal and merged read
        Dimensions mpi_position(0, 0, 0);
        
        
    }*/

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
        readAttribute(id, name, DOMCOL_ATTR_OFFSET,
                fileDomain.getOffset().getPointer(), &mpiPosition);

        readAttribute(id, name, DOMCOL_ATTR_SIZE,
                fileDomain.getSize().getPointer(), &mpiPosition);

        Domain request_domain(requestOffset, requestSize);

        return Domain::testIntersection(request_domain, fileDomain);
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
#if (DC_DEBUG == 1)
        std::cerr << "\nloading from mpi_position " << mpiPosition.toString() << std::endl;
#endif

        bool readResult = false;
        Domain client_domain;
        Domain request_domain(requestOffset, requestSize);

        readAttribute(id, name, DOMCOL_ATTR_OFFSET,
                client_domain.getOffset().getPointer(), &mpiPosition);

        readAttribute(id, name, DOMCOL_ATTR_SIZE,
                client_domain.getSize().getPointer(), &mpiPosition);

        Dimensions data_elements;
        readAttribute(id, name, DOMCOL_ATTR_ELEMENTS,
                data_elements.getPointer(), &mpiPosition);

        DomDataClass tmp_data_class = UndefinedType;
        readAttribute(id, name, DOMCOL_ATTR_CLASS, &tmp_data_class, &mpiPosition);

        if (tmp_data_class == GridType && data_elements != client_domain.getSize())
            throw DCException("DomainCollector::readDomain: Number of data elements must match domain size for Grid data.");

        if (*dataClass == UndefinedType)
        {
            *dataClass = tmp_data_class;
        } else
            if (tmp_data_class != *dataClass)
        {
            throw DCException("DomainCollector::readDomain: Data classes in files are inconsistent!");
        }

#if (DC_DEBUG == 1)
        std::cerr << "clientdom. = " << client_domain.toString() << std::endl;
        std::cerr << "requestdom. = " << request_domain.toString() << std::endl;
#endif

        // test on intersection and add new DomainData to the container if necessary
        if (Domain::testIntersection(request_domain, client_domain))
        {
            readResult = true;

            // Poly data has no internal grid structure, 
            // so the whole chunk has to be read and is added to the DataContainer.
            if (*dataClass == PolyType)
            {
#if (DC_DEBUG == 1)
                std::cerr << "dataclass = Poly" << std::endl;
#endif
                if (data_elements.getScalarSize() > 0)
                {
                    std::stringstream group_id_name;
                    group_id_name << SDC_GROUP_DATA << "/" << id;
                    std::string group_id_string = group_id_name.str();

                    hid_t group_id = H5Gopen(handles.get(mpiPosition), group_id_string.c_str(), H5P_DEFAULT);
                    if (group_id < 0)
                        throw DCException("DomainCollector::readDomain: group not found");

                    size_t datatype_size = 0;
                    DCDataSet::DCDataType dc_datatype = DCDataSet::DCDT_UNKNOWN;

                    try
                    {
                        DCDataSet tmp_dataset(name);
                        tmp_dataset.open(group_id);

                        datatype_size = tmp_dataset.getDataTypeSize();
                        dc_datatype = tmp_dataset.getDCDataType();

                        tmp_dataset.close();
                    } catch (DCException e)
                    {
                        H5Gclose(group_id);
                        throw e;
                    }

                    H5Gclose(group_id);

                    DomainData *client_data = new DomainData(client_domain,
                            data_elements, datatype_size, dc_datatype);

                    if (lazyLoad)
                    {
                        client_data->setLoadingReference(*dataClass,
                                handles.get(mpiPosition), id, name,
                                data_elements,
                                Dimensions(0, 0, 0),
                                Dimensions(0, 0, 0),
                                Dimensions(0, 0, 0));
                    } else
                    {
                        Dimensions elements_read;
                        uint32_t src_dims = 0;
                        readInternal(handles.get(mpiPosition), id, name,
                                data_elements,
                                Dimensions(0, 0, 0),
                                Dimensions(0, 0, 0),
                                Dimensions(0, 0, 0),
                                elements_read,
                                src_dims,
                                client_data->getData());

#if (DC_DEBUG == 1)
                        std::cerr << elements_read.toString() << std::endl;
                        std::cerr << data_elements.toString() << std::endl;
#endif

                        if (!(elements_read == data_elements))
                            throw DCException("DomainCollector::readDomain: Sizes are not equal but should be (1).");
                    }

                    dataContainer->add(client_data);
                } else
                {
#if (DC_DEBUG == 1)
                    std::cerr << "skipping entry with 0 elements" << std::endl;
#endif
                }
            } else
                // For Grid data, only the subchunk is read into its target position
                // in the destination buffer.
                if (*dataClass == GridType)
            {
#if (DC_DEBUG == 1)
                std::cerr << "dataclass = Grid" << std::endl;
#endif

                // When the first intersection is found, the whole destination 
                // buffer is allocated and added to the container.
                if (dataContainer->getNumSubdomains() == 0)
                {
                    std::stringstream group_id_name;
                    group_id_name << SDC_GROUP_DATA << "/" << id;
                    std::string group_id_string = group_id_name.str();

                    hid_t group_id = H5Gopen(handles.get(mpiPosition), group_id_string.c_str(), H5P_DEFAULT);
                    if (group_id < 0)
                        throw DCException("DomainCollector::readDomain: group not found");

                    size_t datatype_size = 0;
                    DCDataSet::DCDataType dc_datatype = DCDataSet::DCDT_UNKNOWN;

                    try
                    {
                        DCDataSet tmp_dataset(name);
                        tmp_dataset.open(group_id);

                        datatype_size = tmp_dataset.getDataTypeSize();
                        dc_datatype = tmp_dataset.getDCDataType();

                        tmp_dataset.close();
                    } catch (DCException e)
                    {
                        H5Gclose(group_id);
                        throw e;
                    }

                    H5Gclose(group_id);

                    DomainData *target_data = new DomainData(
                            request_domain, request_domain.getSize(),
                            datatype_size, dc_datatype);

                    dataContainer->add(target_data);
                }

                // Compute the offsets and sizes for reading and
                // writing this intersection.
                Dimensions dst_offset(0, 0, 0);
                Dimensions src_size(1, 1, 1);
                Dimensions src_offset(0, 0, 0);

                Dimensions client_start = client_domain.getOffset();
                Dimensions client_size = client_domain.getSize();

                size_t rank = getNDims(handles.get(mpiPosition), id, name);

                for (uint32_t i = 0; i < rank; ++i)
                {
                    dst_offset[i] = std::max((int64_t) client_domain.getOffset()[i] - (int64_t) requestOffset[i], (int64_t) 0);

                    dst_offset[i] = std::max((int64_t) client_domain.getOffset()[i] -
                            (int64_t) requestOffset[i], (int64_t) 0);

                    if (requestOffset[i] <= client_start[i])
                    {
                        // request starts before/equal client offset
                        src_offset[i] = 0;

                        if (requestOffset[i] + requestSize[i] >= client_start[i] + client_size[i])
                            // end of request stretches beyond client limits
                            src_size[i] = client_size[i];
                        else
                            // end of request within client limits
                            src_size[i] = requestOffset[i] + requestSize[i] - client_start[i];
                    } else
                    {
                        // request starts after client offset
                        src_offset[i] = requestOffset[i] - client_start[i];

                        if (requestOffset[i] + requestSize[i] >= client_start[i] + client_size[i])
                            // end of request stretches beyond client limits
                            src_size[i] = client_size[i] - src_offset[i];
                        else
                            // end of request within client limits
                            src_size[i] = requestOffset[i] + requestSize[i] -
                                (client_start[i] + src_offset[i]);
                    }
                }

#if (DC_DEBUG == 1)
                std::cerr << "client_domain.getSize() = " <<
                        client_domain.getSize().toString() << std::endl;
                std::cerr << "data_elements = " <<
                        data_elements.toString() << std::endl;
                std::cerr << "dst_offset = " << dst_offset.toString() << std::endl;
                std::cerr << "src_size = " << src_size.toString() << std::endl;
                std::cerr << "src_offset = " << src_offset.toString() << std::endl;

                assert(src_size[0] <= request_domain.getSize()[0]);
                assert(src_size[1] <= request_domain.getSize()[1]);
                assert(src_size[2] <= request_domain.getSize()[2]);
#endif

                // read intersecting partition into destination buffer
                Dimensions elements_read(0, 0, 0);
                uint32_t src_dims = 0;
                readInternal(handles.get(mpiPosition), id, name,
                        dataContainer->getIndex(0)->getSize(),
                        dst_offset,
                        src_size,
                        src_offset,
                        elements_read,
                        src_dims,
                        dataContainer->getIndex(0)->getData());

#if (DC_DEBUG == 1)
                std::cerr << "elements_read = " << elements_read.toString() << std::endl;
#endif

                if (!(elements_read == src_size))
                    throw DCException("DomainCollector::readDomain: Sizes are not equal but should be (2).");
            }
        } else
        {
#if (DC_DEBUG == 1)
            std::cerr << "no loading from this MPI position required" << std::endl;
#endif
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

#if (DC_DEBUG == 1)
        std::cerr << "requestOffset = " << requestOffset.toString() << std::endl;
        std::cerr << "requestSize = " << requestSize.toString() << std::endl;
#endif

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
        Dimensions current_mpi_pos(0, 0, 0);
        Dimensions point_dim(1, 1, 1);

        // try to find top-left corner of requested domain
        // stop if no new file can be tested for the requested domain
        // (current_mpi_pos does not change anymore)
        Dimensions last_mpi_pos(current_mpi_pos);
        bool found_start = false;
        do
        {
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
            return data_container;

        // found top-left corner of requested domain
        // In every file, domain attributes are read and evaluated.
        // If the file domain and the requested domain intersect,
        // the file domain is added to the DataContainer.

        // set new min_dims to top-left corner 
        max_dims = (mpi_size - Dimensions(1, 1, 1));
        min_dims = current_mpi_pos;

        bool found_last_entry = false;
        for (size_t z = min_dims[2]; z <= max_dims[2]; z++)
        {
            for (size_t y = min_dims[1]; y <= max_dims[1]; y++)
            {
                for (size_t x = min_dims[0]; x <= max_dims[0]; x++)
                {
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
                                max_dims[0] = x - 1;
                            else
                                max_dims[1] = y - 1;
                        } else
                        {
                            found_last_entry = true;
                            break;
                        }
                    }
                }

                if (found_last_entry)
                    break;
            }

            if (found_last_entry)
                break;
        }

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
            readInternal(loadingRef->handle,
                    loadingRef->id,
                    loadingRef->name.c_str(),
                    loadingRef->dstBuffer,
                    loadingRef->dstOffset,
                    loadingRef->srcSize,
                    loadingRef->srcOffset,
                    elements_read,
                    src_dims,
                    domainData->getData());

#if (DC_DEBUG == 1)
            std::cerr << elements_read.toString() << std::endl;
#endif

            if (!(elements_read == loadingRef->dstBuffer))
                throw DCException("DomainCollector::readDomainLazy: Sizes are not equal but should be (1).");

        } else
        {
            throw DCException("DomainCollector::readDomainLazy: data class not supported");
        }
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t rank,
            const Dimensions srcData,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {

        writeDomain(id, type, rank, srcData, Dimensions(1, 1, 1), srcData,
                Dimensions(0, 0, 0), name, domainOffset, domainSize, dataClass, buf);
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t rank,
            const Dimensions srcBuffer,
            const Dimensions srcData,
            const Dimensions srcOffset,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {

        writeDomain(id, type, rank, srcBuffer, Dimensions(1, 1, 1), srcData, srcOffset,
                name, domainOffset, domainSize, dataClass, buf);
    }

    void DomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t rank,
            const Dimensions srcBuffer,
            const Dimensions srcStride,
            const Dimensions srcData,
            const Dimensions srcOffset,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {
        ColTypeDim dim_t;
        ColTypeInt int_t;

        write(id, type, rank, srcBuffer, srcStride, srcData, srcOffset, name, buf);

        writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &dataClass);
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, domainSize.getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_OFFSET, domainOffset.getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_ELEMENTS, srcData.getPointer());
    }

    void DomainCollector::appendDomain(int32_t id,
            const CollectionType& type,
            size_t count,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
            const void* buf)
    throw (DCException)
    {
        appendDomain(id, type, count, 0, 1, name, domainOffset, domainSize, buf);
    }

    void DomainCollector::appendDomain(int32_t id,
            const CollectionType& type,
            size_t count,
            size_t offset,
            size_t striding,
            const char* name,
            const Dimensions domainOffset,
            const Dimensions domainSize,
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
            readAttribute(id, name, DOMCOL_ATTR_ELEMENTS, elements.getPointer(), NULL);
        } catch (DCException expected_exception)
        {
            // nothing to do here but to make sure elements is set correctly
            elements.set(0, 1, 1);
        }

        fileStatus = old_file_status;

        elements[0] = elements[0] + count;

        append(id, type, count, offset, striding, name, buf);

        writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &data_class);
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, domainSize.getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_OFFSET, domainOffset.getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_ELEMENTS, elements.getPointer());
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
