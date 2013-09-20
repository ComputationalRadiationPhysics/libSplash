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

#include "basetypes/ColTypeInt.hpp"

#include "ParallelDomainCollector.hpp"
#include "core/DCParallelDataSet.hpp"

using namespace DCollector;

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

ParallelDomainCollector::ParallelDomainCollector(MPI_Comm comm, MPI_Info info,
        const Dimensions topology, uint32_t maxFileHandles) :
ParallelDataCollector(comm, info, topology, maxFileHandles)
{
}

ParallelDomainCollector::~ParallelDomainCollector()
{
}

size_t ParallelDomainCollector::getTotalElements(int32_t id,
        const char* name)
throw (DCException)
{
    if (this->fileStatus == FST_CLOSED)
        throw DCException(getExceptionString("getTotalElements",
            "this access is not permitted", NULL));

    Dimensions total_elements;
    readAttribute(id, name, DOMCOL_ATTR_ELEMENTS, total_elements.getPointer());

    return total_elements.getDimSize();
}

Domain ParallelDomainCollector::getTotalDomain(int32_t id,
        const char* name)
throw (DCException)
{
    if (this->fileStatus == FST_CLOSED)
        throw DCException(getExceptionString("getTotalDomain",
            "this access is not permitted", NULL));

    Dimensions size(1, 1, 1);
    Dimensions offset(0, 0, 0);

    readAttribute(id, name, DOMCOL_ATTR_SIZE, size.getPointer());
    readAttribute(id, name, DOMCOL_ATTR_START, offset.getPointer());

    if (offset.getDimSize() != 0)
    {
        throw DCException(getExceptionString("getTotalDomain",
                "Invalid offset for total domain (must be (0, 0, 0) )", NULL));
    }

    Domain domain(offset, size);

    return domain;
}

bool ParallelDomainCollector::readDomainDataForRank(
        DataContainer *dataContainer,
        DomDataClass *dataClass,
        int32_t id,
        const char* name,
        Dimensions requestOffset,
        Dimensions requestSize,
        bool lazyLoad)
