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
#include <sstream>
#include <vector>
#include <math.h>

#define RESULT_OK 0
#define RESULT_ERROR 1

#ifdef ENABLE_MPI
#include <mpi.h>
#endif

#include "SerialDataCollector.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

using namespace DCollector;

typedef struct
{
    bool singleFile;
    bool checkIntegrity;
    bool deleteStep;
    std::string filename;
    int32_t step;
    int mpiRank;
    int mpiSize;
    int fileIndexStart;
    int fileIndexEnd;
} Options;

int deleteFromStep(Options& options);
int testFileIntegrity(Options& options);

/* helper functions */

void initOptions(Options& options)
{
    options.checkIntegrity = false;
    options.deleteStep = false;
    options.fileIndexEnd = -1;
    options.fileIndexStart = -1;
    options.filename = "";
    options.mpiRank = 0;
    options.mpiSize = 1;
    options.singleFile = false;
    options.step = 0;
}

void deleteFromStepInFile(DataCollector *dc, int32_t step)
{
    size_t num_entries = 0;
    dc->getEntryIDs(NULL, &num_entries);

    int32_t *entries = new int32_t[num_entries];
    dc->getEntryIDs(entries, NULL);

    for (size_t i = 0; i < num_entries; ++i)
    {
        if (entries[i] >= step)
        {
            std::cout << "removing group " << entries[i] << std::endl;
            dc->remove(entries[i]);
        }
    }

    delete[] entries;
    entries = NULL;
}

int parseCmdLine(int argc, char **argv, Options& options)
{
    try
    {
        std::stringstream desc_stream;
        desc_stream << "Usage " << argv[0] << " [options] -f <splash-file>" << std::endl;

        po::options_description desc(desc_stream.str());

        // add possible options
        desc.add_options()
                ("help,h", "print help message")
                ("file,f", po::value<std::string > (&options.filename), "HDF5 libSplash file to edit")
                ("delete,d", po::value<int32_t > (&options.step), "Delete [d,*) simulation steps")
                ("check,c", po::value<bool > (&options.checkIntegrity)->zero_tokens(), "Check file integrity")
                ;

        // parse command line options and config file and store values in vm
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        // print help message and quit
        if (vm.count("help"))
        {
            std::cout << "[" << options.mpiRank << "]" << desc << "\n";
            return RESULT_OK;
        }

        if (vm.count("file") == 0)
        {
            std::cout << "[" << options.mpiRank << "]" <<
                    "Missing libSplash file" << std::endl;
            std::cout << desc << "\n";
            return RESULT_ERROR;
        } else
        {
            if (options.filename.find(".h5") != std::string::npos)
            {
                options.singleFile = true;
                if (options.mpiRank == 0)
                    std::cout << "[" << options.mpiRank << "]" <<
                        "single file mode" << std::endl;
            }
        }

        /* run requested tool command */

        if (vm.count("delete"))
        {
            options.deleteStep = true;
            return RESULT_OK;
        }

        if (vm.count("check"))
        {
            options.checkIntegrity = true;
            return RESULT_OK;
        }

    } catch (boost::program_options::error e)
    {
        std::cout << e.what() << std::endl;
        return RESULT_ERROR;
    }

    return RESULT_ERROR;
}

int testIntegrity(Options &options, std::string filename)
{
    // test file integrity using h5check
    char *command = NULL;
    const char *h5CheckCmd = "h5check -v0";
    const char *devNull = "> /dev/null";
    char checkStr[256];
    int status = 0;

    int len = strlen(h5CheckCmd) + 1 + filename.length() + 1 + strlen(devNull);
    command = new char[len + 1];
    sprintf(command, "%s %s %s", h5CheckCmd, filename.c_str(), devNull);

    FILE *file = popen(command, "r");
    if (!file)
    {
        std::cout << "[" << options.mpiRank << "]" <<
                "failed to execute '" << command << "'" << std::endl;
        delete[] command;
        return RESULT_ERROR;
    }

    delete[] command;
    while (fgets(checkStr, 256, file) != NULL)
        printf("%s", checkStr);

    status = pclose(file);
    if (status == -1)
    {
        std::cout << "[" << options.mpiRank << "]" <<
                "popen failed with status " << status << std::endl;
        return RESULT_ERROR;
    } else
    {
        if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
        {
            std::cout << "[" << options.mpiRank << "]" <<
                    "h5check returned error (status " << status << ")" << std::endl;
            return RESULT_ERROR;
        } else
        {
            std::cout << "[" << options.mpiRank << "]" <<
                    "file '" << filename << "' ok" << std::endl;
            return RESULT_OK;
        }
    }

    return RESULT_ERROR;
}

void filesToProcesses(Options& options, int fileMPISize)
{
    if (options.mpiSize >= fileMPISize)
    {
        // more or equal number of MPI processes than files
        if (options.mpiRank < fileMPISize)
        {
            options.fileIndexStart = options.mpiRank;
            options.fileIndexEnd = options.mpiRank;
        }
    } else
    {
        // less MPI processes than files
        int filesPerProcess = floor((float) fileMPISize / (float) (options.mpiSize));
        options.fileIndexStart = options.mpiRank * filesPerProcess;
        options.fileIndexEnd = (options.mpiRank + 1) * filesPerProcess - 1;
        if (options.mpiRank == options.mpiSize - 1)
            options.fileIndexEnd = fileMPISize - 1;
    }
}

