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



#include <string>
#include <sstream>
#include <cassert>
#include <string.h>

#include "core/DCGroup.hpp"

#define H5_TRUE 1
#define H5_FALSE 0

namespace splash
{

    std::string DCGroup::getExceptionString(std::string msg, std::string name)
    {
        return (std::string("Exception for DCGroup [") + name + std::string("] ") +
                msg);
    }

    DCGroup::DCGroup() :
    checkExistence(true)
    {
    }

    DCGroup::~DCGroup()
    {
        close();
    }

    H5Handle DCGroup::create(H5Handle base, std::string path)
    throw (DCException)
    {
        bool mustCreate = false;
        H5Handle currentHandle = base;
        char c_path[path.size() + 1];
        strcpy(c_path, path.c_str());

        char *token = strtok(c_path, "/");
        while (token)
        {
            if (mustCreate || !H5Lexists(currentHandle, token, H5P_DEFAULT))
            {
                H5Handle newHandle = H5Gcreate(currentHandle, token, H5P_LINK_CREATE_DEFAULT,
                        H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
                if (newHandle < 0)
                    throw DCException(getExceptionString("Failed to create group", path));

                currentHandle = newHandle;
                mustCreate = true;
            } else
            {
                currentHandle = H5Gopen(currentHandle, token, H5P_DEFAULT);
                if (currentHandle < 0)
                {
                    if (checkExistence)
                        throw DCException(getExceptionString("Failed to create group", path));
                    
                    currentHandle = H5Gcreate(currentHandle, token, H5P_LINK_CREATE_DEFAULT,
                        H5P_GROUP_CREATE_DEFAULT, H5P_GROUP_ACCESS_DEFAULT);
                    if (currentHandle < 0)
                        throw DCException(getExceptionString("Failed to create group", path));
                    
                    mustCreate = true;
                }
            }

            handles.push_back(currentHandle);
            token = strtok(NULL, "/");
        }

        return currentHandle;
    }

    H5Handle DCGroup::open(H5Handle base, std::string path)
    throw (DCException)
    {
        H5Handle newHandle;

        if (checkExistence && !H5Lexists(base, path.c_str(), H5P_DEFAULT))
            throw DCException(getExceptionString("Failed to open group", path));

        newHandle = H5Gopen(base, path.c_str(), H5P_DEFAULT);

        if (newHandle < 0)
            throw DCException(getExceptionString("Failed to open group", path));

        handles.push_back(newHandle);
        return newHandle;
    }

    H5Handle DCGroup::openCreate(H5Handle base, std::string path)
    throw (DCException)
    {
        if (checkExistence && exists(base, path))
            return open(base, path);
        else
            return create(base, path);
    }

    void DCGroup::close()
    throw (DCException)
    {
        for (HandlesList::const_reverse_iterator iter = handles.rbegin();
                iter != handles.rend(); ++iter)
        {
            if (H5Gclose(*iter) < 0)
                throw DCException(getExceptionString("Failed to close group", ""));
        }

        handles.clear();
    }

    bool DCGroup::exists(H5Handle base, std::string path)
    {
        return (H5Lexists(base, path.c_str(), H5P_DEFAULT) == H5_TRUE);
    }

    void DCGroup::remove(H5Handle base, std::string path)
    throw (DCException)
    {
        if (H5Ldelete(base, path.c_str(), H5P_LINK_ACCESS_DEFAULT) < 0)
            throw DCException(getExceptionString("failed to remove group", path));
    }

    H5Handle DCGroup::getHandle()
    {
        if (handles.size() > 0)
            return *(handles.rbegin());
        else
            return INVALID_HANDLE;
    }
    
    void DCGroup::getEntriesInternal(H5Handle base, const std::string baseGroup,
            std::string baseName, VisitObjCBType *param)
    throw (DCException)
    {
        H5G_info_t group_info;
        H5Gget_info(base, &group_info);

        for (size_t i = 0; i < group_info.nlinks; ++i)
        {
            std::string currentBaseName = baseName;
            std::string currentEntryName = "";

            H5O_info_t obj_info;
            H5Oget_info_by_idx(base, ".", H5_INDEX_NAME, H5_ITER_INC, i, &obj_info, H5P_DEFAULT);

            if (param->entries)
            {
                ssize_t len_name = H5Lget_name_by_idx(base, ".", H5_INDEX_NAME,
                        H5_ITER_INC, i, NULL, 0, H5P_LINK_ACCESS_DEFAULT);
                char *link_name_c = new char[len_name + 1];
                H5Lget_name_by_idx(base, ".", H5_INDEX_NAME,
                        H5_ITER_INC, i, link_name_c, len_name + 1, H5P_LINK_ACCESS_DEFAULT);

                currentEntryName = std::string(link_name_c) + std::string("/");
                currentBaseName += currentEntryName;
                delete[] link_name_c;
            }

            if (obj_info.type == H5O_TYPE_GROUP)
            {
                hid_t group_id = H5Oopen_by_idx(base, ".", H5_INDEX_NAME, H5_ITER_INC, i, H5P_DEFAULT);
                getEntriesInternal(group_id, baseGroup, currentBaseName, param);
                H5Oclose(group_id);
            }

            if (obj_info.type == H5O_TYPE_DATASET)
            {
                if (param->entries)
                    param->entries[param->count].name = currentEntryName;

                param->count++;
            }
        }
    }
}
