/* Copyright (c) 2026 bhgv
 * EN: Original-Styx fixture serves actual source-built Dis assets alongside independent Unibus records.
 * RU: Исходный Styx-fixture выдаёт настоящие source-built Dis assets рядом с независимыми записями Unibus.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "flashvm/sbindings.h"
#include "../../../launch/program_record.h"
#include "launch_receive.h"
#include "images.h"
static const struct
{
    unsigned long qid; /* Published Styx identity for the asset. */
    const char *source; /* File in the test workspace. */
    const char *name; /* Name in the peer's export. */
} assets[] = { /* Immutable table in publication order. */
    {8, "app.dis", "app.dis"},
    {9, "dep.dis", "dep.dis"},
    {19, "owner/owner.dis", "owner.dis"},
    {22, "runexample.dis", "runexample.dis"},
    {23, "vfsctl.dis", "vfsctl.dis"}
};
static unsigned char *images[nelem(assets)]; /* Native immutable fixture images until server join. */
static unsigned char launch_frame[128]; /* Encoded fixture request, shared read-only. */
static int require_submission; /* In routed tests the caller must post before launch is readable. */
static int submitted; /* Exactly one verified request was accepted by the original server. */
static size_t launch_size; /* Complete frame extent exposed by the original server. */
static size_t lengths[nelem(assets)]; /* Exact bytes advertised in original Styx metadata. */


enum
{
    ProgramModule = 819, /* Directory of this client's loaded module. */
    ProgramRoot = 820,   /* Process registry within that module. */
    ProgramBase = 821,   /* Base process 41. */
    ProgramChild = 822,  /* Selected process 42. */
    ProgramChildFds = 823, /* Directory for replacement descriptors. */
    ProgramChildVfs = 825, /* Optional child namespace. */
    ProgramBaseVfs = 828, /* Fallback namespace. */
    ProgramPid = 833, /* Read-only PID of the selected process. */
    ProgramInvalid = 831 /* Process with an invalid .vfs node. */
};


/* EN/RU: Publish a process namespace and its ordinary environment file.
 * Опубликовать пространство процесса и обычный файл его окружения. */
static int publish_environment(Styxserver *server, Path process, Path vfs)
{
    Styxfile *file;
    if (!styxadddir(server, process, vfs, ".vfs", 0555, "fixture") ||
        !styxadddir(server, vfs, vfs + 1, "env", 0555, "fixture") ||
        !(file = styxaddfile(server, vfs + 1, vfs + 2, "CLIENT", 0444, "fixture")))
    {
        return 0;
    }
    file->d.length = 5;
    return 1;
}


/* EN/RU: Publish the granted loaded module tree and return its base process qid.
 * Опубликовать дерево выданной загрузки модуля и вернуть qid базового процесса. */
Path fixtureprograminit(Styxserver *server)
{
    if (!styxadddir(server, Qroot, ProgramModule, "loaded.dis", 0555, "fixture") ||
        !styxadddir(server, ProgramModule, ProgramRoot, ".prog", 0555, "fixture") ||
        !styxadddir(server, ProgramRoot, ProgramBase, "41", 0555, "fixture") ||
        !styxadddir(server, ProgramRoot, ProgramChild, "42", 0555, "fixture") ||
        !styxadddir(server, ProgramChild, ProgramChildFds, ".fd", 0555, "fixture") ||
        !styxaddfile(server, ProgramChild, ProgramPid, ".pid", 0444, "fixture") ||
        !publish_environment(server, ProgramBase, ProgramBaseVfs) ||
        !styxadddir(server, ProgramRoot, ProgramInvalid, "43", 0555, "fixture") ||
        !styxaddfile(server, ProgramInvalid, ProgramInvalid + 1, ".vfs", 0444, "fixture") ||
        (!getenv("DVM_TEST_TERMINAL_SCOPED_BASE") && !publish_environment(server, ProgramChild, ProgramChildVfs)))
    {
        _exit(3);
    }
    return ProgramBase;
}


/* EN/RU: Add readonly app/dependency nodes from actual compiled fixture files before server starts.
 * Добавить readonly app/dependency nodes из настоящих compiled files до запуска server.
 */
void fixtureimagesinit(Styxserver *server)
{
    for (unsigned i = 0; i < nelem(assets); i++)
    {
        FILE *input = fopen(assets[i].source, "rb"); /* Fixture-native acquisition only, never guest fallback. */
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
        Styxfile *file = styxaddfile(server, Qroot, assets[i].qid, (char *)assets[i].name, 0444, "fixture"); /* Original fid/qid implementation. */
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
        !styxaddfile(server, Qroot, 21, "execute-result", 0444, "fixture")))
    {
        exit(3);
    }
}


/* EN/RU: Copy at most three actual image bytes at the original remote offset, no fabricated module result.
 * Скопировать до трёх настоящих image-bytes по исходному remote-offset, без подмены module result.
 */
char *fixtureimagesread(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path == ProgramPid && offset >= 0)
    {
        *count = styxreadstr(offset, bytes, *count, "42\n");
        return nil;
    }
    if ((qid.path == ProgramChildVfs + 2 || qid.path == ProgramBaseVfs + 2) && offset >= 0)
    {
        *count = styxreadstr(offset, bytes, *count, qid.path == ProgramChildVfs + 2 ? "child" : "base!");
        return nil;
    }
    if (offset < 0 || (qid.path == 10 && require_submission && !submitted))
    {
        return Ebadarg;
    }
    const unsigned char *source = launch_frame;
    size_t length = launch_size;
    if (qid.path != 10)
    {
        unsigned index = 0;
        while (index < nelem(assets) && assets[index].qid != qid.path)
        {
            index++;
        }
        if (index == nelem(assets))
        {
            return Ebadarg;
        }
        source = images[index];
        length = lengths[index];
    }
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
    for (unsigned i = 0; i < nelem(assets); i++)
    {
        free(images[i]);
    }
}


/* EN/RU: Verify the guest's binary payload, including the embedded NUL.
 * Проверить бинарные данные гостя, включая встроенный NUL. */
static int accept_payload(void *state, const void *bytes, size_t size)
{
    USED(state);
    return size == 3 && !memcmp(bytes, "x\0y", size);
}

static StyxLaunchReceiver payload_receiver = {.tag = 17, .start_payload = accept_payload,
    .payload_limit = 3}; /* Independent payload submission after boot. */


// EN/RU: Accept exactly one complete launch frame, before exposing it to the dispatcher.
// Принять ровно один полный кадр запуска перед выдачей диспетчеру.
char *fixturelaunchwrite(char *bytes, ulong *count, vlong offset)
{
    if (submitted)
    {
        return offset >= 0 && styxterminallaunchreceive(&payload_receiver, offset, bytes, *count) ? nil : Ebadarg;
    }
    FVM_SRecord record;
    size_t consumed;
    if (!require_submission || offset != 0 ||
        fvm_srecord_read(bytes, *count, 128, &record, &consumed) != FVM_S_OK ||
        consumed != *count || record.tag != 17)
    {
        return Ebadarg;
    }
    int scoped = getenv("DVM_TEST_TERMINAL_SCOPED") != NULL;
    size_t prefix = scoped ? STYX_LAUNCH_PROGRAM_HEADER : 0;
    DVM_LaunchProgram program;
    if (submitted || record.size != prefix + 3 ||
        (scoped && (!styxlaunchprogramdecode(record.payload, record.size, &program) ||
            program.pid != 42 || program.parent_pid != 41 || program.hidden_fds != (UINT32_C(1) << 2))) ||
        memcmp(record.payload + prefix, "r\0n", 3))
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
