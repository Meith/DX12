#ifndef MISC_H
#define MISC_H

#include <time.h>

static inline size_t align_offset(size_t offset, size_t align)
{
        return (offset + (align - 1)) & ~(align - 1);
}

static inline float time_in_secs()
{
        time_t sec;
        sec = time(NULL);

        return sec / 3600.0f;
}

#endif