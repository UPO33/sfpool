#include "sfpool.h"

#define WORD_SIZE ((size_t) - 1)

inline size_t round_size (size_t size,size_t align)
{
    if(size < align)
    {
        size = align;
        return size;
    }

    size_t mod = size % align;

    if(mod != 0)
    {
        size += align - mod;
    }
    
	return size;
}
