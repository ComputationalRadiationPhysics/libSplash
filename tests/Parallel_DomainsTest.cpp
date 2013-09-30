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


#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "Parallel_DomainsTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Parallel_DomainsTest);

const char* hdf5_file_grid = "h5/testDomainsGridParallel";
const char* hdf5_file_poly = "h5/testDomainsPolyParallel";
const char* hdf5_file_append = "h5/testDomainsAppendParallel";

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

    int argc;
    char** argv;
    int initialized;
    MPI_Initialized(&initialized);
    if (!initialized)
        MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &totalMpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myMpiRank);
}

Parallel_DomainsTest::~Parallel_DomainsTest()
{
    int finalized;
    MPI_Finalized(&finalized);
    if (!finalized)
        MPI_Finalize();
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
    int *data_write = new int[gridSize.getScalarSize()];

    for (size_t i = 0; i < gridSize.getScalarSize(); ++i)
        data_write[i] = currentMpiRank + 1;

    const Dimensions domain_size = gridSize;
    const Dimensions global_domain_offset(17, 32, 5);
    const Dimensions domain_offset = (mpiPos * gridSize) + global_domain_offset;
    const Dimensions global_domain_size = domain_size * mpiSize;
    const Dimensions full_grid_size = gridSize * mpiSize;

    DataCollector::initFileCreationAttr(fattr);
    fattr.fileAccType = DataCollector::FAT_CREATE;
    fattr.enableCompression = true;
    parallelDomainCollector->open(hdf5_file_grid, fattr);



#if defined TESTS_DEBUG
    std::cout << "writing..." << std::endl;
    std::cout << "domain_offset = " << domain_offset.toString() << std::endl;
#endif

    // initial part of the test: data is written to the file
    parallelDomainCollector->writeDomain(iteration, ctInt, 3, gridSize, "grid_data",
            domain_offset, domain_size,
            global_domain_offset, global_domain_size,
            IDomainCollector::GridType, data_write);
    parallelDomainCollector->close();

    delete[] data_write;
    data_write = NULL;

    MPI_CHECK(MPI_Barrier(mpiComm));

    // test written data using all processes
    fattr.fileAccType = DataCollector::FAT_READ;


    // now read again
    parallelDomainCollector->open(hdf5_file_grid, fattr);

#if defined TESTS_DEBUG
    std::cout << "full_grid_size = " << full_grid_size.toString() << std::endl;
#endif

    // test different domain offsets
    for (uint32_t i = 0; i < 5; ++i)
    {
        Dimensions offset(rand() % global_domain_size[0],
                rand() % global_domain_size[1],
                rand() % global_domain_size[2]);

        Dimensions partition_size = global_domain_size - offset;

        offset = offset + global_domain_offset;

#if defined TESTS_DEBUG
        std::cout << "offset = " << offset.toString() << std::endl;
        std::cout << "partition_size = " << partition_size.toString() << std::endl;
#endif

        IDomainCollector::DomDataClass data_class = IDomainCollector::UndefinedType;

        Domain total_domain;
        total_domain = parallelDomainCollector->getGlobalDomain(iteration, "grid_data");
        CPPUNIT_ASSERT(total_domain.getOffset() == global_domain_offset);
        CPPUNIT_ASSERT(total_domain.getSize() == global_domain_size);

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
        Dimensions subdomain_offset = subdomain->getOffset();

#if defined TESTS_DEBUG
        std::cout << "subdomain->getOffset() = " << subdomain->getOffset().toString() << std::endl;
        std::cout << "subdomain->getElements() = " << subdomain_elements.toString() << std::endl;
#endif

        int *subdomain_data = (int*) (subdomain->getData());
        CPPUNIT_ASSERT(subdomain_elements.getScalarSize() != 0);
        CPPUNIT_ASSERT(gridSize.getScalarSize() != 0);

        for (int j = 0; j < subdomain_elements.getScalarSize(); ++j)
        {
            // Find out the expected value (original mpi rank) 
            // for exactly this data element.
            Dimensions j_grid_position(j % subdomain_elements[0],
                    (j / subdomain_elements[0]) % subdomain_elements[1],
                    (j / subdomain_elements[0]) / subdomain_elements[1]);

            Dimensions total_grid_position =
                    (subdomain_offset - global_domain_offset + j_grid_position) / gridSize;

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

#define MIN_K 1
#define MAX_K 8
#define MIN_J 6
#define MAX_J 9
#define MIN_I 5
#define MAX_I 8

void Parallel_DomainsTest::testGridDomains()
{
    const uint32_t dimensions = 3;
    int32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getScalarSize();

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
                            MPI_INFO_NULL, mpi_size, 10);

                    for (uint32_t k = MIN_K; k < MAX_K; k++)
                        for (uint32_t j = MIN_J; j < MAX_J; j++)
                            for (uint32_t i = MIN_I; i < MAX_I; i++)
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
                    iteration += (MAX_K - MIN_K) * (MAX_J - MIN_J) * (MAX_I - MIN_I);

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

}

void Parallel_DomainsTest::subTestPolyDomains(int32_t iteration,
        int currentMpiRank,
        const Dimensions mpiSize, const Dimensions mpiPos,
        const uint32_t numElements, uint32_t dimensions, MPI_Comm mpiComm)
{
    Dimensions domain_size(20, 10, 5);
    Dimensions global_domain_size = domain_size * mpiSize;

    const Dimensions global_domain_offset(2, 4, 8);

    size_t mpi_elements = numElements * (currentMpiRank + 1);
    Dimensions poly_size(mpi_elements, 1, 1);

    // write data
    float *data_write = new float[mpi_elements];
    for (size_t i = 0; i < mpi_elements; ++i)
        data_write[i] = (float) (currentMpiRank + 1);

    DataCollector::FileCreationAttr fattr;
    fattr.fileAccType = DataCollector::FAT_CREATE;

    parallelDomainCollector->open(hdf5_file_poly, fattr);

    Dimensions domain_offset = mpiPos * domain_size + global_domain_offset;

#if defined TESTS_DEBUG
    std::cout << "[" << currentMpiRank << "] writing..." << std::endl;
    std::cout << "[" << currentMpiRank << "] mpi_position = " << mpiPos.toString() << std::endl;
    std::cout << "[" << currentMpiRank << "] domain_offset = " << domain_offset.toString() << std::endl;
    std::cout << "[" << currentMpiRank << "] poly_size = " << poly_size.toString() << std::endl;
#endif

    parallelDomainCollector->writeDomain(iteration, ctFloat, 1, poly_size,
            "poly_data", domain_offset, domain_size,
            global_domain_offset, global_domain_size,
            IDomainCollector::PolyType, data_write);

    parallelDomainCollector->close();

    delete[] data_write;
    data_write = NULL;

    MPI_Barrier(mpiComm);

    // now read and test domain subdomains
    fattr.fileAccType = DataCollector::FAT_READ;
    parallelDomainCollector->open(hdf5_file_poly, fattr);

    Domain total_domain = parallelDomainCollector->getGlobalDomain(iteration, "poly_data");
    CPPUNIT_ASSERT(total_domain.getSize() == global_domain_size);
    CPPUNIT_ASSERT(total_domain.getOffset() == global_domain_offset);

    size_t global_num_elements = 0;
    for (int i = 0; i < mpiSize.getScalarSize(); ++i)
        global_num_elements += numElements * (i + 1);

#if defined TESTS_DEBUG
    std::cout << "[" << currentMpiRank << "] global_num_elements = " << global_num_elements << std::endl;
#endif

    // test different domain offsets
    for (uint32_t i = 0; i < 5; ++i)
    {
        Dimensions offset(rand() % global_domain_size[0],
                rand() % global_domain_size[1],
                rand() % global_domain_size[2]);

        Dimensions partition_size = global_domain_size - offset;

        offset = offset + global_domain_offset;

#if defined TESTS_DEBUG
        std::cout << "offset = " << offset.toString() << std::endl;
        std::cout << "partition_size = " << partition_size.toString() << std::endl;
#endif

        // read data container, returns a single subdomain containing all data
        IDomainCollector::DomDataClass data_class = IDomainCollector::UndefinedType;
        DataContainer *container = parallelDomainCollector->readDomain(iteration,
                "poly_data", offset, partition_size, &data_class);

#if defined TESTS_DEBUG
        std::cout << "container->getNumSubdomains() = " << container->getNumSubdomains() << std::endl;
#endif

        // check the container
        CPPUNIT_ASSERT(data_class == IDomainCollector::PolyType);
        CPPUNIT_ASSERT(container->getNumSubdomains() == 1);

        // check all DomainData entries in the container
        DomainData *subdomain = container->getIndex(0);
        CPPUNIT_ASSERT(subdomain != NULL);
        CPPUNIT_ASSERT(subdomain->getData() != NULL);

#if defined TESTS_DEBUG
        std::cout << "[" << currentMpiRank << "] subdomain->getElements() = " << subdomain->getElements().toString() << std::endl;
        std::cout << "[" << currentMpiRank << "] subdomain->getSize() = " << subdomain->getSize().toString() << std::endl;
#endif

        float *subdomain_data = (float*) (subdomain->getData());
        Dimensions subdomain_elements = subdomain->getElements();
        CPPUNIT_ASSERT(subdomain_elements.getScalarSize() == global_num_elements);

        size_t elements_this_rank = numElements;
        size_t this_rank = 0;
        for (int j = 0; j < subdomain_elements.getScalarSize(); ++j)
        {
            if (j == elements_this_rank)
            {
                this_rank++;
                elements_this_rank += numElements * (this_rank + 1);
            }

#if defined TESTS_DEBUG
            std::cout << "[" << currentMpiRank << "] j = " << j << ", subdomain_data[j]) = " << subdomain_data[j] <<
                    ", subdomain_mpi_rank = " << (float) (this_rank + 1) << std::endl;
#endif
            CPPUNIT_ASSERT(subdomain_data[j] == (float) (this_rank + 1));
        }

        delete container;
        container = NULL;

        MPI_Barrier(mpiComm);
    }

    parallelDomainCollector->close();

    MPI_Barrier(mpiComm);
}

void Parallel_DomainsTest::testPolyDomains()
{
    const uint32_t dimensions = 3;
    int32_t iteration = 0;

    for (uint32_t mpi_z = 1; mpi_z < 3; ++mpi_z)
        for (uint32_t mpi_y = 1; mpi_y < 3; ++mpi_y)
            for (uint32_t mpi_x = 1; mpi_x < 3; ++mpi_x)
            {
                Dimensions mpi_size(mpi_x, mpi_y, mpi_z);
                size_t num_ranks = mpi_size.getScalarSize();

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
                            MPI_INFO_NULL, mpi_size, 10);

                    for (uint32_t elements = 1; elements <= 10; elements++)
                    {
                        if (my_current_mpi_rank == 0)
                        {
#if defined TESTS_DEBUG
                            std::cout << std::endl << "** testing elements = " <<
                                    elements << " with mpi size " <<
                                    mpi_size.toString() << std::endl;
#else
                            std::cout << "." << std::flush;
#endif
                        }

                        MPI_CHECK(MPI_Barrier(mpi_current_comm));

                        subTestPolyDomains(iteration,
                                my_current_mpi_rank,
                                mpi_size,
                                current_mpi_pos,
                                elements,
                                dimensions,
                                mpi_current_comm);

                        MPI_CHECK(MPI_Barrier(mpi_current_comm));

                        iteration++;
                    }

                    delete parallelDomainCollector;
                    parallelDomainCollector = NULL;

                    MPI_Comm_free(&mpi_current_comm);

                } else
                    iteration += 10;

                MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

            }

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

}