void indexToPos(int index, Dimensions mpiSize, Dimensions &mpiPos)
{
    mpiPos[2] = index % mpiSize[2];
    mpiPos[1] = (index / mpiSize[2]) % mpiSize[1];
    mpiPos[0] = index / (mpiSize[1] * mpiSize[2]);
}

/* tool functions */

int deleteFromStep(Options& options)
{
    DataCollector *dc = new SerialDataCollector(100);

    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);

    if (options.singleFile)
    {
        if (options.mpiRank == 0)
        {
            std::cout << "[" << options.mpiRank << "]" <<
                    "deleting from step " << options.step << std::endl;
            fileCAttr.fileAccType = DataCollector::FAT_WRITE;

            dc->open(options.filename.c_str(), fileCAttr);
            deleteFromStepInFile(dc, options.step);
            dc->close();
        }
    } else
    {
        if (options.mpiRank != 0)
            return RESULT_OK;
        
        fileCAttr.fileAccType = DataCollector::FAT_READ;

        // get mpi size and max id from reference file
        dc->open(options.filename.c_str(), fileCAttr);

        Dimensions fileMPISize(0, 0, 0);
        dc->getMPISize(fileMPISize);

        dc->close();

        std::cout << "[" << options.mpiRank << "]" <<
                "deleting from step " << options.step << std::endl;
        std::cout << "[" << options.mpiRank << "]" <<
                "mpi size = " << fileMPISize.toString() << std::endl;

        fileCAttr.fileAccType = DataCollector::FAT_WRITE;
        for (uint32_t z = 0; z < fileMPISize[2]; ++z)
            for (uint32_t y = 0; y < fileMPISize[1]; ++y)
                for (uint32_t x = 0; x < fileMPISize[0]; ++x)
                {
                    fileCAttr.mpiPosition.set(x, y, z);
                    std::cout << "[" << options.mpiRank << "]" <<
                            " Removing from mpi position " <<
                            fileCAttr.mpiPosition.toString() << std::endl;

                    dc->open(options.filename.c_str(), fileCAttr);
                    deleteFromStepInFile(dc, options.step);
                    dc->close();
                }
    }

    delete dc;
    return RESULT_OK;
}

int testFileIntegrity(Options& options)
{
    if (options.singleFile)
    {
        if (options.mpiRank == 0)
            return testIntegrity(options, options.filename);
    } else
    {
        // open master file to detect number of files
        uint32_t fileMPISizeBuffer[3];
        Dimensions fileMPISizeDim(0, 0, 0);
        int fileMPISize = 0;

        if (options.mpiRank == 0)
        {
            DataCollector *dc = new SerialDataCollector(1);

            DataCollector::FileCreationAttr fileCAttr;
            DataCollector::initFileCreationAttr(fileCAttr);
            fileCAttr.fileAccType = DataCollector::FAT_READ;

            dc->open(options.filename.c_str(), fileCAttr);
            dc->getMPISize(fileMPISizeDim);
            dc->close();

            for (int i = 0; i < 3; ++i)
                fileMPISizeBuffer[i] = fileMPISizeDim[i];

            delete dc;
        }

#ifdef ENABLE_MPI
        MPI_Bcast(fileMPISizeBuffer, 3, MPI_INTEGER4, 0, MPI_COMM_WORLD);
#endif

        if (options.mpiRank != 0)
            fileMPISizeDim.set(fileMPISizeBuffer[0], fileMPISizeBuffer[1], fileMPISizeBuffer[2]);

        fileMPISize = fileMPISizeBuffer[0] * fileMPISizeBuffer[1] * fileMPISizeBuffer[2];

        // get file index range for each process
        filesToProcesses(options, fileMPISize);

        if (options.fileIndexStart == -1)
            return RESULT_OK;

        int result = RESULT_OK;
        for (int i = options.fileIndexStart; i <= options.fileIndexEnd; ++i)
        {
            Dimensions mpiPos(0, 0, 0);
            // get mpi position from index
            indexToPos(i, fileMPISizeDim, mpiPos);

            // test file integrity
            std::stringstream mpiFilename;
            mpiFilename << options.filename << "_" << mpiPos[0] << "_" <<
                    mpiPos[1] << "_" << mpiPos[2] << ".h5";

            result |= testIntegrity(options, mpiFilename.str());
        }

        return result;
    }

    return RESULT_OK;
}

/* main */

int main(int argc, char **argv)
{
    int result = RESULT_OK;
    Options options;
    initOptions(options);

#ifdef ENABLE_MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &(options.mpiRank));
    MPI_Comm_size(MPI_COMM_WORLD, &(options.mpiSize));
#endif

    result = parseCmdLine(argc, argv, options);
    if (result == RESULT_OK)
    {
        if (options.checkIntegrity)
            result = testFileIntegrity(options);

        if (options.deleteStep)
            result = deleteFromStep(options);
    }

#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
#endif

    return result;
}
