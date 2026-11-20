/* Copyright (c) 2026 bhgv
 * EN: Original Styx dispatcher for an explicitly granted variable file.
 * RU: Исходный диспетчер Styx для явно выданного файла переменной.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "variable_server.h"
#include "file_provider.h"
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct StyxVariableServer
{
    Styxserver server; /* Owns the original dispatcher and channel. */
    char identity[sizeof("4294967295\n")]; /* Immutable PID file for scoped mode. */
    StyxBacking **backings; /* Exact published nodes in configuration order. */
    size_t count; /* Number of entries, immutable during dispatch. */
    pthread_t worker; /* Dispatch thread joined before retirement. */
};

static __thread StyxVariableServer *active; /* Set on the dispatcher only. */


#define FIRST_VARIABLE 7UL /* Qids below this value belong to the directory structure. */


/* EN/RU: Decode an actual variable qid without accepting unrelated nodes.
 * Разобрать qid переменной, исключая другие узлы. */
static StyxBacking *backing(Path qid)
{
    return qid >= FIRST_VARIABLE && qid - FIRST_VARIABLE < active->count ?
        active->backings[qid - FIRST_VARIABLE] : NULL;
}


/* EN/RU: Open an exact variable fid with its own handle and destructor.
 * Открыть точный fid переменной со своим дескриптором и освобождением. */
