#ifndef MISC_H
#define MISC_H

static inline size_t align_offset(size_t offset, size_t align)
{
        return (offset + (align - 1)) & ~(align - 1);
}

#endif