/**
 * Copyright 2013 Felix Schmitt, Axel Huebl
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

#ifndef VERSION_HPP
#define	VERSION_HPP

/** the splash version reflects the changes in API */
#define SPLASH_VERSION_MAJOR 1
#define SPLASH_VERSION_MINOR 0
#define SPLASH_VERSION_PATCH 0

/** we can always handle files from the same major release
 *  changes in the minor number have to be backwards compatible
 */
#define SPLASH_FILE_FORMAT_MAJOR 1
#define SPLASH_FILE_FORMAT_MINOR 0

#endif	/* VERSION_HPP */
