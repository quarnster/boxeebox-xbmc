#include <osal.h>
#include <stdarg.h>

void * os_map_io_to_mem_cache(
    unsigned long base_address,
    unsigned long size){}

void * os_map_io_to_mem_nocache(
    unsigned long base_address,
    unsigned long size){}

void os_unmap_io_from_mem(
    void * virt_addr,
    unsigned long size){}

void _os_info( TCHAR *szFormat, ... ) {}
