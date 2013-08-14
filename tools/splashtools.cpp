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
    bool verbose;
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
    options.verbose = false;
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
                ("verbose,v", po::value<bool > (&options.verbose)->zero_tokens(), "Verbose output")
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
                if ((options.mpiRank == 0) && (options.verbose))
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
                " failed to execute '" << command << "'" << std::endl;
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
                " popen failed with status " << status << std::endl;
        return RESULT_ERROR;
    } else
    {
        if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
        {
            std::cout << "[" << options.mpiRank << "]" <<
                    " h5check returned error (status " << status << ") for file '" <<
                    filename << "'" << std::endl;
            return RESULT_ERROR;
        } else
        {
            if (options.verbose)
            {
                std::cout << "[" << options.mpiRank << "]" <<
                        " file '" << filename << "' ok" << std::endl;
            }
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

int detectFileMPISize(const char *filename, Dimensions &fileMPISizeDim)
{
    int result = RESULT_OK;

    DataCollector *dc = new SerialDataCollector(1);
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_READ;

    try
    {
        dc->open(filename, fileCAttr);
        dc->getMPISize(fileMPISizeDim);
        dc->close();
    } catch (DCException e)
    {
        std::cerr << "[0] Detecting file MPI size failed!" << std::endl <<
                e.what() << std::endl;
        fileMPISizeDim.set(0, 0, 0);
        result = RESULT_ERROR;
    }

    delete dc;
    dc = NULL;

    return result;
}

/* tool functions */

int executeToolFunction(Options& options,
        int (*toolFunc)(Options& options, DataCollector *dc, const char *filename))
{
    DataCollector *dc = NULL;
    int result = RESULT_OK;

    if (options.singleFile)
    {
        if (options.mpiRank == 0)
        {
            dc = new SerialDataCollector(1);

            result = toolFunc(options, dc, options.filename.c_str());

            delete dc;
            dc = NULL;
        }
    } else
    {
        // open master file to detect number of files
        uint32_t fileMPISizeBuffer[3] = {0, 0, 0};
        Dimensions fileMPISizeDim(0, 0, 0);
        int fileMPISize = 0;

        if (options.mpiRank == 0)
        {
            result = detectFileMPISize(options.filename.c_str(), fileMPISizeDim);

            for (int i = 0; i < 3; ++i)
                fileMPISizeBuffer[i] = fileMPISizeDim[i];
        }

#ifdef ENABLE_MPI
        MPI_Bcast(fileMPISizeBuffer, 3, MPI_INTEGER4, 0, MPI_COMM_WORLD);
#endif

        if (options.mpiRank != 0)
            fileMPISizeDim.set(fileMPISizeBuffer[0], fileMPISizeBuffer[1], fileMPISizeBuffer[2]);

        if (fileMPISizeDim.getDimSize() == 0)
            return RESULT_ERROR;

        fileMPISize = fileMPISizeBuffer[0] * fileMPISizeBuffer[1] * fileMPISizeBuffer[2];

        // get file index range for each process
        filesToProcesses(options, fileMPISize);

        if (options.fileIndexStart == -1)
            return RESULT_OK;

        dc = new SerialDataCollector(1);

        for (int i = options.fileIndexStart; i <= options.fileIndexEnd; ++i)
        {
            Dimensions mpi_pos(0, 0, 0);
            // get mpi position from index
            indexToPos(i, fileMPISizeDim, mpi_pos);

            // delete steps in this file
            std::stringstream mpiFilename;
            mpiFilename << options.filename << "_" << mpi_pos[0] << "_" <<
                    mpi_pos[1] << "_" << mpi_pos[2] << ".h5";

            result |= toolFunc(options, dc, mpiFilename.str().c_str());
        }

        delete dc;
        dc = NULL;

        return result;
    }

    return result;
}

int deleteFromStep(Options& options, DataCollector *dc, const char *filename)
{
    int result = RESULT_OK;
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_WRITE;

    if (options.verbose)
    {
        std::cout << "[" << options.mpiRank << "] Deleting from step " <<
                options.step << " in file " << filename << std::endl;
    }

    try
    {
        dc->open(filename, fileCAttr);
        deleteFromStepInFile(dc, options.step);
        dc->close();
    } catch (DCException e)
    {
        std::cerr << "Deleting in file " << filename << " failed!" << std::endl <<
                e.what() << std::endl;
        result = RESULT_ERROR;
    }

    return result;
}

int testFileIntegrity(Options& options, DataCollector *dc, const char *filename)
{
    return testIntegrity(options, filename);
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
            result = executeToolFunction(options, testFileIntegrity);

        if (options.deleteStep)
            result = executeToolFunction(options, deleteFromStep);
    }

#ifdef ENABLE_MPI
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
#endif

    return result;
}
