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

#ifndef LOGGING_HPP
#define LOGGING_HPP

#ifdef __cplusplus
# define EXTERN extern "C"
#else
# define EXTERN extern
#endif

namespace splash
{
    /**
     * parses environment variables and sets internal configuration
     */
    EXTERN void parseEnvVars(void);

    /**
     * sets the MPI rank to be used for log messages
     *
     * @param rank MPI rank
     */
    EXTERN void setLogMpiRank(int rank);

    /**
     * writes a log message for a given log level
     *
     * @param level required log level
     * @param fmt format string (like printf)
     * @param ... arguments to \p fmt
     */
    EXTERN void log_msg(int level, const char *fmt, ...);
}

#endif /* LOGGING_HPP */
