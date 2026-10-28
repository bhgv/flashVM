/* Test production Sys guards with the actual generated NC64 frame and heap headers. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char exNilref[] = "nil";    /* Exact mock VM error identity for null/H inputs. */
char exBounds[] = "bounds"; /* Exact mock VM error identity for wrapped spans. */
char exHeap[] = "heap";     /* Exact mock VM error identity for invalid owners. */
static char *expected;      /* Borrowed error expected by this isolated test process. */


/* Stop the isolated case through the real guard's error call; unexpected errors fail. */
void error(char *message)
{
    if (expected && !strcmp(expected, message))
    {
        exit(0);
    }
    fprintf(stderr, "unexpected Sys guard error: %s\n", message);
    exit(1);
}


/* Permit sanitizer use where process-based leak discovery is unavailable. */
int __lsan_is_turned_off(void)
{
    return 1;
}


/* Check all packed frame/result offsets, then clobber original arguments like a callback.
 * No heap is borrowed. Return one only when the captured operands/result remain exact. */
static int packed_frames(void)
{
    unsigned int frame_offset;
    unsigned int result_offset;
    for (frame_offset = 0; frame_offset < sizeof(void *); frame_offset++)
    {
        for (result_offset = 0; result_offset < sizeof(void *); result_offset++)
        {
            uchar bytes[sizeof(F_Sys_read) + sizeof(void *)];
            uchar result[3 * sizeof(WORD)];
            uchar wanted[sizeof(result)];
            F_Sys_read source;
            F_Sys_read captured;
            WORD count = 42;
            memset(&source, 0, sizeof(source));
            memset(result, 0xa5, sizeof(result));
            memmove(wanted, result, sizeof(result));
            source.n = 17;
            source.ret = (WORD *)(result + result_offset);
            memmove(bytes + frame_offset, &source, sizeof(source));
            dvm_sys_capture(
                bytes + frame_offset, &captured, sizeof(captured), offsetof(F_Sys_read, ret), sizeof(*captured.ret));
            memset(bytes, 0xa7, sizeof(bytes));
            memmove(captured.ret, &count, sizeof(count));
            memmove(wanted + result_offset, &count, sizeof(count));
            if (captured.n != 17 || memcmp(wanted, result, sizeof(result)))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* Select one isolated valid/error case; production guards raise before unsafe memory access.
 * Actual frame/Heap layouts come from the matched runtime headers, not modeled structs. */
int main(int argc, char **argv)
{
    F_Sys_read source;
    F_Sys_read captured;
    WORD result = 0;
    void *frame = &source;
    void *owner = H;
    void *owners[2];
    Heap *heap = NULL;
    size_t offset = offsetof(F_Sys_read, ret);
    size_t extent = sizeof(result);
    int mode = argc == 2 ? atoi(argv[1]) : 0;
    memset(&source, 0, sizeof(source));
    source.ret = &result;
    if (mode >= 16 && mode <= 18)
    {
        F_Sys_stat stat_frame;
        F_Sys_fstat fstat_frame;
        F_Sys_dirread directory_frame;
        memset(&stat_frame, 0, sizeof(stat_frame));
        memset(&fstat_frame, 0, sizeof(fstat_frame));
        memset(&directory_frame, 0, sizeof(directory_frame));
        expected = exBounds;
        /* A complete tuple wraps even though its leading WORD alone would fit. */
        if (mode == 16)
        {
            stat_frame.ret = (void *)(UINTPTR_MAX - sizeof(WORD));
            dvm_sys_capture(
                &stat_frame, &stat_frame, sizeof(stat_frame), offsetof(F_Sys_stat, ret), sizeof(*stat_frame.ret));
        }
        else if (mode == 17)
        {
            fstat_frame.ret = (void *)(UINTPTR_MAX - sizeof(WORD));
            dvm_sys_capture(
                &fstat_frame, &fstat_frame, sizeof(fstat_frame), offsetof(F_Sys_fstat, ret), sizeof(*fstat_frame.ret));
        }
        else
        {
            directory_frame.ret = (void *)(UINTPTR_MAX - sizeof(WORD));
            dvm_sys_capture(&directory_frame,
                            &directory_frame,
                            sizeof(directory_frame),
                            offsetof(F_Sys_dirread, ret),
                            sizeof(*directory_frame.ret));
        }
        fprintf(stderr, "Sys guard admitted a wrapping aggregate result\n");
        return 1;
    }
    if (!mode)
    {
        if (!packed_frames())
        {
            return 1;
        }
        puts("PASS Sys capture: real NC64 frames, every packed offset, original result after clobber");
        return 0;
    }
    if (mode == 1 || mode == 2 || mode == 4 || mode == 5 || mode == 9)
    {
        expected = exNilref;
    }
    else if (mode == 3 || mode == 6 || mode == 7 || mode == 8)
    {
        expected = exBounds;
    }
    else if (mode != 10 && mode != 13)
    {
        expected = exHeap;
    }
    if (mode == 1)
    {
        frame = NULL;
    }
    if (mode == 2)
    {
        frame = H;
    }
    if (mode == 3)
    {
        frame = (void *)(UINTPTR_MAX - 1);
    }
    if (mode == 4 || mode == 5 || mode == 6)
    {
        source.ret = mode == 4 ? NULL : mode == 5 ? H : (void *)(UINTPTR_MAX - 1);
    }
    if (mode == 7)
    {
        offset = sizeof(source);
    }
    if (mode == 8)
    {
        extent = 0;
    }
    if (mode < 9)
    {
        dvm_sys_capture(frame, &captured, sizeof(captured), offset, extent);
    }
    else
    {
        if (mode == 9)
        {
            owner = NULL;
        }
        else if (mode >= 11 && mode <= 15)
        {
            heap = calloc(1, sizeof(*heap) + sizeof(void *));
            if (!heap)
            {
                return 1;
            }
            heap->ref = mode == 13 ? 2 : mode == 11 ? 0 : 1;
            owner = H2D(void *, heap);
            if (mode == 14)
            {
                owner = (void *)1;
            }
            if (mode == 15)
            {
                owner = (uchar *)owner + 1;
            }
        }
        owner = dvm_sys_owner(&owner);
        owners[0] = owner;
        owners[1] = owner;
        dvm_sys_owners(owners, 2);
        free(heap);
        if (!expected)
        {
            return 0;
        }
    }
    fprintf(stderr, "Sys guard did not raise expected error for case %d\n", mode);
    return 1;
}
