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

void createMessage(char *msgbuff, int fileno, unsigned long size) {
    *((int *)msgbuff) = fileno;
    *((unsigned long *)(msgbuff + HEADER_FNO)) = size;  // enclose (msgbuff + HEADER_FNO) is important
    printf("message created! filenumber: %d, size: %lu\n", fileno, size);
}

int getFilenumber(char *msgbuff) {
    return *((int *)msgbuff); 
}

unsigned long getSizeValue(char *msgbuff) {
    return *((unsigned long *)(msgbuff + HEADER_FNO)); 
}
