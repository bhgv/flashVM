/* Copyright (c) 2026 bhgv
 * EN: Prepare a default-deny Unibus file view before replacing the terminal layer.
 * RU: Подготовка закрытого по умолчанию представления Unibus до замены слоя терминала.
 */
#include "launch.h"
#include "../../../../runtime/vfs/cancel.h"
#include "../../../../runtime/vfs/export.h"
#include <string.h>


/* EN/RU: Test overlap of two canonical guest filenames, including file/child conflicts.
 * Проверить пересечение канонических имён, включая конфликт файла и потомка. */
static int overlaps(const char *left, const char *right)
{
    size_t a = strlen(left);
    size_t b = strlen(right);
    size_t shared = a < b ? a : b;
    return !memcmp(left, right, shared) &&
        (a == b || (a < b ? right[shared] : left[shared]) == '/');
}


/* EN/RU: Check a borrowed absolute guest alias without accessing a provider.
 * Проверить абсолютное имя у гостя без обращения к провайдеру. */
static int guest_path(const char *path)
{
    return path && path[0] == '/' && dvm_terminal_channel_paths_valid(path + 1, path + 1) &&
        strlen(path) < DVM_NS_PATH_LIMIT && strcmp(path + 1, ".");
}


/* EN/RU: Validate FD aliases and sources against the job's explicit broker routes.
 * Проверить имена файлов и их источники с учётом маршрутов брокера этого запуска. */
static int valid_fds(const DVM_TerminalChannels *terminal, const StyxLaunchFile *files, size_t count,
    const StyxLaunchFd *fds, size_t fd_count)
{
    if (fd_count > STYX_LAUNCH_FDS_MAX || (fd_count && (!fds || !dvm_terminal_channels_valid(terminal))))
    {
        return 0;
    }
    for (size_t i = 0; i < fd_count; i++)
    {
        const StyxLaunchFd *fd = &fds[i];
        if (!guest_path(fd->path) || !dvm_terminal_channel_paths_valid(fd->node, fd->node) ||
            !strcmp(fd->node, ".") || !strcmp(fd->node, terminal->receive) ||
            !strcmp(fd->node, terminal->send) || !fd->access ||
            (fd->access & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE)))
        {
            return 0;
        }
        for (size_t j = 0; j < count; j++)
        {
            if (overlaps(fd->path, files[j].path))
            {
                return 0;
            }
        }
        for (size_t j = 0; j < i; j++)
        {
            if (overlaps(fd->path, fds[j].path) || !strcmp(fd->node, fds[j].node))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN/RU: Pin an unopened file fid during preparation and map it with reduced rights.
 * Закрепить fid ещё не открытого файла при подготовке и выдать с суженными правами. */
static int map_fd(DVM_View *view, DVM_Provider *terminal, const StyxLaunchFd *fd)
{
    DVM_Provider *file = dvm_provider_capture(terminal, fd->node);
    FVM_FileInfo info;
    int mapped = file && dvm_provider_info(file, ".", &info) == FVM_OK && !info.directory &&
        dvm_view_map(view, fd->path, file, ".", fd->access, 0);
    dvm_provider_release(file);
    return mapped;
}


/* EN/RU: Stage all file providers before committing the handoff replacement.
 * Подготовить все file providers до замены окружения в handoff. */
StyxOrderedFiles *styxorderedlaunchfileswithfds(StyxClient *client, DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options, const StyxLaunchFile *mappings, size_t count,
    const StyxLaunchFd *fds, size_t fd_count)
{
    if (!client || !handoff || !handoff->group || !styxorderedoptionsvalid(options) ||
        !mappings || !count || count > options->count)
    {
        return NULL;
    }
    for (size_t i = 0; i < count; i++)
    {
        const StyxLaunchFile *entry = &mappings[i];
        if (!entry->path || entry->path[0] != '/' || !entry->path[1] ||
            !entry->access || (entry->access & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE)))
        {
            return NULL;
        }
        size_t tag = 0;
        while (tag < options->count && options->tags[tag] != entry->tag)
        {
            tag++;
        }
        if (tag == options->count)
        {
            return NULL;
        }
        for (size_t previous = 0; previous < i; previous++)
        {
            if (!strcmp(entry->path, mappings[previous].path))
            {
                return NULL;
            }
        }
    }
    if (!valid_fds(&handoff->flashvm, mappings, count, fds, fd_count))
    {
        return NULL;
    }
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    for (size_t i = 0; view && i < fd_count; i++)
    {
        if (!map_fd(view, handoff->flashvm.provider, &fds[i]))
        {
            dvm_view_discard(view);
            return NULL;
        }
    }
    StyxOrderedFiles *files = view && !dvm_cancel_requested(dvm_cancel_current()) ?
        styxorderedfiles(styxbidirorderedlaunch(client, handoff, options)) : NULL;
    if (!files)
    {
        dvm_view_discard(view);
        return NULL;
    }
    for (size_t i = 0; view && i < count; i++)
    {
        const StyxLaunchFile *entry = &mappings[i];
        DVM_Provider *file = styxorderedfile(files, entry->tag, entry->access);
        int mapped = file && dvm_view_map(view, entry->path, file, ".", entry->access, 0);
        dvm_provider_release(file);
        if (!mapped)
        {
            dvm_view_discard(view);
            view = NULL;
        }
    }
    DVM_Provider *terminal = view ? dvm_view_freeze(view) : NULL;
    int adopted = terminal && dvm_launch_handoff_project_terminal(handoff, terminal);
    dvm_provider_release(terminal);
    if (!adopted)
    {
        styxorderedfilesfree(files);
        return NULL;
    }
    return files;
}


/* EN/RU: Preserve the original record-only entry point and its ownership contract.
 * Сохранить исходный вход для выдачи записей и его контракт владения. */
StyxOrderedFiles *styxorderedlaunchfiles(StyxClient *client, DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options, const StyxLaunchFile *mappings, size_t count)
{
    return styxorderedlaunchfileswithfds(client, handoff, options, mappings, count, NULL, 0);
}
