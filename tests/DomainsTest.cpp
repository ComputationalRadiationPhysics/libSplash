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
 *
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

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "DomainsTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(DomainsTest);

const char* hdf5_file_grid = "h5/testDomainsGrid";
const char* hdf5_file_poly = "h5/testDomainsPoly";
const char* hdf5_file_append = "h5/testDomainsAppend";

using namespace splash;

DomainsTest::DomainsTest() :
ctInt()
{
    dataCollector = new DomainCollector(3);
    srand(10);

    int initialized;
    MPI_Initialized(&initialized);
    if (!initialized)
        MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &totalMpiRank);
}

DomainsTest::~DomainsTest()
{
    if (dataCollector != NULL)
    {
        delete dataCollector;
        dataCollector = NULL;
    }

    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
}

void DomainsTest::subTestGridDomains(const Dimensions mpiSize,
        const Dimensions gridSize,
        uint32_t rank, uint32_t iteration)
{
    int max_rank = 1;
    for (int i = 0; i < rank; i++)
        max_rank *= mpiSize[i];

    int mpi_rank = totalMpiRank;
    Dimensions global_grid_size(0, 0, 0);
    Dimensions global_domain_size(1, 1, 1);
    const Dimensions global_domain_offset(12, 7, 1);

    if (mpi_rank < max_rank)
    {
        // initialize data for writing with mpi index (mpi rank)
        Dimensions mpi_position(mpi_rank % mpiSize[0],
                (mpi_rank / mpiSize[0]) % mpiSize[1],
                (mpi_rank / mpiSize[0]) / mpiSize[1]);

        const Dimensions local_grid_size =
                (mpi_position + Dimensions(1, 1, 1)) * gridSize;
        const Dimensions local_domain_size = local_grid_size;
        Dimensions local_domain_offset(0, 0, 0);

        for (size_t i = 0; i < 3; ++i)
            for (size_t m = 0; m < mpiSize[i]; ++m)
            {
                if (m == mpi_position[i])
                    local_domain_offset[i] = global_grid_size[i];

                global_grid_size[i] += gridSize[i] * (m + 1);
            }

        global_domain_size.set(global_grid_size);

        // write data
        int *data_write = new int[local_grid_size.getScalarSize()];
        for (size_t i = 0; i < local_grid_size.getScalarSize(); ++i)
            data_write[i] = mpi_rank;

        DataCollector::FileCreationAttr fattr;
        fattr.fileAccType = DataCollector::FAT_CREATE;
        fattr.mpiSize.set(mpiSize);
        fattr.mpiPosition.set(mpi_position);

        dataCollector->open(hdf5_file_grid, fattr);

#if defined TESTS_DEBUG
        std::cout << "writing..." << std::endl;
        std::cout << "mpi_position         = " << mpi_position.toString() << std::endl;
        std::cout << "local_domain_offset  = " << local_domain_offset.toString() << std::endl;
        std::cout << "local_domain_size    = " << local_domain_size.toString() << std::endl;
        std::cout << "global_domain_offset = " << global_domain_offset.toString() << std::endl;
        std::cout << "global_domain_size   = " << global_domain_size.toString() << std::endl;
#endif

        dataCollector->writeDomain(iteration, ctInt, rank, Selection(local_grid_size), "grid_data",
                Domain(local_domain_offset, local_domain_size),
                Domain(global_domain_offset, global_domain_size),
                DomainCollector::GridType, data_write);

        dataCollector->close();

        int *data_read = new int[local_grid_size.getScalarSize()];
        fattr.fileAccType = DataCollector::FAT_READ;

        dataCollector->open(hdf5_file_grid, fattr);
        DomainCollector::DomDataClass data_class = DomainCollector::UndefinedType;
        DataContainer *container = dataCollector->readDomain(iteration, "grid_data",
                Domain(global_domain_offset + local_domain_offset, local_grid_size),
                &data_class, false);
        dataCollector->close();

        CPPUNIT_ASSERT(container != NULL);
        CPPUNIT_ASSERT(container->getNumSubdomains() == 1);
        CPPUNIT_ASSERT(data_class == DomainCollector::GridType);
        CPPUNIT_ASSERT(container->getNumElements() == local_grid_size.getScalarSize());

        for (size_t i = 0; i < local_grid_size.getScalarSize(); ++i)
            CPPUNIT_ASSERT(*((int*) (container->getElement(i))) == mpi_rank);

        delete container;

        delete[] data_read;

        delete[] data_write;
        data_write = NULL;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (mpi_rank == 0)
    {
        // now read and test domain subdomains
        DataCollector::FileCreationAttr fattr;
        fattr.fileAccType = DataCollector::FAT_READ_MERGED;
        fattr.mpiSize = mpiSize;
        dataCollector->open(hdf5_file_grid, fattr);

#if defined TESTS_DEBUG
        std::cout << "global_grid_size = " << global_grid_size.toString() << std::endl;
#endif

        // test different domain offsets
        for (uint32_t q = 0; q < 5; ++q)
        {
            Dimensions offset_mpi(rand() % mpiSize[0],
                    rand() % mpiSize[1],
                    rand() % mpiSize[2]);

            Dimensions offset(0, 0, 0);
            for (size_t d = 0; d < 3; ++d)
                for (size_t m = 0; m < offset_mpi[d]; ++m)
                {
                    if (m <= offset_mpi[d])
                        offset[d] += gridSize[d] * (m + 1);
                }

            Dimensions partition_size = global_domain_size - offset;

#if defined TESTS_DEBUG
            std::cout << "mpi_size = " << mpiSize.toString() << std::endl;
            std::cout << "offset_mpi = " << offset_mpi.toString() << std::endl;
            std::cout << "offset = " << offset.toString() << std::endl;
            std::cout << "partition_size = " << partition_size.toString() << std::endl;
#endif

            DomainCollector::DomDataClass data_class = DomainCollector::UndefinedType;

            Domain global_domain;
            global_domain = dataCollector->getGlobalDomain(iteration, "grid_data");
            CPPUNIT_ASSERT(global_domain.getOffset() == global_domain_offset);
            CPPUNIT_ASSERT(global_domain.getSize() == global_domain_size);

            // read data container
            DataContainer *container = dataCollector->readDomain(iteration, "grid_data",
                    Domain(offset + global_domain_offset, partition_size), &data_class);

#if defined TESTS_DEBUG
            std::cout << "container->getNumSubdomains() = " << container->getNumSubdomains() << std::endl;
#endif

            // check the container
            CPPUNIT_ASSERT(container != NULL);
            CPPUNIT_ASSERT(data_class == DomainCollector::GridType);
            CPPUNIT_ASSERT(container->getNumSubdomains() == 1);

            // check all DomainData entries in the container

            DomainData *subdomain = container->getIndex(0);
            CPPUNIT_ASSERT(subdomain != NULL);
            CPPUNIT_ASSERT(subdomain->getData() != NULL);

            Dimensions subdomain_elements = subdomain->getElements();
            Dimensions subdomain_offset = subdomain->getOffset();

#if defined TESTS_DEBUG
            std::cout << "subdomain->getOffset() = " << subdomain->getOffset().toString() << std::endl;
            std::cout << "subdomain->getElements() = " << subdomain_elements.toString() << std::endl;
#endif

            int *subdomain_data = (int*) (subdomain->getData());
            CPPUNIT_ASSERT(subdomain_elements == global_grid_size - offset);

            Dimensions mpi_offset(offset);
            Dimensions current_grid_size(0, 0, 0);
            for (int z = offset_mpi[2]; z < mpiSize[2]; ++z)
            {
                mpi_offset[1] = offset[1];
                for (int y = offset_mpi[1]; y < mpiSize[1]; ++y)
                {
                    mpi_offset[0] = offset[0];
                    for (int x = offset_mpi[0]; x < mpiSize[0]; ++x)
                    {
                        int expected_value = z * mpiSize[1] * mpiSize[0] + y * mpiSize[0] + x;
                        Dimensions current_mpi_pos(x, y, z);

                        current_grid_size = (current_mpi_pos + Dimensions(1, 1, 1)) * gridSize;

#if defined TESTS_DEBUG
                        std::cout << "mpi_pos = " << current_mpi_pos.toString() << ", " <<
                                "mpi_offset = " << mpi_offset.toString() << ", " <<
                                "current_grid_size = " << current_grid_size.toString() << std::endl;
#endif

                        for (int k = 0; k < current_grid_size[2]; ++k)
                            for (int j = 0; j < current_grid_size[1]; ++j)
                                for (int i = 0; i < current_grid_size[0]; ++i)
                                {
                                    Dimensions total_grid_position = mpi_offset +
                                            Dimensions(i, j, k) - offset;

                                    size_t index =
                                            total_grid_position[2] * partition_size[1] * partition_size[0] +
                                            total_grid_position[1] * partition_size[0] +
                                            total_grid_position[0];

#if defined TESTS_DEBUG
                                    std::cout << "tgp = " << total_grid_position.toString() <<
                                            ", index = " << index << ", subdomain_data[index] = " << subdomain_data[index] <<
                                            ", expected_value = " << expected_value << std::endl;
#endif

                                    CPPUNIT_ASSERT(subdomain_data[index] == expected_value);
                                }

                        mpi_offset[0] += current_grid_size[0];
                    }

                    mpi_offset[1] += current_grid_size[1];
                }

                mpi_offset[2] += current_grid_size[2];
            }

            delete container;
            container = NULL;
        }

        dataCollector->close();
    }
}

void DomainsTest::testGridDomains()
{
    uint32_t iteration = 0;
    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);

#if defined TESTS_DEBUG
                if (totalMpiRank == 0)
                {
                    std::cout << "** testing mpi_size = " <<
                            mpi_size.toString() << std::endl;
                }
#endif

                for (uint32_t k = 1; k < 8; k++)
                    for (uint32_t j = 5; j < 8; j++)
                        for (uint32_t i = 5; i < 8; i++)
                        {
                            Dimensions grid_size(i, j, k);

                            if (totalMpiRank == 0)
                            {
#if defined TESTS_DEBUG
                                std::cout << "** testing grid_size = " <<
                                        grid_size.toString() << " with mpi size " <<
                                        mpi_size.toString() << std::endl;
#else
                                std::cout << "." << std::flush;
#endif
                            }

                            MPI_Barrier(MPI_COMM_WORLD);

                            subTestGridDomains(mpi_size, grid_size, 3, iteration);

                            MPI_Barrier(MPI_COMM_WORLD);
                            iteration++;
                        }
            }

    // use this for testing a single specific configuration
    /*Dimensions mpi_size(2, 1, 1);
    Dimensions grid_size(5, 5, 1);
    subTestGridDomains(mpi_size, grid_size, 3, 0);*/

}

