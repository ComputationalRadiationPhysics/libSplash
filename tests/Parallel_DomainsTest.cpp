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



#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "Parallel_DomainsTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_DomainsTest);

const char* hdf5_file_grid = "h5/testDomainsGridParallel";
//const char* hdf5_file_poly = "h5/testDomainsPolyParallel";

#define MPI_CHECK(cmd) \
        { \
                int mpi_result = cmd; \
                if (mpi_result != MPI_SUCCESS) {\
                    std::cout << "MPI command " << #cmd << " failed!" << std::endl; \
                        throw DCException("MPI error"); \
                } \
        }

using namespace DCollector;

Parallel_DomainsTest::Parallel_DomainsTest() :
ctInt(),
parallelDomainCollector(NULL)
{
    srand(10);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
}

Parallel_DomainsTest::~Parallel_DomainsTest()
{
}

static void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
{
    mpiPos[2] = index / (mpiSize[0] * mpiSize[1]);
    mpiPos[1] = (index % (mpiSize[0] * mpiSize[1])) / mpiSize[0];
    mpiPos[0] = index % mpiSize[0];
}

void Parallel_DomainsTest::subTestGridDomains(int32_t iteration,
        int currentMpiRank,
        const Dimensions mpiSize, const Dimensions mpiPos,
        const Dimensions gridSize, uint32_t dimensions, MPI_Comm mpiComm)
{
    DataCollector::FileCreationAttr fattr;

    // write data to file
    int *data_write = new int[gridSize.getDimSize()];

    for (size_t i = 0; i < gridSize.getDimSize(); ++i)
        data_write[i] = currentMpiRank + 1;

    DataCollector::initFileCreationAttr(fattr);
    fattr.fileAccType = DataCollector::FAT_CREATE;
    fattr.enableCompression = true;
    parallelDomainCollector->open(hdf5_file_grid, fattr);

    Dimensions domain_offset = mpiPos * gridSize;

#if defined TESTS_DEBUG
    std::cout << "writing..." << std::endl;
    std::cout << "domain_offset = " << domain_offset.toString() << std::endl;
#endif

    // initial part of the test: data is written to the file
    parallelDomainCollector->writeDomain(iteration, ctInt, 3, gridSize, "grid_data",
            domain_offset, gridSize, IDomainCollector::GridType, data_write);
    parallelDomainCollector->close();

    delete[] data_write;
    data_write = NULL;

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test written data using all processes
    fattr.fileAccType = DataCollector::FAT_READ;
    Dimensions full_grid_size = gridSize * mpiSize;

    // now read again
    parallelDomainCollector->open(hdf5_file_grid, fattr);

    size_t global_domain_elements = parallelDomainCollector->getTotalElements(
            iteration, "grid_data");

#if defined TESTS_DEBUG
    std::cout << "global_domain_elements = " << global_domain_elements << std::endl;
    std::cout << "full_grid_size = " << full_grid_size.toString() << std::endl;
#endif

    CPPUNIT_ASSERT(global_domain_elements == full_grid_size.getDimSize());

    // test different domain offsets
    for (uint32_t i = 0; i < 5; ++i)
    {
        Dimensions offset(rand() % full_grid_size[0],
                rand() % full_grid_size[1],
                rand() % full_grid_size[2]);

        Dimensions partition_size = full_grid_size - offset;

#if defined TESTS_DEBUG
        std::cout << "offset = " << offset.toString() << std::endl;
        std::cout << "partition_size = " << partition_size.toString() << std::endl;
#endif

        IDomainCollector::DomDataClass data_class = IDomainCollector::UndefinedType;

        Domain total_domain;
        total_domain = parallelDomainCollector->getTotalDomain(iteration, "grid_data");
        CPPUNIT_ASSERT(total_domain.getStart() == Dimensions(0, 0, 0));
        CPPUNIT_ASSERT(total_domain.getSize() == full_grid_size);

        // read data container
        DataContainer *container = parallelDomainCollector->readDomain(
                iteration, "grid_data",
                offset, partition_size, &data_class);

#if defined TESTS_DEBUG
        std::cout << "container->getNumSubdomains() = " << container->getNumSubdomains() << std::endl;
#endif

        // check the container
        CPPUNIT_ASSERT(data_class == IDomainCollector::GridType);
        CPPUNIT_ASSERT(container->getNumSubdomains() == 1);

        // check all DomainData entries in the container

        DomainData *subdomain = container->getIndex(0);
        CPPUNIT_ASSERT(subdomain != NULL);
        CPPUNIT_ASSERT(subdomain->getData() != NULL);

        Dimensions subdomain_elements = subdomain->getElements();
        Dimensions subdomain_start = subdomain->getStart();

#if defined TESTS_DEBUG
        std::cout << "subdomain->getStart() = " << subdomain->getStart().toString() << std::endl;
        std::cout << "subdomain->getElements() = " << subdomain_elements.toString() << std::endl;
#endif

        int *subdomain_data = (int*) (subdomain->getData());
        CPPUNIT_ASSERT(subdomain_elements.getDimSize() != 0);
        CPPUNIT_ASSERT(gridSize.getDimSize() != 0);

        for (int j = 0; j < subdomain_elements.getDimSize(); ++j)
        {
            // Find out the expected value (original mpi rank) 
            // for exactly this data element.
            Dimensions j_grid_position(j % subdomain_elements[0],
                    (j / subdomain_elements[0]) % subdomain_elements[1],
                    (j / subdomain_elements[0]) / subdomain_elements[1]);

            Dimensions total_grid_position =
                    (subdomain_start + j_grid_position) / gridSize;

            int expected_value = total_grid_position[2] * mpiSize[0] * mpiSize[1] +
                    total_grid_position[1] * mpiSize[0] + total_grid_position[0] + 1;

#if defined TESTS_DEBUG
            std::cout << "j = " << j << ", subdomain_data[j] = " << subdomain_data[j] <<
                    ", expected_value = " << expected_value << std::endl;
#endif
            CPPUNIT_ASSERT(subdomain_data[j] == expected_value);
        }

        delete container;
        container = NULL;
        
        MPI_CHECK(MPI_Barrier(mpiComm));
    }
    
    parallelDomainCollector->close();

    MPI_CHECK(MPI_Barrier(mpiComm));
}

