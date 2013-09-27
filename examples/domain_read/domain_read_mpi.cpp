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

#include <iostream>
#include <string>
#include <sstream>
#include <mpi.h>
#include <math.h>
#include <stdint.h>
#include "splash.h"

using namespace DCollector;

/**
 * This libSplash example demonstrates on how to use the DomainCollector class
 * to read multiple, domain-annotated libSplash files using N MPI processes.
 * 
 * The program expects the base part to a distributed libSplash file, i.e.
 * 'my_data', given that you have files like 'my_data_0_0_0.h5', ...
 */

void filesToProcesses(int mpiSize, int mpiRank, int fileMPISize,
        int &fileIndexStart, int &fileIndexEnd)
{
    if (mpiSize >= fileMPISize)
    {
        // more or equal number of MPI processes than files
        if (mpiRank < fileMPISize)
        {
            fileIndexStart = mpiRank;
            fileIndexEnd = mpiRank;
        }
    } else
    {
        // less MPI processes than files
        int filesPerProcess = floor((float) fileMPISize / (float) (mpiSize));
        fileIndexStart = mpiRank * filesPerProcess;
        fileIndexEnd = (mpiRank + 1) * filesPerProcess - 1;
        if (mpiRank == mpiSize - 1)
            fileIndexEnd = fileMPISize - 1;
    }
}

void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
{
    mpiPos[2] = index % mpiSize[2];
    mpiPos[1] = (index / mpiSize[2]) % mpiSize[1];
    mpiPos[0] = index / (mpiSize[1] * mpiSize[2]);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <libsplash-file-base>" << std::endl;
        return -1;
    }

    int mpi_rank, mpi_size;
    int files_start = 0, files_end = 0;
    Dimensions file_mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    // libSplash filename
    std::string filename;
    filename.assign(argv[1]);

    // create DomainCollector
    // read single file, not merged
    DomainCollector dc(100);
    DataCollector::FileCreationAttr fAttr;
    DataCollector::initFileCreationAttr(fAttr);
    fAttr.fileAccType = DataCollector::FAT_READ;

    // broadcast file MPI size from root to all processes
    uint32_t f_mpi_size[3];
    if (mpi_rank == 0)
    {
        fAttr.mpiPosition.set(0, 0, 0);
        dc.open(filename.c_str(), fAttr);

        dc.getMPISize(file_mpi_size);
        std::cout << mpi_rank << ": total file MPI size = " <<
                file_mpi_size.toString() << std::endl;

        for (int i = 0; i < 3; ++i)
            f_mpi_size[i] = file_mpi_size[i];

        dc.close();
    }
    MPI_Bcast(f_mpi_size, 3, MPI_INTEGER4, 0, MPI_COMM_WORLD);
    file_mpi_size.set(f_mpi_size[0], f_mpi_size[1], f_mpi_size[2]);

    // get number of files for this MPI process
    filesToProcesses(mpi_size, mpi_rank, file_mpi_size.getScalarSize(),
            files_start, files_end);

    for (int f = files_start; f <= files_end; ++f)
    {
        // get file MPI pos for this file index
        indexToPos(f, file_mpi_size, fAttr.mpiPosition);
        std::cout << mpi_rank << ": opening position " <<
                fAttr.mpiPosition.toString() << std::endl;

        dc.open(filename.c_str(), fAttr);

        // get number of entries
        int32_t *ids = NULL;
        size_t num_ids = 0;
        dc.getEntryIDs(NULL, &num_ids);

        if (num_ids == 0)
        {
            dc.close();
            continue;
        } else
        {
            ids = new int32_t[num_ids];
            dc.getEntryIDs(ids, &num_ids);
        }

        // get entries for 1. id (timestep)
        DataCollector::DCEntry *entries = NULL;
        size_t num_entries = 0;
        dc.getEntriesForID(ids[0], NULL, &num_entries);

        if (num_entries == 0)
        {
            delete[] ids;
            dc.close();
            continue;
        } else
        {
            entries = new DataCollector::DCEntry[num_entries];
            dc.getEntriesForID(ids[0], entries, &num_entries);
        }

        // read 1. entry
        DataCollector::DCEntry first_entry = entries[0];
        std::cout << "  " << mpi_rank << ": reading entry " << first_entry.name << std::endl;

        // read complete domain
        Domain domain = dc.getGlobalDomain(ids[0], first_entry.name.c_str());
        DomainCollector::DomDataClass dataClass = DomainCollector::UndefinedType;
        DataContainer* container = dc.readDomain(ids[0], first_entry.name.c_str(),
                domain.getOffset(), domain.getSize(), &dataClass, false);

        // access all elements, no matter how many subdomains
        for (size_t i = 0; i < container->getNumElements(); ++i)
        {
            void *element = container->getElement(i);
            // do anything with this element
            // ...
        }

        // POLY data might be distributed over multiple subdomains
        for (size_t d = 0; d < container->getNumSubdomains(); ++d)
        {
            DomainData* subdomain = container->getIndex(d);
            Dimensions size = subdomain->getSize();

            std::cout << "  " << mpi_rank << ": subdomain " << d << " has size " <<
                    size.toString() << std::endl;
            
            // access the underlying buffer of a subdomain
            void *elements = subdomain->getData();
        }

        // don't forget to delete the container allocated by DomainCollector
        delete container;

        delete[] entries;
        delete[] ids;

        dc.close();
    }

    MPI_Finalize();

    return 0;
}
