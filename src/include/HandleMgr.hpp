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



#ifndef HANDLEMGR_HPP
#define	HANDLEMGR_HPP

#include <map>
#include <set>
#include <string>

#include "DCException.hpp"
#include "Dimensions.hpp"

namespace DCollector
{

    typedef hid_t H5Handle;

    /**
     * Helper class for SerialDataCollector which manages a limited number of
     * concurrently opened file handles.
     * If the maximum number of opened file handles is reached, the handle
     * with the lowest number of accesses is closed.
     */
    class HandleMgr
    {
    private:

        typedef struct
        {
            H5Handle handle;
            uint32_t ctr;
        } HandleCtrStr;

        typedef struct
        {
            uint32_t index;
            uint32_t ctr;
        } IndexCtrStr;

        typedef std::map<uint32_t, HandleCtrStr> HandleMap;
        
    public:
        enum FileNameScheme
        {
            FNS_MPI = 0, FNS_ITERATIONS
        };
        
        typedef void (*FileCreateCallback)(H5Handle handle);
        typedef void (*FileOpenCallback)(H5Handle handle);
        typedef void (*FileCloseCallback)(H5Handle handle);
        
        /**
         * Constructor
         * @param maxHandles maximum number of allowed open file handles
         */
        HandleMgr(uint32_t maxHandles, FileNameScheme fileNameScheme);

        /**
         * Destructor
         */
        virtual ~HandleMgr();

        /**
         * Opens the handle manager for multiple files/handles
         * @param mpiSize MPI size
         * @param baseFilename base filename part (w/o MPI/ext)
         * @param fileAccProperties from SerialDataCollector
         * @param flags from SerialDataCollector
         */
        void open(Dimensions mpiSize, const std::string baseFilename,
                hid_t fileAccProperties, unsigned flags);

        /**
         * Opens the handle manager for a single file/handle
         * @param fullFilename full filename (w/ MPI/ext)
         * @param fileAccProperties from SerialDataCollector
         * @param flags from SerialDataCollector
         */
        void open(const std::string fullFilename,
                hid_t fileAccProperties, unsigned flags);

        /**
         * Closes the handle manager, closes all open file handles.
         */
        void close();

        /**
         * Get a file handle, opens/creates the file if necessary.
         * @param index MPI rank/iteration
         * @return file handle
         */
        H5Handle get(uint32_t index) throw (DCException);

        /**
         * Get a file handle, opens/creates the file if necessary.
         * @param mpiPos MPI position
         * @return file handle
         */
        H5Handle get(Dimensions mpiPos) throw (DCException);
        
        void registerFileCreate(FileCreateCallback callback);

        void registerFileOpen(FileOpenCallback callback);

        void registerFileClose(FileCloseCallback callback);

    private:
        uint32_t maxHandles;
        uint32_t numHandles;

        Dimensions mpiSize;
        std::string filename;
        FileNameScheme fileNameScheme;
        
        hid_t fileAccProperties;
        unsigned fileFlags;
        bool singleFile;

        HandleMap handles;
        IndexCtrStr leastAccIndex;
        std::set<uint32_t> createdFiles;
        
        // callback handles
        FileCreateCallback fileCreateCallback;
        FileOpenCallback fileOpenCallback;
        FileCloseCallback fileCloseCallback;

        static std::string getExceptionString(std::string func,
                std::string msg, const char *info);

        uint32_t indexFromPos(Dimensions& mpiPos);
        Dimensions posFromIndex(uint32_t index);
    };

}

#endif	/* HANDLEMGR_HPP */

