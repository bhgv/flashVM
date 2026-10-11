#ifndef FVM_TEST_IMAGE_MEMORY_H
#define FVM_TEST_IMAGE_MEMORY_H
#include <stddef.h>
#include <string.h>
/* Fixed-capacity test streams exercise failure paths without disk/OS APIs. */
typedef struct TestImageBytes
{
    unsigned char data[65536];
    size_t size, position;
} TestImageBytes;
#endif
