/* Copyright (c) 2026 bhgv
 * EN: Original-Styx fixture serves actual source-built Dis assets alongside independent Unibus records.
 * RU: Исходный Styx-fixture выдаёт настоящие source-built Dis assets рядом с независимыми записями Unibus.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/sbindings.h"
static unsigned char *images[2]; /* Native immutable fixture images until server join. */
static unsigned char launch_frame[128]; /* Encoded fixture request, shared read-only. */
static size_t launch_size; /* Complete frame extent exposed by the original server. */
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
    const unsigned char arguments[] = {'r', 0, 'n'};
    const FVM_SBinding binding = {17, 1, sizeof("test.launch") - 1, "test.launch"};
    unsigned char declaration[64];
    size_t declared;
    size_t encoded;
    Styxfile *request = styxaddfile(server, Qroot, 10, "launch", 0444, "fixture");
    if (!request || fvm_sbindings_payload_write(&binding, 1, declaration, sizeof(declaration),
            &declared) != FVM_S_OK ||
        fvm_srecord_write(0, declaration, declared, launch_frame, sizeof(launch_frame),
            &launch_size) != FVM_S_OK ||
        fvm_srecord_write(17, arguments, sizeof(arguments), launch_frame + launch_size,
            sizeof(launch_frame) - launch_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    launch_size += encoded;
    request->d.length = launch_size;
}


/* EN/RU: Copy at most three actual image bytes at the original remote offset, no fabricated module result.
 * Скопировать до трёх настоящих image-bytes по исходному remote-offset, без подмены module result.
 */
char *fixtureimagesread(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path < 8 || qid.path > 10 || offset < 0)
    {
        return Ebadarg;
    }
    unsigned index = qid.path - 8; /* Exact readonly fixture node. */
    const unsigned char *source = index == 2 ? launch_frame : images[index];
    size_t length = index == 2 ? launch_size : lengths[index];
    if ((uvlong)offset > length)
    {
        return Ebadarg;
    }
    if (*count > length - (size_t)offset)
    {
        *count = length - (size_t)offset;
    }
    if (*count > 3)
    {
        *count = 3;
    }
    memmove(bytes, source + offset, *count);
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
