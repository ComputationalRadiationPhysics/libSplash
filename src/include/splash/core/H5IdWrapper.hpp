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

#include "splash/core/splashMacros.hpp"

#include <hdf5.h>


namespace splash
{
    namespace policies
    {
        template<typename T>
        struct NoCopy
        {
            static T copy(const T&)
            {
                // Make it depend on template parameter
                SPLASH_UNUSED static char copyNotAllowed[sizeof(T) ? -1 : 0];
            }

            static bool release(const T&)
            {
                return true;
            }
        };

        template<typename T>
        struct RefCounted
        {
            RefCounted(): refCt_(new unsigned)
            {
                *refCt_ = 1;
            }

            T copy(const T& obj)
            {
                ++*refCt_;
                return obj;
            }

            bool release(const T&)
            {
                if(!--*refCt_)
                {
                    delete refCt_;
                    refCt_ = NULL;
                    return true;
                }
                return false;
            }

            friend void swap(RefCounted& lhs, RefCounted& rhs)
            {
                std::swap(lhs.refCt_, rhs.refCt_);
            }
        private:
            unsigned* refCt_;
        };
    }

    /** RAII wrapper for a hid_t (HDF5 identifier)
     *  Calls T_CloseMethod on destruction and allows implicit conversion
     *  Calls T_DestructionPolicy::copy on copy which should return the id to store
     *  Calls T_DestructionPolicy::release on close/destruction which should return true if the handle should be closed
     */
    template<H5_DLL herr_t (*T_CloseMethod)(hid_t), template<class> class T_DestructionPolicy>
    struct H5IdWrapper: public T_DestructionPolicy<hid_t>
    {
        typedef T_DestructionPolicy<hid_t> DestructionPolicy;

        H5IdWrapper(): id_(-1){}
        explicit H5IdWrapper(hid_t id): id_(id){}
        H5IdWrapper(const H5IdWrapper& rhs): DestructionPolicy(rhs)
        {
            id_ = DestructionPolicy::copy(rhs.id_);
        }

        H5IdWrapper& operator=(const H5IdWrapper& rhs)
        {
            H5IdWrapper tmp(rhs);
            swap(*this, tmp);
            return *this;
        }

        ~H5IdWrapper()
        {
            if(DestructionPolicy::release(id_))
            {
                if(id_ >= 0)
                    T_CloseMethod(id_);
            }
        }

        /** Sets the id to invalid closing it if required */
        void close()
        {
            reset(-1);
        }

        /** Resets the id closing the current one if required */
        void reset(hid_t id)
        {
            H5IdWrapper tmp(id);
            swap(*this, tmp);
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
        operator bool() const { return id_ >= 0; }

        friend void swap(H5IdWrapper& lhs, H5IdWrapper& rhs)
        {
            std::swap(lhs.id_, rhs.id_);
            std::swap(static_cast<DestructionPolicy&>(lhs), static_cast<DestructionPolicy&>(rhs));
        }

    private:
        hid_t id_;
    };

    /** Wrapper for HDF5 attribute identifiers */
    typedef H5IdWrapper<H5Aclose, policies::NoCopy> H5AttributeId;
    typedef H5IdWrapper<H5Aclose, policies::RefCounted> H5AttributeIdRefCt;
    /** Wrapper for HDF5 space identifiers */
    typedef H5IdWrapper<H5Sclose, policies::NoCopy> H5DataspaceId;
    typedef H5IdWrapper<H5Sclose, policies::RefCounted> H5DataspaceIdRefCt;
    /** Wrapper for HDF5 type identifiers */
    typedef H5IdWrapper<H5Tclose, policies::NoCopy> H5TypeId;
    typedef H5IdWrapper<H5Tclose, policies::RefCounted> H5TypeIdRefCt;
    /** Wrapper for HDF5 type identifiers */
    typedef H5IdWrapper<H5Oclose, policies::NoCopy> H5ObjectId;
    typedef H5IdWrapper<H5Oclose, policies::RefCounted> H5ObjectIdRefCt;

    template<H5_DLL herr_t (*T_CloseMethod)(hid_t), template<class> class T_DestructionPolicy>
    inline bool operator==(const H5IdWrapper<T_CloseMethod, T_DestructionPolicy>& lhs, const H5IdWrapper<T_CloseMethod, T_DestructionPolicy>& rhs)
    {
        return static_cast<hid_t>(lhs) == static_cast<hid_t>(rhs);
    }
    template<H5_DLL herr_t (*T_CloseMethod)(hid_t), template<class> class T_DestructionPolicy>
    inline bool operator!=(const H5IdWrapper<T_CloseMethod, T_DestructionPolicy>& lhs, const H5IdWrapper<T_CloseMethod, T_DestructionPolicy>& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif /* H5ID_WRAPPER_HPP */
