/* Copyright (c) 2026 bhgv
 * EN: Original-Styx fixture serves actual source-built Dis assets alongside independent Unibus records.
 * RU: Исходный Styx-fixture выдаёт настоящие source-built Dis assets рядом с независимыми записями Unibus.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned char *images[2]; /* Native immutable fixture images until server join. */
static size_t lengths[2]; /* Exact bytes advertised in original Styx metadata. */


/* EN/RU: Add readonly app/dependency nodes from actual compiled fixture files before server starts.
 * Добавить readonly app/dependency nodes из настоящих compiled files до запуска server.
 */
void fixtureimagesinit(Styxserver *server)
{
    const char *names[] = {"app.dis", "dep.dis"}; /* Independent ordinary original-Styx file names. */
    for (unsigned i = 0; i < 2; i++)
    {
        FILE *input = fopen(names[i], "rb"); /* Fixture-native acquisition only, never guest fallback. */
        if (!input || fseek(input, 0, SEEK_END))
        {
            exit(3);
        }
        long length = ftell(input); /* Validate exact positive finite module extent. */
        if (length <= 0 || length >= 8 * 1024 * 1024 || fseek(input, 0, SEEK_SET))
        {
            exit(3);
        }
        lengths[i] = (size_t)length;
        images[i] = malloc(lengths[i]);
        if (!images[i] || fread(images[i], 1, lengths[i], input) != lengths[i] || fclose(input))
        {
            exit(3);
        }
        Styxfile *file = styxaddfile(server, Qroot, 8 + i, (char *)names[i], 0444, "fixture"); /* Original fid/qid implementation. */
        if (!file)
        {
            exit(3);
        }
        file->d.length = lengths[i];
    }
}


/* EN/RU: Copy at most three actual image bytes at the original remote offset, no fabricated module result.
 * Скопировать до трёх настоящих image-bytes по исходному remote-offset, без подмены module result.
 */
char *fixtureimagesread(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path < 8 || qid.path > 9 || offset < 0)
    {
        return Ebadarg;
    }
    unsigned index = qid.path - 8; /* Exact readonly fixture node. */
    if ((uvlong)offset > lengths[index])
    {
        return Ebadarg;
    }
    if (*count > lengths[index] - (size_t)offset)
    {
        *count = lengths[index] - (size_t)offset;
    }
    if (*count > 3)
    {
        *count = 3;
    }
    memmove(bytes, images[index] + offset, *count);
    return nil;
}


/* EN/RU: Consume immutable image storage only after original server and every read callback retires.
 * Освободить immutable image storage после завершения original server и всех read-callbacks.
 */
void fixtureimagesfree(void)
{
    for (unsigned i = 0; i < 2; i++)
    {
        free(images[i]);
    }
}
