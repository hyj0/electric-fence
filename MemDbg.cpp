//
// Created by dell-pc on 2016/12/10.
//

#include "MemDbg.h"

MemoryInfo g_memoryInfoHeadInstance;

extern C_LINKAGE void *
        malloc(size_t size);

extern C_LINKAGE void
free(void * address)
{
    MemDbg::getInstance().deleteMemory(address);
}

extern C_LINKAGE void *
realloc(void * oldBuffer, size_t newSize)
{
    void *newBuffer = malloc(newSize);
    if (oldBuffer) {
        size_t size = MemDbg::getInstance().getAllocSize(oldBuffer);
        if (newSize < size) {
            size = newSize;
        }
        if (size > 0) {
            memcpy(newBuffer, oldBuffer, size);
        }
        free(oldBuffer);
    }
    return newBuffer;
}

extern C_LINKAGE void *
malloc(size_t size)
{
    void *address = MemDbg::getInstance().createMemory(size);
    return address;
}

extern C_LINKAGE char *
strdup(const char *s1)
{
    if (!s1) return 0;
    char *s2 = (char *) malloc(strlen(s1) + 1);

    if (!s2) {
        errno = ENOMEM;
        return 0;
    }

    return strcpy(s2, s1);
}

extern C_LINKAGE char *
strndup(const char *s1, size_t n)
{
    if (!s1) return 0;
    int complete_size = n;  /* includes terminating null */
    for (int i = 0; i < n - 1; i++) {
        if (!s1[i]) {
            complete_size = i + 2;
            break;
        }
    }
    char *s2 = (char *) malloc(complete_size);

    if (!s2) {
        errno = ENOMEM;
        return 0;
    }

    strncpy(s2, s1, complete_size - 1);
    s2[complete_size - 1] = '\0';

    return s2;
}

extern C_LINKAGE void *
calloc(size_t nelem, size_t elsize)
{
    size_t	size = nelem * elsize;
    void * allocation;

    allocation = malloc(size);
    memset(allocation, 0, size);

    return allocation;
}

/*
 * This will catch more bugs if you remove the page alignment, but it
 * will break some software.
 */
extern C_LINKAGE void *
valloc (size_t size)
{
    //with no test
    void * allocation;
    allocation = malloc(size);

    return allocation;
}
