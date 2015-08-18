/**
 * Copyright 2013-2015 Felix Schmitt, Richard Pausch
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
#include <stdlib.h>
#include <string.h>

#define RESULT_OK 0
#define RESULT_ERROR 1

#if (ENABLE_MPI==1)
#include <mpi.h>
#endif

#include "splash/splash.h"

using namespace splash;

typedef struct
{
    bool singleFile;
    bool checkIntegrity;
    bool deleteStep;
    bool listEntries;
    bool parallelFile;
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
    options.listEntries = false;
    options.parallelFile = false;
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
    std::stringstream usage_stream;
    usage_stream << "Usage " << argv[0] << " [options] -f <splash-file>";

    std::stringstream full_desc_stream;
    full_desc_stream << usage_stream.str() << std::endl <<
            " --help,-h\t\t\t print this help message" << std::endl <<
            " --file,-f\t<file>\t\t HDF5 libSplash file to edit" << std::endl <<
            " --delete,-d\t<step>\t\t Delete [d,*) simulation steps" << std::endl <<
            " --check,-c\t\t\t Check file integrity" << std::endl <<
            " --list,-l\t\t\t List all file entries" << std::endl <<
#if (SPLASH_SUPPORTED_PARALLEL==1)
            " --parallel,-p\t\t\t Input is parallel libSplash file" << std::endl <<
#endif
            " --verbose,-v\t\t\t Verbose output";

    if (argc < 2)
    {
        std::cerr << "Too few arguments" << std::endl;
        std::cerr << usage_stream.str() << std::endl;
        return RESULT_ERROR;
    }

    for (int i = 1; i < argc; ++i)
    {
        const char *option = argv[i];
        char *next_option = NULL;
        int has_next_option = (i < argc - 1);
        if (has_next_option)
            next_option = argv[i + 1];

        // help
        if ((strcmp(option, "-h") == 0) || (strcmp(option, "--help") == 0))
        {
            std::cout << "[" << options.mpiRank << "] " <<
                    full_desc_stream.str() << std::endl;
            return RESULT_OK;
        }

        // verbose
        if ((strcmp(option, "-v") == 0) || (strcmp(option, "--verbose") == 0))
        {
            options.verbose = true;
            continue;
        }

        // delete
        if ((strcmp(option, "-d") == 0) || (strcmp(option, "--delete") == 0))
        {
            if (!has_next_option)
            {
                std::cerr << "Option delete requires argument" << std::endl;
                return RESULT_ERROR;
            }

            options.step = atoi(next_option);
            i++;
            continue;
        }

        // filename
        if ((strcmp(option, "-f") == 0) || (strcmp(option, "--file") == 0))
        {
            if (!has_next_option)
            {
                std::cerr << "Option file requires argument" << std::endl;
                return RESULT_ERROR;
            }

            options.filename.assign(next_option);
            i++;
            continue;
        }

        // check
        if ((strcmp(option, "-c") == 0) || (strcmp(option, "--check") == 0))
        {
            options.checkIntegrity = true;
            continue;
        }

        // list
        if ((strcmp(option, "-l") == 0) || (strcmp(option, "--list") == 0))
        {
            options.listEntries = true;
            continue;
        }

#if (SPLASH_SUPPORTED_PARALLEL==1)
        // parallel file
        if ((strcmp(option, "-p") == 0) || (strcmp(option, "--parallel") == 0))
        {
            options.parallelFile = true;
            continue;
        }
#endif

        std::cerr << "Unknown option '" << option << "'" << std::endl;
        std::cerr << full_desc_stream.str() << std::endl;
        return RESULT_ERROR;
    }

    if (options.filename.size() == 0)
    {
        std::cerr << "Missing libSplash filename" << std::endl;
        std::cerr << usage_stream.str() << std::endl;
        return RESULT_ERROR;
    }

    if (options.filename.find(".h5") != std::string::npos)
    {
        options.singleFile = true;
        if ((options.mpiRank == 0) && (options.verbose))
            std::cout << "[" << options.mpiRank << "] " <<
                "single file mode" << std::endl;
    }

    return RESULT_OK;
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
        std::cout << "[" << options.mpiRank << "] " <<
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
        std::cout << "[" << options.mpiRank << "] " <<
                "popen failed with status " << status << std::endl;
        return RESULT_ERROR;
    } else
    {
        if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
        {
            std::cout << "[" << options.mpiRank << "] " <<
                    "h5check returned error (status " << status << ") for file '" <<
                    filename << "'" << std::endl;
            return RESULT_ERROR;
        } else
        {
            if (options.verbose)
            {
                std::cout << "[" << options.mpiRank << "] " <<
                        "file '" << filename << "' ok" << std::endl;
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

int detectFileMPISize(Options& options, Dimensions &fileMPISizeDim)
{
    int result = RESULT_OK;

    DataCollector *dc = NULL;
#if (SPLASH_SUPPORTED_PARALLEL==1)
    if (options.parallelFile)
        dc = new ParallelDataCollector(MPI_COMM_WORLD, MPI_INFO_NULL,
            Dimensions(options.mpiSize, 1, 1), 1);
    else
#endif
        dc = new SerialDataCollector(1);

    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_READ;

    try
    {
        dc->open(options.filename.c_str(), fileCAttr);
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
#if (SPLASH_SUPPORTED_PARALLEL==1)
            if (options.parallelFile)
                dc = new ParallelDataCollector(MPI_COMM_WORLD, MPI_INFO_NULL,
                    Dimensions(options.mpiSize, 1, 1), 1);
            else
#endif
                dc = new SerialDataCollector(1);

            result = toolFunc(options, dc, options.filename.c_str());

            delete dc;
            dc = NULL;
        }
    } else
    {
        // open master file to detect number of files
        uint64_t fileMPISizeBuffer[3] = {0, 0, 0};
        Dimensions fileMPISizeDim(0, 0, 0);
        int fileMPISize = 0;

        if (options.mpiRank == 0)
        {
            result = detectFileMPISize(options, fileMPISizeDim);

            for (int i = 0; i < 3; ++i)
                fileMPISizeBuffer[i] = fileMPISizeDim[i];
        }

#if (ENABLE_MPI==1)
        MPI_Bcast(fileMPISizeBuffer, 3, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
#endif

        if (options.mpiRank != 0)
            fileMPISizeDim.set(fileMPISizeBuffer[0], fileMPISizeBuffer[1], fileMPISizeBuffer[2]);

        if (fileMPISizeDim.getScalarSize() == 0)
            return RESULT_ERROR;

        fileMPISize = fileMPISizeBuffer[0] * fileMPISizeBuffer[1] * fileMPISizeBuffer[2];

        // get file index range for each process
        filesToProcesses(options, fileMPISize);

        if (options.fileIndexStart == -1)
            return RESULT_OK;

#if (SPLASH_SUPPORTED_PARALLEL==1)
        if (options.parallelFile)
            dc = new ParallelDataCollector(MPI_COMM_WORLD, MPI_INFO_NULL,
                Dimensions(options.mpiSize, 1, 1), 1);
        else
#endif
            dc = new SerialDataCollector(1);

        for (int i = options.fileIndexStart; i <= options.fileIndexEnd; ++i)
        {
            Dimensions mpi_pos(0, 0, 0);
            // get mpi position from index
            indexToPos(i, fileMPISizeDim, mpi_pos);

            // delete steps in this file
            std::stringstream mpiFilename;
#if (SPLASH_SUPPORTED_PARALLEL==1)
            if (options.parallelFile)
                mpiFilename << options.filename << "_" << i << ".h5";
            else
#endif
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
        std::cerr << "[" << options.mpiRank << "] " <<
                "Deleting in file " << filename << " failed!" << std::endl <<
                e.what() << std::endl;
        result = RESULT_ERROR;
    }

    return result;
}

int testFileIntegrity(Options& options, DataCollector* /*dc*/, const char* filename)
{
    return testIntegrity(options, filename);
}

