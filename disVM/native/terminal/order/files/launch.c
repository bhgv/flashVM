/* Copyright (c) 2026 bhgv
 * EN: Prepare a default-deny Unibus file view before replacing the terminal layer.
 * RU: Подготовка закрытого по умолчанию представления Unibus до замены слоя терминала.
 */
#include "launch.h"
#include "../../../../runtime/vfs/export.h"
#include <string.h>


/* EN/RU: Stage all file providers before committing the handoff replacement.
 * Подготовить все file providers до замены окружения в handoff. */
StyxOrderedFiles *styxorderedlaunchfiles(StyxClient *client, DVM_LaunchHandoff *handoff,
    const StyxOrderedOptions *options, const StyxLaunchFile *mappings, size_t count)
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
    StyxOrderedFiles *files = styxorderedfiles(styxbidirorderedlaunch(client, handoff, options));
    DVM_View *view = files ? dvm_view_create(NULL, NULL, NULL) : NULL;
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
