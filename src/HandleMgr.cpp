/**
 * Copyright 2013-2016 Felix Schmitt, Alexander Grund
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



#include <limits>
#include <iostream>
#include <hdf5.h>

#include "splash/core/HandleMgr.hpp"
#include "splash/core/logging.hpp"
#include "splash/core/DCHelper.hpp"

namespace splash
{

    HandleMgr::HandleMgr(uint32_t maxHandles, FileNameScheme fileNameScheme) :
    maxHandles(maxHandles),
    mpiSize(1, 1, 1),
    fileNameScheme(fileNameScheme),
    fileFlags(0),
    fileCreateCallback(NULL),
    fileCreateUserData(NULL),
    fileOpenCallback(NULL),
    fileOpenUserData(NULL),
    fileCloseCallback(NULL),
    fileCloseUserData(NULL)
    {
        if (maxHandles == 0)
            this->maxHandles = std::numeric_limits<uint32_t>::max() - 1;

        leastAccIndex.ctr = 0;
        leastAccIndex.index = 0;
    }

    HandleMgr::~HandleMgr()
    {
    }

    std::string HandleMgr::getExceptionString(std::string func,
            std::string msg, const char *info)
    {
        std::stringstream full_msg;
        full_msg << "Exception for HandleMgr::" << func <<
                ": " << msg;

        if (info != NULL)
            full_msg << " (" << info << ")";

        return full_msg.str();
    }

    void HandleMgr::setFileNameScheme(FileNameScheme fileNameScheme) throw (DCException)
    {
        if (this->fileNameScheme == fileNameScheme)
            return;
        if (!filename.empty())
            throw DCException(getExceptionString("setFileNameScheme",
                                    "Tried to change scheme while file(s) were still open", ""));
        this->fileNameScheme = fileNameScheme;
    }

    void HandleMgr::open(Dimensions mpiSize, const std::string baseFilename,
            hid_t fileAccProperties, unsigned flags)
    throw (DCException)
    {
        this->mpiSize.set(mpiSize);
        this->filename = baseFilename;
        this->fileAccProperties = fileAccProperties;
        this->fileFlags = flags;
        // Validation: For parallel files we normally append MPI rank or iteration number.
        //             This is disabled by using FNS_FULLNAME
        //             or when the filename already contains an h5-extension.
        if (fileNameScheme != FNS_FULLNAME && baseFilename.find(".h5") == baseFilename.length() - 3)
        {
            if (mpiSize.getScalarSize() > 1)
            {
                throw DCException(getExceptionString("open",
                        "Passed full filename for parallel file operations",
                        baseFilename.c_str()));
            } else
            {
                log_msg(1, "\n"
                        "\tWarning: Passed full filename for parallel file operations: %s\n"
                        "It is recommended to pass only the base name (no extension)"
                        "and let the implementation choose a filename.\n", filename.c_str());
            }
        }
    }

    void HandleMgr::open(const std::string fullFilename,
            hid_t fileAccProperties, unsigned flags)
    throw (DCException)
    {
        setFileNameScheme(FNS_FULLNAME);
        this->mpiSize.set(1, 1, 1);
        this->filename = fullFilename;
        this->fileAccProperties = fileAccProperties;
        this->fileFlags = flags;
    }

    uint32_t HandleMgr::indexFromPos(Dimensions& mpiPos)
    {
        return mpiPos[0] + mpiPos[1] * mpiSize[0] +
                mpiPos[2] * mpiSize[0] * mpiSize[1];
    }

    Dimensions HandleMgr::posFromIndex(uint32_t index)
    {
        Dimensions pos(0, 0, 0);

        if (index > 0)
        {
            if (fileNameScheme == FNS_MPI)
                pos.set(
                    index % mpiSize[0],
                    (index / mpiSize[0]) % mpiSize[1],
                    index / (mpiSize[1] * mpiSize[0]));
            else
                pos.set(index, 0, 0);
        }

        return pos;
    }

    H5Handle HandleMgr::get(uint32_t index)
    throw (DCException)
    {
        return get(posFromIndex(index));
    }

    H5Handle HandleMgr::get(Dimensions mpiPos)
    throw (DCException)
    {
        uint32_t index = 0;
        if (fileNameScheme != FNS_FULLNAME)
            index = indexFromPos(mpiPos);

        HandleMap::iterator iter = handles.find(index);
        if (iter == handles.end())
        {
            if (handles.size() + 1 > maxHandles)
            {
                HandleMap::iterator rmHandle = handles.find(leastAccIndex.index);
                if (fileCloseCallback)
                {
                    fileCloseCallback(rmHandle->second.handle,
                            leastAccIndex.index, fileCloseUserData);
                }

                if (H5Fclose(rmHandle->second.handle) < 0)
                {
                    throw DCException(getExceptionString("get", "Failed to close file handle",
                            mpiPos.toString().c_str()));
                }

                handles.erase(rmHandle);
                leastAccIndex.ctr = 0;
            }

            // Append prefix and extension if we don't have a full filename (extension)
            std::string fullFilename;
            if (fileNameScheme != FNS_FULLNAME && filename.find(".h5") != filename.length() - 3)
            {
                std::stringstream filenameStream;
                filenameStream << filename;
                if (fileNameScheme == FNS_MPI)
                {
                    filenameStream << "_" << mpiPos[0] << "_" << mpiPos[1] <<
                            "_" << mpiPos[2] << ".h5";
                } else if(fileNameScheme == FNS_ITERATIONS)
                    filenameStream << "_" << mpiPos[0] << ".h5";
                fullFilename = filenameStream.str();
            }else
                fullFilename = filename;

            H5Handle newHandle = 0;

            // serve requests to create files once as create and as read/write afterwards
            if ((fileFlags & H5F_ACC_TRUNC) && (createdFiles.find(index) == createdFiles.end()))
            {
                DCHelper::testFilename(fullFilename);

                newHandle = H5Fcreate(fullFilename.c_str(), fileFlags,
                        H5P_FILE_CREATE_DEFAULT, fileAccProperties);
                if (newHandle < 0)
                    throw DCException(getExceptionString("get", "Failed to create file",
                            fullFilename.c_str()));

                createdFiles.insert(index);

                if (fileCreateCallback)
                    fileCreateCallback(newHandle, index, fileCreateUserData);
            } else
            {
                // file open or already created
                unsigned tmp_flags = fileFlags;
                if (fileFlags & H5F_ACC_TRUNC)
                    tmp_flags = H5F_ACC_RDWR;

                newHandle = H5Fopen(fullFilename.c_str(), tmp_flags, fileAccProperties);
                if (newHandle < 0)
                    throw DCException(getExceptionString("get", "Failed to open file",
                            fullFilename.c_str()));

                if (fileOpenCallback)
                    fileOpenCallback(newHandle, index, fileOpenUserData);
            }


            handles[index].handle = newHandle;
            handles[index].ctr = 1;

            if ((leastAccIndex.ctr == 0) || (1 < leastAccIndex.ctr))
            {
                leastAccIndex.ctr = 1;
                leastAccIndex.index = index;
            }

            return newHandle;
        } else
        {
            iter->second.ctr++;
            if (leastAccIndex.index == index)
                leastAccIndex.ctr++;

            return iter->second.handle;
        }
    }

    void HandleMgr::close()
    {
        // clean internal state
        createdFiles.clear();
        filename = "";
        fileAccProperties = 0;
        fileFlags = 0;
        leastAccIndex.ctr = 0;
        leastAccIndex.index = 0;
        mpiSize.set(1, 1, 1);

        // close all remaining handles
        HandleMap::const_iterator iter = handles.begin();
        for (; iter != handles.end(); ++iter)
        {
            if (fileCloseCallback)
                fileCloseCallback(iter->second.handle, iter->first, fileCloseUserData);

            if (H5Fclose(iter->second.handle) < 0)
            {
                throw DCException(getExceptionString("close", "Failed to close file handle",
                        posFromIndex(iter->first).toString().c_str()));
            }
        }

        handles.clear();
    }

    void HandleMgr::registerFileCreate(FileCreateCallback callback, void *userData)
    {
        fileCreateCallback = callback;
        fileCreateUserData = userData;
    }

    void HandleMgr::registerFileOpen(FileOpenCallback callback, void *userData)
    {
        fileOpenCallback = callback;
        fileOpenUserData = userData;
    }

    void HandleMgr::registerFileClose(FileCloseCallback callback, void *userData)
    {
        fileCloseCallback = callback;
        fileCloseUserData = userData;
    }

}
