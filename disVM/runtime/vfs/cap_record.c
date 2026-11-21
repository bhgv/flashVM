/* Copyright (c) 2026 bhgv
 * EN: Compose named VFS fields from existing live variable capabilities.
 * RU: Собрать именованные поля VFS из возможностей живых переменных.
 */
#include "cap_internal.h"
#include "view_internal.h"
#include <stdio.h>

#define RECORD_FIELD_LIMIT (DVM_VIEW_RULE_LIMIT - 2U) /* The remaining mappings hold metadata. */

typedef struct RecordField
{
    String *name; /* Rooted by the caller's immutable list. */
    void *root; /* Rooted native capability shell. */
} RecordField;


/* EN: Pin borrowed field roots by basename and install immutable metadata.
 * RU: Удержать заимствованные корни полей по имени и добавить метаданные. */
static DVM_Provider *record_view(List *fields)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    unsigned count = 0;
    for (List *item = fields; item != H; item = item->tail)
    {
        RecordField field;
        memcpy(&field, item->data, sizeof(field));
        DVM_CapState *root = dvm_cap_try_root(field.root);
        char path[FVM_FS_NAME_MAX + 1] = "/";
        DVM_ViewRule *previous;
        char relative[DVM_NS_PATH_LIMIT];
        if (count >= RECORD_FIELD_LIMIT || !root || !root->root ||
            !dvm_cap_text(field.name, path + 1, sizeof(path) - 1) || path[1] == '.' || strchr(path + 1, '/') ||
            dvm_view_route(view, path, 0, &previous, relative) ||
            !dvm_view_map(view, path, root->root, ".", FVM_ACCESS_MASK, DVM_VIEW_TREE))
        {
            dvm_view_discard(view);
            return NULL;
        }
        count++;
    }
    char length[16];
    snprintf(length, sizeof(length), "%u\n", count);
    if (!dvm_cap_map_metadata(view, "/.type", "record\n") || !dvm_cap_map_metadata(view, "/.len", length))
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN: Build a record from the exact named capability list rooted by the frame.
 * RU: Создать запись из списка именованных возможностей, укоренённого кадром. */
void Vfsexport_record(void *frame)
{
    F_Vfsexport_record call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_record, ret), sizeof(*call.ret));
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (state)
    {
        state->root = record_view(call.fields);
        if (!state->root)
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}
