#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "TinyFile.h"

size_t fread_buf( void* ptr, size_t size, FILE* stream)
{
    return fread( ptr, 1, size, stream);
}


size_t fwrite_buf( void const* ptr, size_t size, FILE* stream)
{
    return fwrite( ptr, 1, size, stream);
}
