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
static unsigned char *images[3]; /* Native immutable fixture images until server join. */
static unsigned char launch_frame[128]; /* Encoded fixture request, shared read-only. */
static int require_submission; /* In routed tests the caller must post before launch is readable. */
static int owner_requested; /* One owner execution request was accepted. */
static int submitted; /* Exactly one verified request was accepted by the original server. */
static size_t launch_size; /* Complete frame extent exposed by the original server. */
static size_t lengths[3]; /* Exact bytes advertised in original Styx metadata. */


/* EN/RU: Add readonly app/dependency nodes from actual compiled fixture files before server starts.
 * Добавить readonly app/dependency nodes из настоящих compiled files до запуска server.
 */
void fixtureimagesinit(Styxserver *server)
{
    const char *names[] = {"app.dis", "dep.dis", "owner/owner.dis"}; /* Independent ordinary original-Styx file names. */
    for (unsigned i = 0; i < 3; i++)
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
        Styxfile *file = styxaddfile(server, Qroot, i == 2 ? 19 : 8 + i, (char *)(i == 2 ? "owner.dis" : names[i]), 0444, "fixture"); /* Original fid/qid implementation. */
        if (!file)
        {
            exit(3);
        }
        file->d.length = lengths[i];
    }
    const unsigned char arguments[] = {'r', 0, 'n'};
    require_submission = getenv("DVM_TEST_TERMINAL_SUBMIT") != NULL;
    int custom = getenv("DVM_TEST_TERMINAL_CUSTOM") != NULL;
    const FVM_SBinding binding = {custom ? 23 : 17, 1, sizeof("disvm.launch") - 1, "disvm.launch"};
    unsigned char declaration[64];
    size_t declared;
    size_t encoded;
    Styxfile *request = styxaddfile(server, Qroot, 10, custom ? "job-request" : "launch", 0444, "fixture");
    if (!request || fvm_sbindings_payload_write(&binding, 1, declaration, sizeof(declaration),
            &declared) != FVM_S_OK ||
        fvm_srecord_write(0, declaration, declared, launch_frame, sizeof(launch_frame),
            &launch_size) != FVM_S_OK ||
        fvm_srecord_write(binding.tag, arguments, sizeof(arguments), launch_frame + launch_size,
            sizeof(launch_frame) - launch_size, &encoded) != FVM_S_OK)
    {
        exit(3);
    }
    launch_size += encoded;
    request->d.length = launch_size;
    if (require_submission && (!styxaddfile(server, Qroot, 11, "submit", 0222, "fixture") ||
        !styxaddfile(server, Qroot, 20, "execute", 0222, "fixture")))
    {
        exit(3);
    }
}


/* EN/RU: Copy at most three actual image bytes at the original remote offset, no fabricated module result.
 * Скопировать до трёх настоящих image-bytes по исходному remote-offset, без подмены module result.
 */
char *fixtureimagesread(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if ((qid.path != 19 && (qid.path < 8 || qid.path > 10)) || offset < 0 ||
        (qid.path == 10 && require_submission && !submitted))
    {
        return Ebadarg;
    }
    unsigned index = qid.path == 19 ? 2 : qid.path - 8; /* Exact readonly fixture node. */
    const unsigned char *source = qid.path == 10 ? launch_frame : images[index];
    size_t length = qid.path == 10 ? launch_size : lengths[index];
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
    for (unsigned i = 0; i < 3; i++)
    {
        free(images[i]);
    }
}


// EN/RU: Accept exactly one complete launch frame, before exposing it to the dispatcher.
// Принять ровно один полный кадр запуска перед выдачей диспетчеру.
char *fixturelaunchwrite(char *bytes, ulong *count, vlong offset)
{
    FVM_SRecord record;
    size_t consumed;
    if (!require_submission || offset != 0 ||
        fvm_srecord_read(bytes, *count, 128, &record, &consumed) != FVM_S_OK ||
        consumed != *count || record.tag != 17)
    {
        return Ebadarg;
    }
    if (submitted || record.size != 3 ||
        record.payload[0] != 'r' || record.payload[1] != 0 || record.payload[2] != 'n')
    {
        return Ebadarg;
    }
    size_t binding_size; /* Retain the existing tag-zero binding header, replace staged launch bytes. */
    FVM_SRecord binding;
    if (fvm_srecord_read(launch_frame, launch_size, 128, &binding, &binding_size) != FVM_S_OK ||
        *count > sizeof(launch_frame) - binding_size)
    {
        return Ebadarg;
    }
    memmove(launch_frame + binding_size, bytes, *count);
    launch_size = binding_size + *count;
    submitted = 1;
    return nil;
}


// EN/RU: Accept exactly one empty request on the owner's dedicated command endpoint.
// Принять один пустой запрос на отдельном узле команды владельца.
char *fixtureownerwrite(char *bytes, ulong *count, vlong offset)
{
    FVM_SRecord record;
    size_t consumed;
    if (owner_requested || offset != 0 ||
        fvm_srecord_read(bytes, *count, 128, &record, &consumed) != FVM_S_OK ||
        consumed != *count || record.tag != 17 || record.size)
    {
        return Ebadarg;
    }
    owner_requested = 1;
    return nil;
}


// EN/RU: Report the owner's completed request admission after the server drains.
// Сообщить о принятом запросе владельца после завершения сервера.
int fixtureownerrequested(void)
{
    return owner_requested;
}
