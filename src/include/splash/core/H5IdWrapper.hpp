/**
 * Copyright 2016 Alexander Grund
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

#ifndef H5ID_WRAPPER_HPP
#define H5ID_WRAPPER_HPP

#include <hdf5.h>

namespace splash
{
    /** RAII wrapper for a hid_t (HDF5 identifier)
     *  Calls T_CloseMethod on destruction and allows implicit conversion */
    template<H5_DLL herr_t (*T_CloseMethod)(hid_t)>
    struct H5IdWrapper
    {
        H5IdWrapper(): id_(-1){}
        H5IdWrapper(hid_t id): id_(id){}
        ~H5IdWrapper()
        {
            close();
        }

        /** Closes the id, resetting it to invalid */
        void close()
        {
            if(id_ < 0)
                return;
            T_CloseMethod(id_);
            id_ = -1;
        }

        /** Closes the old id and sets a new one */
        void reset(hid_t id)
        {
            close();
            id_ = id;
        }

        /** Resets the internal id to invalid without closing it.
         *  @return Old id */
        hid_t release()
        {
            hid_t id = id_;
            id_ = -1;
            return id;
        }

        operator hid_t() const { return id_; }
        bool operator!() const { return id_ < 0; }

    private:
        // Don't copy!
        H5IdWrapper(const H5IdWrapper&);
        H5IdWrapper& operator=(const H5IdWrapper&);
        hid_t id_;
    };

    /** Wrapper for HDF5 attribute identifiers */
    typedef H5IdWrapper<H5Aclose> H5AttributeId;
    /** Wrapper for HDF5 space identifiers */
    typedef H5IdWrapper<H5Sclose> H5DataspaceId;
    /** Wrapper for HDF5 type identifiers */
    typedef H5IdWrapper<H5Tclose> H5TypeId;
    /** Wrapper for HDF5 type identifiers */
    typedef H5IdWrapper<H5Oclose> H5ObjectId;
}

#endif /* H5ID_WRAPPER_HPP */
