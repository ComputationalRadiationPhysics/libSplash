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
 


#ifndef DATAMERGER_H
#define	DATAMERGER_H

namespace DCollector
{
    class DataMerger
    {
    public:
        /**
         * Merges single hdf5 files to one file.
         *
         * @param filename hdf5 filename without extensions must be the same for all mpi processes
         * @param enableCompression enables writing compressed data
         */
        virtual void merge(const char* filename,
                bool enableCompression) = 0;
    };
}

#endif	/* DATAMERGER_H */

