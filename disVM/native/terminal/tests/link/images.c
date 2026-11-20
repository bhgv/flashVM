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
#include "variable_file.h"
#include "indexed_styx.h"
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
    ProgramVar = 834, /* Variable directory. */
    ProgramCounter = 835, /* Mutable signed integer. */
    ProgramPid = 833, /* Read-only PID of the selected process. */
    ProgramInvalid = 831 /* Process with an invalid .vfs node. */
};


static int32_t counter = 1; /* Client-owned value, accessed by the Styx dispatcher. */


/* EN/RU: Read the current owner variable into the supplied output.
 * Прочитать текущее значение переменной владельца. */
static int load_counter(void *state, int32_t *value)
{
    *value = *(int32_t *)state;
    return 1;
}


/* EN/RU: Commit the new owner variable value.
 * Записать новое значение переменной владельца. */
static int store_counter(void *state, int32_t value)
{
    *(int32_t *)state = value;
    return 1;
}

static const StyxIntVariable variable = {&counter, load_counter, store_counter}; /* Proxies owner storage. */


static int32_t elements[8] = {11, 22}; /* Client-owned array test storage. */
static StyxIntArray array; /* Live collection state. */
static StyxIntArray table; /* Live keyed collection. */
static StyxArrayFiles table_files; /* Reusable decoder for table qids. */
static uint32_t table_key; /* Key of the fixture's single live entry. */
static int32_t table_value; /* Owner storage for that entry. */
static StyxArrayFiles array_files; /* Reusable decoder for array qids. */


/* EN/RU: Read the requested element from the client storage.
 * Прочитать запрошенный элемент из хранилища клиента. */
static int load_element(void *state, uint32_t index, int32_t *value)
{
    *value = ((int32_t *)state)[index];
    return 1;
}


/* EN/RU: Replace the requested element in the client storage.
 * Заменить запрошенный элемент в хранилище клиента. */
static int store_element(void *state, uint32_t index, int32_t value)
{
    ((int32_t *)state)[index] = value;
    return 1;
}


/* EN/RU: Insert a value and move the suffix within the fixture's capacity.
 * Вставить значение и сдвинуть хвост в пределах ёмкости стенда. */
static int insert_element(void *state, uint32_t index, int32_t value)
{
    int32_t *items = state;
    if (array.length >= nelem(elements))
    {
        return 0;
    }
    memmove(items + index + 1, items + index, (array.length - index) * sizeof(*items));
    items[index] = value;
    return 1;
}


/* EN/RU: Return the last element; the adapter then decrements the length.
 * Вернуть последний элемент, после чего адаптер уменьшит длину. */
static int pop_element(void *state, int32_t *value)
{
    *value = ((int32_t *)state)[array.length - 1];
    return 1;
}


/* EN/RU: Match the stored unsigned key, independent of the entry count.
 * Найти сохранённый беззнаковый ключ независимо от числа записей. */
static int contains_key(void *state, uint32_t key)
{
    return table.length && key == *(uint32_t *)state;
}


/* EN/RU: Copy the fixture's value only for its live key.
 * Скопировать значение стенда только для существующего ключа. */
static int load_key(void *state, uint32_t key, int32_t *value)
{
    if (!contains_key(state, key))
    {
        return 0;
    }
    *value = table_value;
    return 1;
}


/* EN/RU: Replace the value of the fixture's live key.
 * Обновить значение существующего ключа стенда. */
static int store_key(void *state, uint32_t key, int32_t value)
{
    if (!contains_key(state, key))
    {
        return 0;
    }
    table_value = value;
    return 1;
}


/* EN/RU: Create the fixture's single supported entry.
 * Создать единственную поддерживаемую запись стенда. */
static int add_key(void *state, uint32_t key, int32_t value)
{
    if (table.length)
    {
        return 0;
    }
    *(uint32_t *)state = key;
    table_value = value;
    return 1;
}


/* EN/RU: Route exact indexed nodes to their library descriptor.
 * Передать точные узлы коллекций библиотечному описанию. */
static StyxArrayFiles *indexed_node(Path qid)
{
    if (array_files.server && qid >= array_files.base && qid - array_files.base < 8)
    {
        return &array_files;
    }
    return table_files.server && qid >= table_files.base && qid - table_files.base < 9 ? &table_files : NULL;
}


/* EN/RU: Forward array opens; let unrelated fixture nodes open normally.
 * Передать открытия массива, сохранив обычное открытие других узлов стенда. */
char *fixturevariableopen(Qid *qid, int mode)
{
    StyxArrayFiles *files = indexed_node(qid->path);
    return files ? styxarrayfileopen(files, qid, mode) : nil;
}


/* EN/RU: Apply one complete scalar update to the fixture variable.
 * Применить полное изменение скалярной переменной стенда. */
