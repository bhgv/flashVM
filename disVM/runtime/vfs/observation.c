/* Copyright (c) 2026 bhgv
 * EN: Coherent text slices from existing namespace state; no retained snapshot cache.
 * RU: Согласованные text slices существующего namespace; без постоянного snapshot cache.
 */
#include "internal.h"
#include "observation.h"
#include <string.h>


/* EN/RU: Select borrowed text while group lock is held; NULL for invalid field/index.
 * Выбрать borrowed text под group lock; NULL для неверных field/index. No I/O. */
static const char *text(DVM_Namespace *group, size_t index, int field, int *held)
{
    DVM_NamespaceBinding *binding;
    if (field < DVM_NS_FIELD_TARGET)
    {
        if (index)
        {
            return NULL;
        }
        switch (field)
        {
        case DVM_NS_FIELD_ROOT:
            return group->root;
        case DVM_NS_FIELD_CWD:
            return group->cwd;
        case DVM_NS_FIELD_IMPORT:
            return group->load_directory;
        case DVM_NS_FIELD_CWD_SOURCE:
            return group->cwd_source;
        case DVM_NS_FIELD_IMPORT_SOURCE:
            return group->load_source;
        default:
            return NULL;
        }
    }
    for (binding = group->bindings; binding && index; binding = binding->next)
    {
        index--;
    }
    if (!binding)
    {
        return NULL;
    }
    *held = binding->anchor != NULL || binding->provider != NULL;
    switch (field)
    {
    case DVM_NS_FIELD_TARGET:
        return binding->on;
    case DVM_NS_FIELD_SOURCE:
        return binding->source;
    case DVM_NS_FIELD_COMPONENT:
        return binding->component;
    default:
        return NULL;
    }
}


/* EN/RU: See observation.h. Copy values only; borrowed group is never retained here.
 * См. observation.h. Только copied values; borrowed group здесь не retain. */
int dvm_namespace_observe(DVM_Namespace *group,
                          const uint64_t *expected,
                          size_t index,
                          int field,
                          size_t offset,
                          unsigned char *bytes,
                          size_t capacity,
                          DVM_NamespaceObservation *output)
{
    DVM_NamespaceBinding *binding;
    const char *source;
    int result = DVM_NS_OBSERVED;
    if (!group || !output || (expected && (!bytes || !capacity || capacity > DVM_NS_OBSERVE_BYTES)))
    {
        return DVM_NS_OBSERVE_INVALID;
    }
    memset(output, 0, sizeof(*output));
    fvm_port_sync_lock(group->lock);
    output->version = group->version;
    output->references = group->references;
    output->root_held = group->root_anchor != NULL;
    output->cwd_held = (group->cwd_anchor != NULL || group->cwd_provider != NULL);
    output->import_held = (group->load_anchor != NULL || group->load_provider != NULL);
    for (binding = group->bindings; binding; binding = binding->next)
    {
        output->bindings++;
    }
    if (expected)
    {
        if (*expected != group->version)
        {
            result = DVM_NS_OBSERVE_STALE;
        }
        else
        {
            source = text(group, index, field, &output->binding_held);
            if (!source || offset > strlen(source))
            {
                result = DVM_NS_OBSERVE_INVALID;
            }
            else
            {
                output->length = strlen(source);
                output->copied = output->length - offset;
                if (output->copied > capacity)
                {
                    output->copied = capacity;
                }
                memcpy(bytes, source + offset, output->copied);
            }
        }
    }
    fvm_port_sync_unlock(group->lock);
    return result;
}