void DomainsTest::subTestPolyDomains(const Dimensions mpiSize, uint32_t numElements,
        uint32_t iteration)
{
    Dimensions local_grid_size(20, 10, 5);
    Dimensions global_grid_size = local_grid_size * mpiSize;

    const Dimensions global_domain_offset(15, 0, 7);
        const Dimensions global_domain_size = mpiSize * local_grid_size;

    int max_rank = 1;
    for (int i = 0; i < 3; i++)
        max_rank *= mpiSize[i];


    int mpi_rank = totalMpiRank;
    size_t mpi_elements = numElements * (mpi_rank + 1);

    if (mpi_rank < max_rank)
    {
        // write data
        float *data_write = new float[mpi_elements];

        // initialize data for writing with mpi index (mpi rank)
        Dimensions mpi_position(mpi_rank % mpiSize[0],
                (mpi_rank / mpiSize[0]) % mpiSize[1],
                (mpi_rank / mpiSize[0]) / mpiSize[1]);

        for (size_t i = 0; i < mpi_elements; ++i)
            data_write[i] = (float) mpi_rank;

        DataCollector::FileCreationAttr fattr;
        fattr.fileAccType = DataCollector::FAT_CREATE;
        fattr.mpiSize.set(mpiSize);
        fattr.mpiPosition.set(mpi_position);

        dataCollector->open(hdf5_file_poly, fattr);

        Dimensions local_domain_offset = mpi_position * local_grid_size;
        Dimensions local_domain_size = local_grid_size;

#if defined TESTS_DEBUG
        std::cout << "writing..." << std::endl;
        std::cout << "mpi_position = " << mpi_position.toString() << std::endl;
        std::cout << "local_domain_offset = " << local_domain_offset.toString() << std::endl;
        std::cout << "local_domain_size = " << local_domain_size.toString() << std::endl;
        std::cout << "global_domain_offset = " << global_domain_offset.toString() << std::endl;
        std::cout << "global_domain_size = " << global_domain_size.toString() << std::endl;
#endif

        dataCollector->writeDomain(iteration, ctFloat, 1, Dimensions(mpi_elements, 1, 1),
                "poly_data",
                Domain(local_domain_offset, local_domain_size),
                Domain(global_domain_offset, global_domain_size),
                DomainCollector::PolyType, data_write);

        dataCollector->close();

        delete[] data_write;
        data_write = NULL;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (mpi_rank == 0)
    {
        // now read and test domain subdomains
        DataCollector::FileCreationAttr fattr;
        fattr.fileAccType = DataCollector::FAT_READ_MERGED;
        fattr.mpiSize = mpiSize;
        dataCollector->open(hdf5_file_poly, fattr);

        size_t global_num_elements = 0;
        for (int i = 0; i < mpiSize.getScalarSize(); ++i)
            global_num_elements += numElements * (i + 1);

#if defined TESTS_DEBUG
        std::cout << "numElements         = " << numElements << std::endl;
        std::cout << "global_num_elements = " << global_num_elements << std::endl;
#endif

        // test different domain offsets
        for (uint32_t i = 0; i < 5; ++i)
        {
            Dimensions offset(rand() % global_grid_size[0],
                    rand() % global_grid_size[1],
                    rand() % global_grid_size[2]);

            Dimensions partition_size = global_grid_size - offset;

#if defined TESTS_DEBUG
            std::cout << "offset         = " << offset.toString() << std::endl;
            std::cout << "partition_size = " << partition_size.toString() << std::endl;
#endif

            IDomainCollector::DomDataClass data_class = IDomainCollector::UndefinedType;

            // read data container
            DataContainer *container = dataCollector->readDomain(iteration, "poly_data",
                    Domain(global_domain_offset + offset, partition_size), &data_class);

#if defined TESTS_DEBUG
            std::cout << "container->getNumSubdomains() = " << container->getNumSubdomains() << std::endl;
#endif

            // check the container
            CPPUNIT_ASSERT(data_class == IDomainCollector::PolyType);
            CPPUNIT_ASSERT(container->getNumSubdomains() >= 1);

            // check all DomainData entries in the container
            for (int i = 0; i < container->getNumSubdomains(); ++i)
            {
                DomainData *subdomain = container->getIndex(i);
                CPPUNIT_ASSERT(subdomain != NULL);
                CPPUNIT_ASSERT(subdomain->getData() != NULL);

                Dimensions subdomain_elements = subdomain->getElements();
                Dimensions subdomain_offset = subdomain->getOffset();

                // Find out the expected value (original mpi rank)
                // for this subdomain.
                Dimensions subdomain_mpi_pos = (subdomain_offset - global_domain_offset) / local_grid_size;
                int subdomain_mpi_rank = subdomain_mpi_pos[2] * mpiSize[1] * mpiSize[0] +
                        subdomain_mpi_pos[1] * mpiSize[0] + subdomain_mpi_pos[0];

#if defined TESTS_DEBUG
                std::cout << "subdomain->getElements() = " << subdomain->getElements().toString() << std::endl;
                std::cout << "subdomain->getSize()     = " << subdomain->getSize().toString() << std::endl;
                std::cout << "subdomain_mpi_rank       = " << subdomain_mpi_rank << std::endl;
#endif

                float *subdomain_data = (float*) (subdomain->getData());
                CPPUNIT_ASSERT(subdomain_elements.getScalarSize() != 0);

                CPPUNIT_ASSERT(subdomain_elements.getScalarSize() == numElements * (subdomain_mpi_rank + 1));

                for (int j = 0; j < subdomain_elements.getScalarSize(); ++j)
                {
#if defined TESTS_DEBUG
                    std::cout << "j = " << j << ", subdomain_data[j]) = " << subdomain_data[j] <<
                            ", subdomain_mpi_rank = " << subdomain_mpi_rank << std::endl;
#endif
                    CPPUNIT_ASSERT(subdomain_data[j] == (float) subdomain_mpi_rank);
                }
            }

            delete container;
            container = NULL;
        }

        dataCollector->close();
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

void DomainsTest::testPolyDomains()
{
    uint32_t iteration = 0;
    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);

#if defined TESTS_DEBUG
                if (totalMpiRank == 0)
                {
                    std::cout << "** testing mpi_size = " <<
                            mpi_size.toString() << std::endl;
                }
#endif

                for (uint32_t elements = 1; elements <= 100; elements *= 100)
                {
                    if (totalMpiRank == 0)
                    {
#if defined TESTS_DEBUG
                        std::cout << "** testing elements = " <<
                                elements << std::endl;
#else
                        std::cout << "." << std::flush;
#endif
                    }

                    MPI_Barrier(MPI_COMM_WORLD);

                    subTestPolyDomains(mpi_size, elements, iteration);
                    iteration++;
                }
            }

    // use this for testing a single specific configuration
    /*Dimensions mpi_size(2, 1, 1);
    subTestPolyDomains(mpi_size, 10, 0);*/

}

void DomainsTest::testAppendDomains()
{
    int mpi_rank = totalMpiRank;


    if (mpi_rank == 0)
    {
        Dimensions mpi_size(1, 1, 1);
        Dimensions mpi_position(0, 0, 0);
        Dimensions grid_size(12, 40, 7);
        uint32_t elements = 100;

        const Dimensions globalDomainOffset(0, 0, 0);
        const Dimensions globalDomainSize(grid_size);
        const Dimensions domainOffset(globalDomainOffset);

        DataCollector::FileCreationAttr fattr;
        fattr.fileAccType = DataCollector::FAT_CREATE;
        fattr.mpiSize.set(mpi_size);
        fattr.mpiPosition.set(mpi_position);

        dataCollector->open(hdf5_file_append, fattr);

#if defined TESTS_DEBUG
        std::cout << "writing..." << std::endl;
        std::cout << "mpi_position = " << mpi_position.toString() << std::endl;
#endif

        float *data_write = new float[elements];

        for (size_t i = 0; i < elements; ++i)
            data_write[i] = (float) i;

        dataCollector->appendDomain(0, ctFloat, 10, 0, 1, "append_data",
                Domain(domainOffset, grid_size),
                Domain(globalDomainOffset, globalDomainSize), data_write);

        dataCollector->appendDomain(0, ctFloat, elements - 10, 10, 1, "append_data",
                Domain(domainOffset, grid_size),
                Domain(globalDomainOffset, globalDomainSize), data_write);

        dataCollector->close();

        delete[] data_write;
        data_write = NULL;

        // now read and test domain subdomain
        fattr.fileAccType = DataCollector::FAT_READ_MERGED;
        fattr.mpiSize.set(mpi_size);
        fattr.mpiPosition.set(mpi_position);
        dataCollector->open(hdf5_file_append, fattr);

        // read data container
        IDomainCollector::DomDataClass data_class = IDomainCollector::UndefinedType;
        DataContainer *container = dataCollector->readDomain(0, "append_data",
                Domain(Dimensions(0, 0, 0), grid_size), &data_class);

#if defined TESTS_DEBUG
        std::cout << "container->getNumSubdomains() = " << container->getNumSubdomains() << std::endl;
#endif

        // check the container
        CPPUNIT_ASSERT(data_class == IDomainCollector::PolyType);
        CPPUNIT_ASSERT(container->getNumSubdomains() == 1);

        DomainData *subdomain = container->getIndex(0);
        CPPUNIT_ASSERT(subdomain != NULL);
        CPPUNIT_ASSERT(subdomain->getData() != NULL);

        Dimensions subdomain_elements = subdomain->getElements();

#if defined TESTS_DEBUG
        std::cout << "subdomain->getElements() = " << subdomain->getElements().toString() << std::endl;
        std::cout << "subdomain->getSize() = " << subdomain->getSize().toString() << std::endl;
#endif

        float *subdomain_data = (float*) (subdomain->getData());
        CPPUNIT_ASSERT(subdomain_elements.getScalarSize() == elements && subdomain_elements[0] == elements);

        for (int j = 0; j < elements; ++j)
        {
            CPPUNIT_ASSERT(subdomain_data[j] == (float) j);
        }

        delete container;
        container = NULL;

        dataCollector->close();
    }

    MPI_Barrier(MPI_COMM_WORLD);
}