static char *open_node(Qid *qid, int mode)
{
    StyxBacking *node = backing(qid->path);
    if (!node)
    {
        return mode == OREAD ? nil : Eperm;
    }
    StyxOpen *opened = styxfileopen(node, mode);
    if (!opened || styxsetauxowned(&active->server, opened, styxfileclose) < 0)
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Return only a confirmed read from the currently opened variable.
 * Вернуть подтверждённое чтение из открытой переменной. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path == 6 && offset >= 0)
    {
        *count = styxreadstr(offset, bytes, *count, active->identity);
        return nil;
    }
    if (!backing(qid.path) || offset < 0)
    {
        return Ebadarg;
    }
    StyxTransfer result;
    styxfileread(styxgetaux(&active->server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}


/* EN/RU: Forward one write and acknowledge only a confirmed byte count.
 * Передать одну запись и подтвердить только достоверное число байтов. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (!backing(qid.path) || offset < 0)
    {
        return Ebadarg;
    }
    StyxTransfer result;
    styxfilewrite(styxgetaux(&active->server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}

static Styxops ops = {.open = open_node, .read = read_node, .write = write_node}; /* Shared dispatcher operations. */


/* EN/RU: Retire all fids before the host releases the retained backing.
 * Закрыть все fid до освобождения удерживаемого файла владельцем. */
static void *serve(void *state)
{
    active = state;
    while (!styxwait(&active->server) && !styxprocess(&active->server))
    {
    }
    styxend(&active->server);
    active = NULL;
    return NULL;
}


/* EN/RU: Validate a trusted directory or filename as one Styx component.
 * Проверить имя каталога или файла как один компонент Styx. */
static int component(const char *name)
{
    return name && *name && strcmp(name, ".") && strcmp(name, "..") && !strchr(name, '/');
}


/* EN/RU: Build the process directory and its read-only PID file before dispatch.
 * Создать каталог процесса и файл PID только для чтения до запуска диспетчера. */
static int program_tree(StyxVariableServer *server, const char *module, uint32_t pid)
{
    char name[sizeof("4294967295")];
    snprintf(name, sizeof(name), "%" PRIu32, pid);
    snprintf(server->identity, sizeof(server->identity), "%s\n", name);
    if (!styxadddir(&server->server, Qroot, 3, (char *)module, 0555, "variable") ||
        !styxadddir(&server->server, 3, 4, ".prog", 0555, "variable") ||
        !styxadddir(&server->server, 4, 5, name, 0555, "variable"))
    {
        return 0;
    }
    Styxfile *identity = styxaddfile(&server->server, 5, 6, ".pid", 0444, "variable");
    if (!identity)
    {
        return 0;
    }
    identity->d.length = strlen(server->identity);
    return 1;
}


/* EN/RU: Release every retained node, including partial startup.
 * Снять все удерживаемые ссылки, включая частичный запуск. */
static void release_files(StyxVariableServer *server)
{
    for (size_t i = 0; i < server->count; i++)
    {
        styxbackingfree(server->backings[i]);
    }
    free(server->backings);
    free(server);
}


/* EN/RU: Validate and publish exactly the named node and its advertised permissions.
 * Проверить и опубликовать точный именованный узел и его права. */
static int publish_file(StyxVariableServer *server, const StyxPublishedVariable *file, size_t index)
{
    if (!component(file->name) || !file->file)
    {
        return 0;
    }
    StyxBacking *node = server->backings[index] = styxbacking(file->file);
    StyxMeta info;
    if (!node || !styxbackinginfo(node, &info) || info.directory || !(info.mode & 0666))
    {
        return 0;
    }
    Styxfile *entry = styxaddfile(&server->server, 1, FIRST_VARIABLE + index,
        (char *)file->name, info.mode & 0666, "variable");
    if (!entry)
    {
        return 0;
    }
    entry->d.length = info.length;
    return 1;
}


/* EN/RU: Build the entire unpublished tree before starting the dispatcher.
 * Построить полное неопубликованное дерево до запуска диспетчера. */
static int publish_tree(StyxVariableServer *server, const char *module, uint32_t pid,
    const StyxPublishedVariable *files)
{
    if ((module && !program_tree(server, module, pid)) ||
        !styxadddir(&server->server, module ? 5 : Qroot, 1, ".var", 0555, "variable"))
    {
        return 0;
    }
    for (size_t i = 0; i < server->count; i++)
    {
        if (!publish_file(server, &files[i], i))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: Stage all files in a consumed connection, returning an owned server on success.
 * Подготовить все файлы в принятом соединении, вернуть сервер при успехе. */
static StyxVariableServer *start(RemotePhy *channel, const char *module, uint32_t pid,
    const StyxPublishedVariable *files, size_t count)
{
    StyxVariableServer *server = channel && files && count && count <= STYX_VARIABLE_SERVER_LIMIT &&
        (!module || (pid && component(module))) ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->backings = calloc(count, sizeof(*server->backings));
    if (!server->backings)
    {
        phyfree(channel);
        free(server);
        return NULL;
    }
    server->count = count;
    if (!styxphyinit(&server->server, &ops, channel, 0555, 1, 2))
    {
        if (publish_tree(server, module, pid, files) && !pthread_create(&server->worker, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    release_files(server);
    return NULL;
}


/* EN/RU: Publish the exact file in the existing unscoped layout.
 * Опубликовать точный файл в существующем дереве без контекста процесса. */
StyxVariableServer *styxvariableserverstart(RemotePhy *channel, const char *name, DVM_Provider *file)
{
    const StyxPublishedVariable variable = {name, file};
    return start(channel, NULL, 0, &variable, 1);
}


/* EN/RU: Scope one published file to a trusted module name and client PID.
 * Ограничить один опубликованный файл именем доверенной загрузки и PID клиента. */
StyxVariableServer *styxvariableserverstartprogram(RemotePhy *channel, const char *module,
    uint32_t pid, const char *name, DVM_Provider *file)
{
    const StyxPublishedVariable variable = {name, file};
    return styxvariableserverstartprogramfiles(channel, module, pid, &variable, 1);
}


/* EN/RU: Scope the complete publication to a trusted module name and client PID.
 * Ограничить всю публикацию именем доверенной загрузки и PID клиента. */
StyxVariableServer *styxvariableserverstartprogramfiles(RemotePhy *channel, const char *module,
    uint32_t pid, const StyxPublishedVariable *files, size_t count)
{
    if (!module)
    {
        phyfree(channel);
        return NULL;
    }
    return start(channel, module, pid, files, count);
}


/* EN/RU: Finish server-side ownership after the dispatcher has retired.
 * Освободить владение сервера после остановки диспетчера. */
int styxvariableserverjoin(StyxVariableServer *server)
{
    if (!server || pthread_join(server->worker, NULL))
    {
        return 0;
    }
    release_files(server);
    return 1;
}