void Parallel_DomainsTest::testAppendDomains()
{
    const ColTypeInt ctInt;
    const Dimensions mpi_size(totalMpiSize, 1, 1);
    const Dimensions local_grid_size(10, 5, 3);
    const Dimensions global_grid_size = mpi_size * local_grid_size;
    Dimensions mpi_position;
    indexToPos(myMpiRank, mpi_size, mpi_position);

    ParallelDomainCollector *pdc =
            new ParallelDomainCollector(MPI_COMM_WORLD, MPI_INFO_NULL, mpi_size, 1);
    DomainCollector::FileCreationAttr fAttr;
    fAttr.fileAccType = DataCollector::FAT_CREATE;

    pdc->open(hdf5_file_append, fAttr);

    pdc->reserveDomain(10, global_grid_size, 3, ctInt, "append/data",
            Dimensions(0, 0, 0), global_grid_size, DomainCollector::GridType);
    
    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));

    if (mpi_position[0] % 2 == 0)
    {
        int buffer[local_grid_size.getScalarSize()];

        for (int m = 0; m < 2; ++m)
        {
            for (size_t i = 0; i < local_grid_size.getScalarSize(); ++i)
                buffer[i] = mpi_position[0] + m;

            Dimensions write_offset = Dimensions(mpi_position[0] + m,
                    mpi_position[1], mpi_position[2]) * local_grid_size;
            
            pdc->append(10, local_grid_size, 3, write_offset, "append/data", buffer);
        }
    }

    pdc->close();

    // test data
    fAttr.fileAccType = DataCollector::FAT_READ;
    pdc->open(hdf5_file_append, fAttr);
            
    DataContainer *container = pdc->readDomain(10, "append/data",
            mpi_position * local_grid_size,
            local_grid_size, NULL, false);
    
    CPPUNIT_ASSERT(container);
    CPPUNIT_ASSERT(container->getNumSubdomains() == 1);
    
    int *data = (int*)(container->getIndex(0)->getData());
    for (size_t i = 0; i < container->getIndex(0)->getSize().getScalarSize(); ++i)
        CPPUNIT_ASSERT(data[i] == mpi_position[0]);
    
    delete container;
    
    pdc->close();
    
    delete pdc;
    pdc = NULL;

    MPI_CHECK(MPI_Barrier(MPI_COMM_WORLD));
}
