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

#include "splash/basetypes/basetypes.hpp"

#include "splash/ParallelDomainCollector.hpp"
#include "splash/core/DCParallelDataSet.hpp"
#include "splash/core/logging.hpp"

namespace splash
{

    std::string ParallelDomainCollector::getExceptionString(std::string func,
            std::string msg, const char *info)
    {
        std::stringstream full_msg;
        full_msg << "Exception for ParallelDomainCollector::" << func <<
                ": " << msg;

        if (info != NULL)
            full_msg << " (" << info << ")";

        return full_msg.str();
    }
    
    void ParallelDomainCollector::writeDomainAttributes(
            int32_t id,
            const char *name,
            DomDataClass dataClass,
            const Domain localDomain,
            const Domain globalDomain)
    {
        ColTypeInt32 int_t;
        ColTypeDim dim_t;
        writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &dataClass);
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, localDomain.getSize().getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_OFFSET, localDomain.getOffset().getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_GLOBAL_SIZE, globalDomain.getSize().getPointer());
        writeAttribute(id, dim_t, name, DOMCOL_ATTR_GLOBAL_OFFSET, globalDomain.getOffset().getPointer());
    }

    ParallelDomainCollector::ParallelDomainCollector(MPI_Comm comm, MPI_Info info,
            const Dimensions topology, uint32_t maxFileHandles) :
    ParallelDataCollector(comm, info, topology, maxFileHandles)
    {
    }

    ParallelDomainCollector::~ParallelDomainCollector()
    {
    }

    Domain ParallelDomainCollector::getGlobalDomain(int32_t id,
            const char* name)
    throw (DCException)
    {
        if (this->fileStatus == FST_CLOSED)
            throw DCException(getExceptionString("getGlobalDomain",
                "this access is not permitted", NULL));

        Domain domain;

        readAttribute(id, name, DOMCOL_ATTR_GLOBAL_SIZE, domain.getSize().getPointer());
        readAttribute(id, name, DOMCOL_ATTR_GLOBAL_OFFSET, domain.getOffset().getPointer());

        return domain;
    }

    Domain ParallelDomainCollector::getLocalDomain(int32_t id,
            const char* name)
    throw (DCException)
    {
        if (this->fileStatus == FST_CLOSED)
            throw DCException(getExceptionString("getLocalDomain",
                "this access is not permitted", NULL));

        Domain domain;

        readAttribute(id, name, DOMCOL_ATTR_SIZE, domain.getSize().getPointer());
        readAttribute(id, name, DOMCOL_ATTR_OFFSET, domain.getOffset().getPointer());

        return domain;
    }

    bool ParallelDomainCollector::readDomainDataForRank(
            DataContainer *dataContainer,
            DomDataClass *dataClass,
            int32_t id,
            const char* name,
            const Domain requestDomain,
            bool lazyLoad)
    throw (DCException)
    {
        Domain local_client_domain, global_client_domain;
        
        readAttribute(id, name, DOMCOL_ATTR_OFFSET, local_client_domain.getOffset().getPointer());
        readAttribute(id, name, DOMCOL_ATTR_SIZE, local_client_domain.getSize().getPointer());
        readAttribute(id, name, DOMCOL_ATTR_GLOBAL_OFFSET, global_client_domain.getOffset().getPointer());
        readAttribute(id, name, DOMCOL_ATTR_GLOBAL_SIZE, global_client_domain.getSize().getPointer());
        
        Domain client_domain(
                local_client_domain.getOffset() + global_client_domain.getOffset(),
                local_client_domain.getSize());

        Dimensions data_elements;
        read(id, name, data_elements, NULL);

        DomDataClass tmp_data_class = UndefinedType;
        readAttribute(id, name, DOMCOL_ATTR_CLASS, &tmp_data_class);

        if (tmp_data_class == GridType && data_elements != client_domain.getSize())
            throw DCException(getExceptionString("readDomainDataForRank",
                "Number of data elements must match domain size for Grid data.", NULL));

        if (*dataClass == UndefinedType)
        {
            *dataClass = tmp_data_class;
        } else
            if (tmp_data_class != *dataClass)
        {
            throw DCException(getExceptionString("readDomainDataForRank",
                    "Data classes in files are inconsistent!", NULL));
        }

        log_msg(3, "clientdom. = %s", client_domain.toString().c_str());
        log_msg(3, "requestdom. = %s", requestDomain.toString().c_str());

        // test on intersection and add new DomainData to the container if necessary
        if ((requestDomain.getSize().getScalarSize() > 0) && !Domain::testIntersection(requestDomain, client_domain))
            return false;

        // Poly data has no internal grid structure, 
        // so the whole chunk has to be read and is added to the DataContainer.
        if (*dataClass == PolyType)
        {
            log_msg(3, "dataclass = Poly");
            if (data_elements.getScalarSize() > 0)
            {
                std::stringstream group_id_name;
                group_id_name << SDC_GROUP_DATA << "/" << id;
                std::string group_id_string = group_id_name.str();

                hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
                if (group_id < 0)
                    throw DCException(getExceptionString("readDomainDataForRank",
                        "group not found", group_id_string.c_str()));

                size_t datatype_size = 0;
                DCDataType dc_datatype = DCDT_UNKNOWN;

                try
                {
                    DCParallelDataSet tmp_dataset(name);
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
                            handles.get(id), id, name,
                            data_elements,
                            Dimensions(0, 0, 0),
                            Dimensions(0, 0, 0),
                            Dimensions(0, 0, 0));
                } else
                {
                    Dimensions elements_read;
                    uint32_t src_rank = 0;
                    readCompleteDataSet(handles.get(id), id, name,
                            data_elements,
                            Dimensions(0, 0, 0),
                            Dimensions(0, 0, 0),
                            elements_read,
                            src_rank,
                            client_data->getData());

                    if (!(elements_read == data_elements))
                        throw DCException(getExceptionString("readDomainDataForRank",
                            "Sizes are not equal but should be (1).", NULL));
                }

                dataContainer->add(client_data);
            } else
            {
                log_msg(3, "skipping entry with 0 elements");
            }
        } else
            // For Grid data, only the subchunk is read into its target position
            // in the destination buffer.
            if (*dataClass == GridType)
        {
            log_msg(3, "dataclass = Grid");

            // When the first intersection is found, the whole destination 
            // buffer is allocated and added to the container.
            if (dataContainer->getNumSubdomains() == 0)
            {
                std::stringstream group_id_name;
                group_id_name << SDC_GROUP_DATA << "/" << id;
                std::string group_id_string = group_id_name.str();

                hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
                if (group_id < 0)
                    throw DCException(getExceptionString("readDomainDataForRank",
                        "group not found", group_id_string.c_str()));

                size_t datatype_size = 0;
                DCDataType dc_datatype = DCDT_UNKNOWN;

                try
                {
                    DCParallelDataSet tmp_dataset(name);
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
                        requestDomain, requestDomain.getSize(),
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

            size_t ndims = getNDims(handles.get(id), id, name);

            const Dimensions request_offset = requestDomain.getOffset();
            const Dimensions request_size = requestDomain.getSize();
            for (uint32_t i = 0; i < ndims; ++i)
            {
                dst_offset[i] = std::max((int64_t) client_domain.getOffset()[i] -
                        (int64_t) request_offset[i], (int64_t) 0);

                if (request_offset[i] <= client_start[i])
                {
                    src_offset[i] = 0;

                    if (request_offset[i] + request_size[i] >= client_start[i] + client_size[i])
                        src_size[i] = client_size[i];
                    else
                        src_size[i] = request_offset[i] + request_size[i] - client_start[i];
                } else
                {
                    src_offset[i] = request_offset[i] - client_start[i];

                    if (request_offset[i] + request_size[i] >= client_start[i] + client_size[i])
                        src_size[i] = client_size[i] - src_offset[i];
                    else
                        src_size[i] = request_offset[i] + request_size[i] -
                            (client_start[i] + src_offset[i]);
                }
            }

            log_msg(3,
                    "client_domain.getSize() = %s\n"
                    "data_elements = %s\n"
                    "dst_offset = %s\n"
                    "src_size = %s\n"
                    "src_offset = %s",
                    client_domain.getSize().toString().c_str(),
                    data_elements.toString().c_str(),
                    dst_offset.toString().c_str(),
                    src_size.toString().c_str(),
                    src_offset.toString().c_str());

            assert(src_size[0] <= requestDomain.getSize()[0]);
            assert(src_size[1] <= requestDomain.getSize()[1]);
            assert(src_size[2] <= requestDomain.getSize()[2]);

            // read intersecting partition into destination buffer
            Dimensions elements_read(0, 0, 0);
            uint32_t src_rank = 0;
            {
                readDataSet(handles.get(id), id, name,
                        dataContainer->getIndex(0)->getSize(),
                        dst_offset,
                        src_size,
                        src_offset,
                        elements_read,
                        src_rank,
                        dataContainer->getIndex(0)->getData());
            }

            log_msg(3, "elements_read = %s", elements_read.toString().c_str());
            bool read_success = true;

            if ((request_size.getScalarSize() == 0) && (elements_read.getScalarSize() != 0))
                read_success = false;

            if ((request_size.getScalarSize() != 0) && (elements_read != src_size))
                read_success = false;

            if (!read_success)
            {
                throw DCException(getExceptionString("readDomainDataForRank",
                    "Sizes are not equal but should be (2).", NULL));
            }
        }

        return true;
    }

    DataContainer *ParallelDomainCollector::readDomain(int32_t id,
            const char* name,
            const Domain requestDomain,
            DomDataClass* dataClass,
            bool lazyLoad)
    throw (DCException)
    {
        if (fileStatus == FST_CLOSED)
            throw DCException(getExceptionString("readDomain",
                "this access is not permitted", NULL));

        DataContainer * data_container = new DataContainer();

        log_msg(3, "requestDomain = %s", requestDomain.toString().c_str());

        DomDataClass data_class = UndefinedType;

        readDomainDataForRank(data_container,
                &data_class,
                id,
                name,
                requestDomain,
                lazyLoad);

        if (dataClass != NULL)
            *dataClass = data_class;

        return data_container;
    }

    void ParallelDomainCollector::readDomainLazy(DomainData *domainData)
    throw (DCException)
    {
        if (domainData == NULL)
        {
            throw DCException(getExceptionString("readDomainLazy",
                    "Invalid parameter, DomainData must not be NULL", NULL));
        }

        DomainH5Ref *loadingRef = domainData->getLoadingReference();
        if (loadingRef == NULL)
        {
            throw DCException(getExceptionString("readDomainLazy",
                    "This DomainData does not allow lazy loading", NULL));
        }

        if (loadingRef->dataClass == UndefinedType)
        {
            throw DCException(getExceptionString("readDomainLazy",
                    "DomainData has invalid data class", NULL));
        }

        if (loadingRef->dataClass == PolyType)
        {
            Dimensions elements_read;
            uint32_t src_rank = 0;
            readDataSet(loadingRef->handle,
                    loadingRef->id,
                    loadingRef->name.c_str(),
                    loadingRef->dstBuffer,
                    loadingRef->dstOffset,
                    loadingRef->srcSize,
                    loadingRef->srcOffset,
                    elements_read,
                    src_rank,
                    domainData->getData());

            if (!(elements_read == loadingRef->dstBuffer))
                throw DCException(getExceptionString("readDomainLazy",
                    "Sizes are not equal but should be (1).", NULL));

        } else
        {

            throw DCException(getExceptionString("readDomainLazy",
                    "data class not supported", NULL));
        }
    }

    void ParallelDomainCollector::writeDomain(int32_t id,
            const CollectionType& type,
            uint32_t ndims,
            const Selection select,
            const char* name,
            const Domain /*localDomain*/,
            const Domain globalDomain,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {
        Dimensions globalSize, globalOffset;
        gatherMPIWrites(ndims, select.count, globalSize, globalOffset);

        writeDomain(id, globalSize, globalOffset,
                type, ndims, select,
                name, globalDomain, dataClass, buf);
    }

    void ParallelDomainCollector::writeDomain(int32_t id,
            const Dimensions globalSize,
            const Dimensions globalOffset,
            const CollectionType& type,
            uint32_t ndims,
            const Selection select,
            const char* name,
            const Domain globalDomain,
            DomDataClass dataClass,
            const void* buf)
    throw (DCException)
    {
        write(id, globalSize, globalOffset, type, ndims, select, name, buf);
        Domain localDomain(Dimensions(0, 0, 0), globalDomain.getSize());

        writeDomainAttributes(id, name, dataClass, localDomain, globalDomain);
    }

    void ParallelDomainCollector::reserveDomain(int32_t id,
            const Dimensions globalSize,
            uint32_t ndims,
            const CollectionType& type,
            const char* name,
            const Domain globalDomain,
            DomDataClass dataClass)
    throw (DCException)
    {
        reserve(id, globalSize, ndims, type, name);
        Domain localDomain(Dimensions(0, 0, 0), globalDomain.getSize());

        writeDomainAttributes(id, name, dataClass, localDomain, globalDomain);
    }

    void ParallelDomainCollector::reserveDomain(int32_t id,
            const Dimensions size,
            Dimensions *globalSize,
            Dimensions *globalOffset,
            uint32_t ndims,
            const CollectionType& type,
            const char* name,
            const Domain globalDomain,
            DomDataClass dataClass)
    throw (DCException)
    {
        reserve(id, size, globalSize, globalOffset, ndims, type, name);
        Domain localDomain(Dimensions(0, 0, 0), globalDomain.getSize());

        writeDomainAttributes(id, name, dataClass, localDomain, globalDomain);
    }

    void ParallelDomainCollector::appendDomain(int32_t id,
            const CollectionType& type,
            size_t count,
            const char *name,
            const Domain localDomain,
            const Domain globalDomain,
            const void *buf)
    throw (DCException)
    {
        appendDomain(id, type, count, 0, 1, name, localDomain, globalDomain, buf);
    }

    void ParallelDomainCollector::appendDomain(int32_t /*id*/,
            const CollectionType& /*type*/,
            size_t /*count*/,
            size_t /*offset*/,
            size_t /*striding*/,
            const char* /*name*/,
            const Domain /*localDomain*/,
            const Domain /*globalDomain*/,
            const void* /*buf*/)
    throw (DCException)
    {
        throw DCException("This feature is not supported in ParallelDomainCollector. "
                "Use ParallelDataCollector::append instead.");
    }

}
