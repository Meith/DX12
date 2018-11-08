#ifndef MISC_H
#define MISC_H

#include <time.h>
#include <stdio.h>
#include <stdarg.h>

static inline size_t align_offset(size_t offset, size_t align)
{
        return (offset + (align - 1)) & ~(align - 1);
}

static inline time_t time_in_secs()
{
        time_t sec;
        sec = time(NULL);

        return sec;
}

static inline void debug_print(const char *in_str, ...)
{
        va_list arg_list;
        va_start(arg_list, in_str);

        char out_str[1024];
        vsprintf(out_str, in_str, arg_list);
        
        OutputDebugString(out_str);

        va_end(arg_list);
}

#endif