int listAvailableDatasets(Options& options, DataCollector *dc, const char* /*filename*/)
{
    DataCollector::FileCreationAttr fileCAttr;
    DataCollector::initFileCreationAttr(fileCAttr);
    fileCAttr.fileAccType = DataCollector::FAT_READ;

    dc->open(options.filename.c_str(), fileCAttr);

    int32_t id = 0;

    if (!options.parallelFile)
    {
        size_t num_ids = 0;
        dc->getEntryIDs(NULL, &num_ids);
        if (num_ids > 0)
        {
            int32_t ids[num_ids];
            dc->getEntryIDs(ids, NULL);
            id = ids[0];
        } else
        {
            std::cout << "no IDs" << std::endl;
            dc->close();
            return RESULT_OK;
        }
    }

    // number of iterations in this file
    size_t num_entries = 0;
    dc->getEntriesForID(id, NULL, &num_entries);

    if (num_entries > 0)
    {
        DataCollector::DCEntry *entries = new DataCollector::DCEntry[num_entries];
        dc->getEntriesForID(id, entries, NULL);

        for (size_t i = 0; i < num_entries; ++i)
            std::cout << entries[i].name << std::endl;
        
        delete[] entries;
    }

    dc->close();
    return RESULT_OK;
}

/* main */

int main(int argc, char **argv)
{
    int result = RESULT_OK;
    Options options;
    initOptions(options);

#if (ENABLE_MPI==1)
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

        if (options.listEntries)
            result = executeToolFunction(options, listAvailableDatasets);
    }

#if (ENABLE_MPI==1)
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
#endif

    return result;
}