throw (DCException)
{
    Domain request_domain(requestOffset, requestSize);

    Domain client_domain;
    readAttribute(id, name, DOMCOL_ATTR_START, client_domain.getStart().getPointer());
    readAttribute(id, name, DOMCOL_ATTR_SIZE, client_domain.getSize().getPointer());

    Dimensions data_elements;
    readAttribute(id, name, DOMCOL_ATTR_ELEMENTS, data_elements.getPointer());

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

#if (DC_DEBUG == 1)
    std::cerr << "clientdom. = " << client_domain.toString() << std::endl;
    std::cerr << "requestdom. = " << request_domain.toString() << std::endl;
#endif

    // test on intersection and add new DomainData to the container if necessary
    if (!testIntersection(request_domain, client_domain))
        return false;

    // Poly data has no internal grid structure, 
    // so the whole chunk has to be read and is added to the DataContainer.
    if (*dataClass == PolyType)
    {
#if (DC_DEBUG == 1)
        std::cerr << "dataclass = Poly" << std::endl;
#endif
        if (data_elements.getDimSize() > 0)
        {
            std::stringstream group_id_name;
            group_id_name << SDC_GROUP_DATA << "/" << id;
            std::string group_id_string = group_id_name.str();

            hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
            if (group_id < 0)
                throw DCException(getExceptionString("readDomainDataForRank",
                    "group not found", group_id_string.c_str()));

            size_t datatype_size = 0;
            DCDataSet::DCDataType dc_datatype = DCDataSet::DCDT_UNKNOWN;

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
                readDataSet(handles.get(id), id, name,
                        false,
                        data_elements,
                        Dimensions(0, 0, 0),
                        Dimensions(0, 0, 0),
                        Dimensions(0, 0, 0),
                        elements_read,
                        src_rank,
                        client_data->getData());

#if (DC_DEBUG == 1)
                std::cerr << elements_read.toString() << std::endl;
                std::cerr << data_elements.toString() << std::endl;
#endif

                if (!(elements_read == data_elements))
                    throw DCException(getExceptionString("readDomainDataForRank",
                        "Sizes are not equal but should be (1).", NULL));
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

            hid_t group_id = H5Gopen(handles.get(id), group_id_string.c_str(), H5P_DEFAULT);
            if (group_id < 0)
                throw DCException(getExceptionString("readDomainDataForRank",
                    "group not found", group_id_string.c_str()));

            size_t datatype_size = 0;
            DCDataSet::DCDataType dc_datatype = DCDataSet::DCDT_UNKNOWN;

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
                    request_domain, request_domain.getSize(),
                    datatype_size, dc_datatype);

            dataContainer->add(target_data);
        }

        // Compute the offsets and sizes for reading and
        // writing this intersection.
        Dimensions dst_offset(0, 0, 0);
        Dimensions src_size(1, 1, 1);
        Dimensions src_offset(0, 0, 0);

        Dimensions client_start = client_domain.getStart();
        Dimensions client_size = client_domain.getSize();

        size_t rank = getRank(handles.get(id), id, name);

        for (uint32_t i = 0; i < rank; ++i)
        {
            dst_offset[i] = std::max((int64_t) client_domain.getStart()[i] -
                    (int64_t) requestOffset[i], (int64_t) 0);

            if (requestOffset[i] <= client_start[i])
            {
                src_offset[i] = 0;

                if (requestOffset[i] + requestSize[i] >= client_start[i] + client_size[i])
                    src_size[i] = client_size[i];
                else
                    src_size[i] = requestOffset[i] + requestSize[i] - client_start[i];
            } else
            {
                src_offset[i] = requestOffset[i] - client_start[i];

                if (requestOffset[i] + requestSize[i] >= client_start[i] + client_size[i])
                    src_size[i] = client_size[i] - src_offset[i];
                else
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
        uint32_t src_rank = 0;
        if (src_size.getDimSize() > 0)
        {
            readDataSet(handles.get(id), id, name, false,
                    dataContainer->getIndex(0)->getSize(),
                    dst_offset,
                    src_size,
                    src_offset,
                    elements_read,
                    src_rank,
                    dataContainer->getIndex(0)->getData());
        }

#if (DC_DEBUG == 1)
        std::cerr << "elements_read = " << elements_read.toString() << std::endl;
#endif

        if (!(elements_read == src_size))
            throw DCException(getExceptionString("readDomainDataForRank",
                "Sizes are not equal but should be (2).", NULL));
    }

    return true;
}

DataContainer *ParallelDomainCollector::readDomain(int32_t id,
        const char* name,
        Dimensions requestOffset,
        Dimensions requestSize,
        DomDataClass* dataClass,
        bool lazyLoad)
throw (DCException)
{
    if (fileStatus == FST_CLOSED)
        throw DCException(getExceptionString("readDomain",
            "this access is not permitted", NULL));

    DataContainer * data_container = new DataContainer();

#if (DC_DEBUG == 1)
    std::cerr << "requestOffset = " << requestOffset.toString() << std::endl;
    std::cerr << "requestSize = " << requestSize.toString() << std::endl;
#endif

    DomDataClass data_class = UndefinedType;

    readDomainDataForRank(data_container,
            &data_class,
            id,
            name,
            requestOffset,
            requestSize,
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
                false,
                loadingRef->dstBuffer,
                loadingRef->dstOffset,
                loadingRef->srcSize,
                loadingRef->srcOffset,
                elements_read,
                src_rank,
                domainData->getData());

#if (DC_DEBUG == 1)
        std::cerr << elements_read.toString() << std::endl;
#endif

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
        uint32_t rank,
        const Dimensions srcData,
        const char* name,
        const Dimensions domainOffset,
        const Dimensions domainSize,
        DomDataClass dataClass,
        const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset, globalDomSize, globalDomOffset;
    gatherMPIWrites(rank, srcData, domainSize, domainOffset,
            globalSize, globalOffset, globalDomSize, globalDomOffset);

    writeDomain(id, globalSize, globalOffset,
            type, rank, srcData, Dimensions(1, 1, 1), srcData,
            Dimensions(0, 0, 0), name, globalDomOffset, globalDomSize, dataClass, data);
}

void ParallelDomainCollector::writeDomain(int32_t id,
        const CollectionType& type,
        uint32_t rank,
        const Dimensions srcBuffer,
        const Dimensions srcData,
        const Dimensions srcOffset,
        const char* name,
        const Dimensions domainOffset,
        const Dimensions domainSize,
        DomDataClass dataClass,
        const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset, globalDomSize, globalDomOffset;
    gatherMPIWrites(rank, srcData, domainSize, domainOffset,
            globalSize, globalOffset, globalDomSize, globalDomOffset);

    writeDomain(id, globalSize, globalOffset,
            type, rank, srcBuffer, Dimensions(1, 1, 1), srcData, srcOffset,
            name, globalDomOffset, globalDomSize, dataClass, data);
}

void ParallelDomainCollector::writeDomain(int32_t id,
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
        const void* data)
throw (DCException)
{
    Dimensions globalSize, globalOffset, globalDomSize, globalDomOffset;
    gatherMPIWrites(rank, srcData, domainSize, domainOffset,
            globalSize, globalOffset, globalDomSize, globalDomOffset);

    writeDomain(id, globalSize, globalOffset,
            type, rank, srcBuffer, srcStride, srcData, srcOffset,
            name, globalDomOffset, globalDomSize, dataClass, data);
}

void ParallelDomainCollector::writeDomain(int32_t id,
        const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type,
        uint32_t rank,
        const Dimensions srcData,
        const char* name,
        const Dimensions globalDomainOffset,
        const Dimensions globalDomainSize,
        DomDataClass dataClass,
        const void* data)
throw (DCException)
{
    writeDomain(id, globalSize, globalOffset,
            type, rank, srcData, Dimensions(1, 1, 1), srcData,
            Dimensions(0, 0, 0), name, globalDomainOffset, globalDomainSize,
            dataClass, data);
}

void ParallelDomainCollector::writeDomain(int32_t id,
        const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type,
        uint32_t rank,
        const Dimensions srcBuffer,
        const Dimensions srcData,
        const Dimensions srcOffset,
        const char* name,
        const Dimensions globalDomainOffset,
        const Dimensions globalDomainSize,
        DomDataClass dataClass,
        const void* data)
throw (DCException)
{
    writeDomain(id, globalSize, globalOffset,
            type, rank, srcBuffer, Dimensions(1, 1, 1), srcData, srcOffset,
            name, globalDomainOffset, globalDomainSize, dataClass, data);
}

void ParallelDomainCollector::writeDomain(int32_t id,
        const Dimensions globalSize,
        const Dimensions globalOffset,
        const CollectionType& type,
        uint32_t rank,
        const Dimensions srcBuffer,
        const Dimensions srcStride,
        const Dimensions srcData,
        const Dimensions srcOffset,
        const char* name,
        const Dimensions globalDomainOffset,
        const Dimensions globalDomainSize,
        DomDataClass dataClass,
        const void* data)
throw (DCException)
{
    ColTypeDim dim_t;
    ColTypeInt int_t;

    write(id, globalSize, globalOffset, type, rank, srcBuffer, srcStride,
            srcData, srcOffset, name, data);

    writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &dataClass);
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, globalDomainSize.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_START, globalDomainOffset.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_ELEMENTS, globalSize.getPointer());
}