char *fixturevariablewrite(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxArrayFiles *files = indexed_node(qid.path);
    if (files)
    {
        return styxarrayfilewrite(files, qid, bytes, count, offset);
    }
    return qid.path == ProgramCounter && offset >= 0 &&
        styxintvariablewrite(&variable, offset, bytes, *count) ? nil : Ebadarg;
}


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
    Styxfile *file;
    if (!styxadddir(server, Qroot, ProgramModule, "loaded.dis", 0555, "fixture") ||
        !styxadddir(server, ProgramModule, ProgramRoot, ".prog", 0555, "fixture") ||
        !styxadddir(server, ProgramRoot, ProgramBase, "41", 0555, "fixture") ||
        !styxadddir(server, ProgramRoot, ProgramChild, "42", 0555, "fixture") ||
        !styxadddir(server, ProgramChild, ProgramChildFds, ".fd", 0555, "fixture") ||
        !styxaddfile(server, ProgramChild, ProgramPid, ".pid", 0444, "fixture") ||
        !styxadddir(server, ProgramChild, ProgramVar, ".var", 0555, "fixture") ||
        !(file = styxaddfile(server, ProgramVar, ProgramCounter, "counter", 0666, "fixture")) ||
        !publish_environment(server, ProgramBase, ProgramBaseVfs) ||
        !styxadddir(server, ProgramRoot, ProgramInvalid, "43", 0555, "fixture") ||
        !styxaddfile(server, ProgramInvalid, ProgramInvalid + 1, ".vfs", 0444, "fixture") ||
        (!getenv("DVM_TEST_TERMINAL_SCOPED_BASE") && !publish_environment(server, ProgramChild, ProgramChildVfs)))
    {
        _exit(3);
    }
    array = (StyxIntArray){.state = elements, .length = 2, .load = load_element,
        .store = store_element, .insert = insert_element, .pop = pop_element};
    table = (StyxIntArray){.state = &table_key, .load = load_key, .store = store_key,
        .contains = contains_key, .add = add_key};
    if (!styxarrayfilepublish(&array_files, server, ProgramVar, 840, "items", "fixture", &array) ||
        !styxarrayfilepublish(&table_files, server, ProgramVar, 848, "lookup", "fixture", &table))
    {
        _exit(3);
    }
    file->d.length = STYX_VARIABLE_INT_SIZE;
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
    StyxArrayFiles *files = indexed_node(qid.path);
    if (files)
    {
        return styxarrayfileread(files, qid, bytes, count, offset);
    }
    if (qid.path == ProgramCounter && offset >= 0)
    {
        size_t size = *count;
        if (!styxintvariableread(&variable, offset, bytes, &size))
        {
            return Ebadarg;
        }
        *count = size;
        return nil;
    }
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
    if (!getenv("DVM_TEST_TERMINAL_SCOPED"))
    {
        return size == 3 && !memcmp(bytes, "x\0y", size);
    }
    DVM_LaunchProgram caller;
    if (size != STYX_LAUNCH_PROGRAM_HEADER + 7 ||
        !styxlaunchprogramdecode(bytes, size, &caller) || caller.parent_pid || caller.hidden_fds)
    {
        return 0;
    }
    const unsigned char *payload = (const unsigned char *)bytes + STYX_LAUNCH_PROGRAM_HEADER;
    uint32_t expected = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) |
        ((uint32_t)payload[2] << 8) | payload[3];
    uint32_t *previous = state;
    if (caller.pid != expected || caller.pid == *previous || memcmp(payload + 4, "x\0y", 3))
    {
        return 0;
    }
    *previous = caller.pid;
    return 1;
}

static uint32_t previous_pid; /* Scoped fixture requires two distinct real callers. */
static StyxLaunchReceiver second_receiver = {.tag = 17, .start_payload = accept_payload,
    .state = &previous_pid, .payload_limit = STYX_LAUNCH_PROGRAM_HEADER + 7}; /* Scoped second write. */
static StyxLaunchReceiver payload_receiver = {.tag = 17, .start_payload = accept_payload,
    .state = &previous_pid, .payload_limit = 3}; /* Independent payload submission after boot. */


// EN/RU: Accept exactly one complete launch frame, before exposing it to the dispatcher.
// Принять ровно один полный кадр запуска перед выдачей диспетчеру.
char *fixturelaunchwrite(char *bytes, ulong *count, vlong offset)
{
    if (submitted)
    {
        if (getenv("DVM_TEST_TERMINAL_SCOPED"))
        {
            payload_receiver.payload_limit = STYX_LAUNCH_PROGRAM_HEADER + 7;
            if (payload_receiver.used)
            {
                return offset >= 0 && styxterminallaunchreceive(&second_receiver, offset, bytes, *count) ? nil : Ebadarg;
            }
        }
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
