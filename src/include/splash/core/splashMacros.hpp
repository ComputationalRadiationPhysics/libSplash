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

#ifndef SPLASH_MACROS_HPP
#define SPLASH_MACROS_HPP

//! @TODO Add macro defines for ICC, XLC, PGI

// Mark a function as deprecated: `SPLASH_DEPRECATED("Use bar instead") void foo();`
#ifdef __clang__
#   define SPLASH_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(__GNUC__)
#   define SPLASH_DEPRECATED(msg) __attribute__((deprecated))
#elif defined(_MSC_VER)
#   define SPLASH_DEPRECATED(msg) __declspec(deprecated)
#else
#   define SPLASH_DEPRECATED(msg)
#endif

// Suppress a "unused variable" warning: `SPLASH_UNUSED static int i = 42;`
#ifdef __clang__
#   define SPLASH_UNUSED __attribute__((unused))
#elif defined(__GNUC__)
#   define SPLASH_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#   define SPLASH_UNUSED
#elif defined(__xlc__)
#   define SPLASH_UNUSED __attribute__((unused))
#elif defined(__INTEL_COMPILER)
#   define SPLASH_UNUSED
#elif defined(__PGI)
#   define SPLASH_UNUSED
#else
#   define SPLASH_UNUSED
#endif

#endif /* SPLASH_MACROS_HPP */
