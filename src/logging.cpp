#include <stdio.h>
#include <cstdlib>
#include <cstdarg>
#include "core/logging.hpp"

namespace DCollector
{
    static int verbosity_level = 0;

    void parseEnvVars(void)
    {
        char *verbosity = getenv("SPLASH_VERBOSE");
        if (verbosity != NULL)
        {
            verbosity_level = atoi(verbosity);
            log_msg(1, "Setting verbosity level to %d\n", verbosity_level);
        }
    }

    void log_msg(int level, const char *fmt, ...)
    {
        va_list argp;
        
        if (level <= verbosity_level)
        {
            fprintf(stderr, "[SPLASH_LOG] ");
            
            va_start(argp, fmt);
            vfprintf(stderr, fmt, argp);
            va_end(argp);

            fprintf(stderr, "\n");
        }
    }

}
