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

#endif /* SPLASH_MACROS_HPP */