void ParallelDomainCollector::reserveDomain(int32_t id,
        const Dimensions globalSize,
        uint32_t rank,
        const CollectionType& type,
        const char* name,
        const Dimensions domainOffset,
        const Dimensions domainSize,
        DomDataClass dataClass)
throw (DCException)
{
    ColTypeDim dim_t;
    ColTypeInt int_t;

    reserve(id, globalSize, rank, type, name);

    writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &dataClass);
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, domainSize.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_START, domainOffset.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_ELEMENTS, globalSize.getPointer());
}

void ParallelDomainCollector::reserveDomain(int32_t id,
        const Dimensions size,
        Dimensions *globalSize,
        Dimensions *globalOffset,
        uint32_t rank,
        const CollectionType& type,
        const char* name,
        const Dimensions domainOffset,
        const Dimensions domainSize,
        DomDataClass dataClass)
throw (DCException)
{
    ColTypeDim dim_t;
    ColTypeInt int_t;

    Dimensions globalDomainSize, globalDomainOffset;
    gatherMPIDomains(rank, domainSize, domainOffset, globalDomainSize, globalDomainOffset);

    reserve(id, size, globalSize, globalOffset, rank, type, name);

    writeAttribute(id, int_t, name, DOMCOL_ATTR_CLASS, &dataClass);
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_SIZE, globalDomainSize.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_START, globalDomainOffset.getPointer());
    writeAttribute(id, dim_t, name, DOMCOL_ATTR_ELEMENTS, globalSize->getPointer());
}

