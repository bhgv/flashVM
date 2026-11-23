/* Copyright (c) 2026 bhgv
 * EN: Callback grants for a C-owned process with a variable, fd and file.
 * RU: Колбэки процесса C с расшаренной переменной, fd и файлом.
 */
#include "reverse.h"
#include "styxrpc/variable_file.h"
#include <string.h>
#include <stdlib.h>

enum
{
    Root,    /* Attach root. */
    Progs,   /* .prog */
    Process, /* .prog/42 */
    Pid,     /* Caller PID text. */
    Fds,     /* Numbered descriptors. */
    Output,  /* FD 7 with write-only permission. */
    Vars,    /* Shared variables. */
    Value,   /* Signed 32-bit callback variable. */
    Vfs,     /* Reverse namespace. */
    Marker,  /* Read-only origin marker. */
    Parked,  /* Write-only checkpoint for cancellation. */
    Input,   /* Read-only standard input. */
    Stdout   /* Write-only standard output. */
};

static const StyxRpcEntry entries[] = { /* Stable descriptors in enumeration order. */
    {Root, "/", 0555, 1},
    {Progs, ".prog", 0555, 1},
    {Process, "42", 0555, 1},
    {Pid, ".pid", 0444, 0},
    {Fds, ".fd", 0555, 1},
    {Output, "7", 0222, 0},
    {Vars, ".var", 0555, 1},
    {Value, "count", 0666, 0},
    {Vfs, ".vfs", 0555, 1},
    {Marker, "marker", 0444, 0},
    {Parked, "parked", 0222, 0},
    {Input, "0", 0444, 0},
    {Stdout, "1", 0222, 0}
};
static const uint64_t parents[] = {Root, Root, Progs, Process, Process, Fds, Process, Vars, Process, Vfs, Vfs, Fds, Fds};
static const char pid[] = "42\n";   /* Wire identity file. */
static const char input[] = "I";    /* Byte available on read-only fd 0. */
static const char marker[] = "C\n"; /* Caller-owned virtual namespace content. */


/* EN: Return the indexed child by scanning the tiny immutable test tree.
 * RU: Вернуть потомка по индексу в небольшом неизменяемом тестовом дереве. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    (void)context;
    for (size_t i = 1; i < sizeof(entries) / sizeof(*entries); ++i)
    {
        if (parents[i] == directory && !index--)
        {
            return entries + i;
        }
    }
    return NULL;
}


/* EN: Copy current metadata for the requested ID.
 * RU: Скопировать текущие метаданные для запрошенного ID. */
static int stat_entry(void *context, uint64_t id, StyxRpcNode *node)
{
    (void)context;
    if (id >= sizeof(entries) / sizeof(*entries))
    {
        return 0;
    }
    *node = (StyxRpcNode){entries[id], parents[id],
        id == Pid ? sizeof(pid) - 1 : id == Value ? 4 : id == Marker ? sizeof(marker) - 1 : id == Input ? sizeof(input) - 1 : 0, 0};
    return 1;
}


typedef struct Opened
{
    ReverseState *state; /* Borrowed shared reverse tree state. */
    uint64_t id;         /* Exact opened node ID. */
} Opened;


/* EN: Open one file and retain its ID independently of every other fid.
 * RU: Открыть файл и удержать его ID независимо от других fid. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    (void)access;
    Opened *opened = malloc(sizeof(*opened));
    if (!opened)
    {
        return 0;
    }
    *opened = (Opened){context, id};
    *cookie = opened;
    return 1;
}


/* EN: Load the live client variable from its native storage.
 * RU: Прочитать живую переменную клиента из нативного хранилища. */
static int load_value(void *state, int32_t *value)
{
    *value = ((ReverseState *)state)->value;
    return 1;
}


/* EN: Commit one exact decoded store to client storage.
 * RU: Применить одну декодированную запись в хранилище клиента. */
static int store_value(void *state, int32_t value)
{
    ((ReverseState *)state)->value = value;
    return 1;
}


/* EN: Read the shared variable, PID or virtual marker according to the opened ID.
 * RU: Прочитать общую переменную, PID или маркер по ID открытого узла. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Opened *opened = cookie;
    if (opened->id == Value)
    {
        const StyxIntVariable variable = {opened->state, load_value, store_value};
        *size = capacity;
        return styxintvariableread(&variable, offset, bytes, size);
    }
    const char *source = opened->id == Pid ? pid : opened->id == Input ? input : marker;
    size_t length = strlen(source);
    size_t remain = offset < length ? length - (size_t)offset : 0;
    *size = capacity < remain ? capacity : remain;
    if (*size)
    {
        memcpy(bytes, source + (size_t)offset, *size);
    }
    return 1;
}


/* EN: Store a complete signed variable or one byte in the exported output descriptor.
 * RU: Записать целый int либо один байт в выданный дескриптор вывода. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Opened *opened = cookie;
    if (opened->id == Value)
    {
        const StyxIntVariable variable = {opened->state, load_value, store_value};
        if (!styxintvariablewrite(&variable, offset, bytes, count))
        {
            return 0;
        }
    }
    else if (opened->id == Parked && !offset && count == 1 && *(const char *)bytes == 'P')
    {
        __atomic_store_n(&opened->state->parked, 1, __ATOMIC_RELEASE);
    }
    else if (opened->id == Stdout && !offset && count == 1)
    {
        opened->state->output = *(const unsigned char *)bytes;
    }
    else if (opened->id == Output && !offset && count == 1)
    {
        opened->state->byte = *(const unsigned char *)bytes;
    }
    else
    {
        return 0;
    }
    *size = count;
    return 1;
}


const StyxRpcTree reverse_tree = {{enumerate, open_file, read_file, write_file, free}, stat_entry};


/* EN/RU: See reverse.h. / См. reverse.h. */
void *run_reverse(void *context)
{
    ReverseState *state = context;
    while (styxrpc_server_step(state->server))
    {
    }
    styxrpc_server_destroy(state->server);
    return NULL;
}
