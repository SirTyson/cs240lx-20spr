#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int 
printk (const char* str, ...)
{
    va_list args;
    va_start(args, str);
        printf("PI:");
        int res = vfprintf(stdout, str, args);
        fflush(stdout);
    va_end(args);
    return res;

}