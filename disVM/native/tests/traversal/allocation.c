/* Copyright (c) 2026 bhgv
 * EN: Actual native allocation refusals leave active histories balanced and retire every partial packet/import buffer.
 * RU: Настоящие allocation refusals сохраняют balanced history и освобождают partial packet/import buffers.
 */
#include "packet.h"
#include <stdio.h>
#include <stdlib.h>
static volatile unsigned refusal; /* Exact next native allocation ordinal to fail; zero disables injection. */
static volatile size_t live; /* Actual owned library allocations, excluding libc-private storage. */


/* EN/RU: Real malloc/calloc/free, linker-owned entry points preserve production allocator behavior.
 * Настоящие malloc/calloc/free; linker entry points сохраняют production allocator behavior.
 */
extern void *__real_malloc(size_t size);
extern void *__real_calloc(size_t count, size_t size);
extern void __real_free(void *bytes);


/* EN/RU: Fail one actual allocation or count its resulting owned native block; size is unchanged.
 * Отказать одной actual allocation либо учесть native block, size без изменений.
 */
void *__wrap_malloc(size_t size)
{
    if (refusal && !--refusal)
    {
        return NULL;
    }
    void *bytes = __real_malloc(size);
    live += bytes != NULL;
    return bytes;
}


/* EN/RU: Fail one actual calloc or count the owned zeroed block; count/size forwarded exactly.
 * Отказать actual calloc либо учесть owned zeroed block, count/size без изменений.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (refusal && !--refusal)
    {
        return NULL;
    }
    void *bytes = __real_calloc(count, size);
    live += bytes != NULL;
    return bytes;
}


/* EN/RU: Consume only library-owned allocation/NULL; count once before production free.
 * Поглотить library-owned allocation/NULL; учесть один раз до production free.
 */
void __wrap_free(void *bytes)
{
    if (bytes)
    {
        if (!live)
        {
            fputs("walk allocation: freeing unowned native block\n", stderr);
            exit(2);
        }
        live--;
    }
    __real_free(bytes);
}


/* EN/RU: Explicit release-build check with borrowed diagnostic and exact failure code.
 * Явный release-build check с borrowed diagnostic и exact failure code.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "walk allocation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Refuse deep-stack growth, imported strings/growth and both packet allocations, then prove successful recovery.
 * Отказать deep-stack growth/import strings/growth и обеим packet allocations, затем проверить recovery.
 */
int main(void)
{
    unsigned long domain[4] = {4, 3, 2, 1};
    WalkEntity entity;
    WalkScope held[9] = {{0}};
    check(walk_domain(domain) && walk_entity(&entity), "domain");
    for (unsigned i = 0; i < 8; i++)
    {
        check(walk_enter(&held[i], &entity, 0, i, WALK_INFO, "file", "") == WALK_OK, "inline enter");
    }
    refusal = 1;
    check(walk_enter(&held[8], &entity, 0, 8, WALK_INFO, "file", "") == WALK_NOMEM &&
        !held[8].active && walk_depth() == 8 && !live, "growth OOM changed active state");
    check(walk_enter(&held[8], &entity, 0, 8, WALK_INFO, "file", "") == WALK_OK && live == 1, "growth recovery");
    unsigned char snapshot[4096];
    size_t length = 0;
    check(walk_export(snapshot, sizeof(snapshot), &length), "nine-state snapshot");
    for (unsigned i = 9; i; i--)
    {
        check(walk_leave(&held[i - 1]), "unwind original scopes");
    }
    check(!walk_depth() && !live, "grown stack retained allocation");
    for (unsigned failure = 1; failure <= 2; failure++)
    {
        refusal = failure;
        WalkScope imported = {0};
        check(walk_import(&imported, snapshot, length) == WALK_NOMEM && !refusal &&
            !imported.active && !walk_depth() && !live, "import allocation failure did not roll back");
    }
    const unsigned char frame[] = {9, 0, 0, 0, 108, 1, 0, 2, 0}; /* Original Tflush, not an allocator substitute. */
    for (unsigned failure = 1; failure <= 2; failure++)
    {
        refusal = failure;
        check(!walk_packet_write(WALK_PACKET_REQUEST, 39, NULL, frame, sizeof(frame), &length) &&
            !refusal && !live && !walk_depth(), "packet allocation failure leaked partial state");
    }
    void *packet = walk_packet_write(WALK_PACKET_REQUEST, 39, NULL, frame, sizeof(frame), &length);
    check(packet && live == 1, "packet allocation recovery");
    free(packet);
    check(!live && !walk_depth(), "native allocation ledger did not drain");
    puts("PASS actual deep-stack/import/packet allocation refusals and complete native ownership drain");
    return 0;
}