void ParallelDomainCollector::appendDomain(int32_t id,
        const CollectionType& type,
        size_t count,
        const char *name,
        const Dimensions domainOffset,
        const Dimensions domainSize,
        const void *data)
throw (DCException)
{
    appendDomain(id, type, count, 0, 1, name, domainOffset, domainSize, data);
}

void ParallelDomainCollector::appendDomain(int32_t /*id*/,
        const CollectionType& /*type*/,
        size_t /*count*/,
        size_t /*offset*/,
        size_t /*striding*/,
        const char* /*name*/,
        const Dimensions /*domainOffset*/,
        const Dimensions /*domainSize*/,
        const void* /*data*/)
throw (DCException)
{
    throw DCException("This feature is not supported in ParallelDomainCollector. "
            "Use ParallelDataCollector::append instead.");
}

void ParallelDomainCollector::gatherMPIDomains(int /*rank*/,
        const Dimensions localDomainSize, const Dimensions /*localDomainOffset*/,
        Dimensions &globalDomainSize, Dimensions &globalDomainOffset)
throw (DCException)
{
    //uint64_t send_offsets[3] = {localDomainOffset[0], localDomainOffset[1], localDomainOffset[2]};
    //uint64_t recv_offsets[3];
    uint64_t send_sizes[3] = {localDomainSize[0], localDomainSize[1], localDomainSize[2]};
    uint64_t recv_sizes[3];

    if ((options.mpiPos[1] != 0) || (options.mpiPos[2] != 0))
        send_sizes[0] = 0;

    if ((options.mpiPos[0] != 0) || (options.mpiPos[2] != 0))
        send_sizes[1] = 0;

    if ((options.mpiPos[0] != 0) || (options.mpiPos[1] != 0))
        send_sizes[2] = 0;

    /*if (MPI_Allreduce(send_offsets, recv_offsets, 3, MPI_INTEGER8, MPI_MIN,
            options.mpiComm) != MPI_SUCCESS)
        throw DCException(getExceptionString("gatherMPIWrites",
            "MPI_Allreduce (1) failed", NULL));*/

    if (MPI_Allreduce(send_sizes, recv_sizes, 3, MPI_INTEGER8, MPI_SUM,
            options.mpiComm) != MPI_SUCCESS)
        throw DCException(getExceptionString("gatherMPIWrites",
            "MPI_Allreduce (2) failed", NULL));

    // every total domain (all combined MPI processes) is required to start at
    // (0, 0, 0)
    globalDomainOffset.set(0, 0, 0);
    globalDomainSize.set(recv_sizes[0], recv_sizes[1], recv_sizes[2]);
}

void ParallelDomainCollector::gatherMPIWrites(int rank, const Dimensions localSize,
        const Dimensions localDomainSize, const Dimensions localDomainOffset,
        Dimensions &globalSize, Dimensions &globalOffset,
        Dimensions &globalDomainSize, Dimensions &globalDomainOffset)
throw (DCException)
{
    ParallelDataCollector::gatherMPIWrites(rank, localSize, globalSize, globalOffset);

    gatherMPIDomains(rank, localDomainSize, localDomainOffset, globalDomainSize, globalDomainOffset);
}