void Parallel_DomainsTest::testGridDomains()
{
    const uint32_t dimensions = 3;
    int32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getDimSize();

                int ranks[num_ranks];
                for (uint32_t i = 0; i < num_ranks; ++i)
                    ranks[i] = i;

                MPI_Group world_group, comm_group;
                MPI_Comm mpi_current_comm;
                MPI_CHECK(MPI_Comm_group(MPI_COMM_WORLD, &world_group));
                MPI_CHECK(MPI_Group_incl(world_group, num_ranks, ranks, &comm_group));
                MPI_CHECK(MPI_Comm_create(MPI_COMM_WORLD, comm_group, &mpi_current_comm));

                // filter mpi ranks
                if (myMpiRank < num_ranks)
                {
                    int my_current_mpi_rank;
                    MPI_Comm_rank(mpi_current_comm, &my_current_mpi_rank);

                    Dimensions current_mpi_pos;
                    indexToPos(my_current_mpi_rank, mpi_size, current_mpi_pos);

#if defined TESTS_DEBUG
                    if (my_current_mpi_rank == 0)
                    {
                        std::cout << std::endl << "** testing mpi size " <<
                                mpi_size.toString() << std::endl;
                    }
#endif

                    parallelDomainCollector = new ParallelDomainCollector(mpi_current_comm,
                            MPI_INFO_NULL, mpi_size, my_current_mpi_rank, 10);

                    for (uint32_t k = 1; k < 8; k++)
                        for (uint32_t j = 5; j < 8; j++)
                            for (uint32_t i = 5; i < 8; i++)
                            {
                                Dimensions grid_size(i, j, k);

                                if (my_current_mpi_rank == 0)
                                {
#if defined TESTS_DEBUG
                                    std::cout << std::endl << "** testing grid_size = " <<
                                            grid_size.toString() << " with mpi size " <<
                                            mpi_size.toString() << std::endl;
#else
                                    std::cout << "." << std::flush;
#endif
                                }

                                MPI_CHECK(MPI_Barrier(mpi_current_comm));

                                subTestGridDomains(iteration,
                                        my_current_mpi_rank,
                                        mpi_size,
                                        current_mpi_pos,
                                        grid_size,
                                        dimensions,
                                        mpi_current_comm);

                                MPI_CHECK(MPI_Barrier(mpi_current_comm));

                                iteration++;
                            }

                    delete parallelDomainCollector;
                    parallelDomainCollector = NULL;

                    MPI_Comm_free(&mpi_current_comm);

                } else
                    iteration += (8 - 1) * (8 - 5) * (8 - 5);

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

}

