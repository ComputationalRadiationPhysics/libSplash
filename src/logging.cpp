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

#include <stdio.h>
#include <cstdlib>
#include <cstdarg>

#include "splash/core/logging.hpp"

namespace splash
{
    static int verbosity_level = 0;
    static int my_rank = 0;

    void parseEnvVars(void)
    {
        char *verbosity = getenv("SPLASH_VERBOSE");
        if (verbosity != NULL)
        {
            verbosity_level = atoi(verbosity);
            log_msg(1, "Setting verbosity level to %d\n", verbosity_level);
        }
    }
    
    void setLogMpiRank(int rank)
    {
        my_rank = rank;
    }

    void log_msg(int level, const char *fmt, ...)
    {
        va_list argp;
        
        if (level <= verbosity_level)
        {
            fprintf(stderr, "[SPLASH_LOG:%d] ", my_rank);
            
            va_start(argp, fmt);
            vfprintf(stderr, fmt, argp);
            va_end(argp);

            fprintf(stderr, "\n");
        }
    }

}
