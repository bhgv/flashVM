/* Copyright (c) 2026 bhgv
 * EN: Replacement objects at the same spelling are distinct; actual held-handle recursion remains a cycle.
 * RU: Replacement objects с одним именем различны; actual recursion held handle остаётся cycle.
 */
#include "../../../runtime/vfs/provider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main retains both independently opened replacement objects through every callback.
 * Main удерживает оба independently opened replacement objects до завершения callbacks.
 */
typedef struct State
{
    DVM_VfsHandle *next; /* Borrowed optional recursive metadata/list target, changed only between completed operations. */
    unsigned opens; /* Captured immutable object identity supplied to each new open. */
    unsigned calls; /* Metadata/list callback entry count. */
} State;


/* EN/RU: One open owns only this native descriptor, never the factory or another handle.
 * Один open владеет лишь native descriptor, не factory/другим handle.
 */
typedef struct Held
{
    State *source; /* Borrowed factory state under ordinary provider wrapper pin. */
    unsigned object; /* Captured replacement identity, never changed after open. */
} Held;


/* EN/RU: Explicit invariant independent of NDEBUG; borrowed message used only for reporting.
 * Явный invariant независимо от NDEBUG; message borrowed лишь для reporting.
 */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "held walk: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Named factory metadata has no side effects and describes a directory resource.
 * Factory metadata без effects, описывает directory resource.
 */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    (void)path;
    memset(output, 0, sizeof(*output));
    output->directory = 1;
    output->mode = 0555;
    return FVM_OK;
}


/* EN/RU: First held object may inspect another captured object; second returns its own immutable identity.
 * Первый held object может исследовать другой captured object; второй возвращает собственную identity.
 */
static FVM_Result held_info(void *state, FVM_FileInfo *output)
{
    Held *held = state;
    held->source->calls++;
    if (held->object == 1 && held->source->next)
    {
        return dvm_vfs_info(held->source->next, output);
    }
    info(NULL, ".", output);
    output->identity.low = held->object;
    return FVM_OK;
}


/* EN/RU: Directory callbacks use the same held-object identity rule and leave output empty on refusal.
 * Directory callbacks используют ту же held-object identity, output пуст при refusal.
 */
static FVM_Result held_list(void *state, FVM_FileInfo **entries, size_t *count)
{
    Held *held = state;
    held->source->calls++;
    if (held->object == 1 && held->source->next)
    {
        return dvm_vfs_list(held->source->next, entries, count);
    }
    *entries = NULL;
    *count = 0;
    return FVM_OK;
}


/* EN/RU: Consume one opened native descriptor; factory state is still owned by main.
 * Поглотить opened native descriptor; factory state принадлежит main.
 */
static void close_held(void *state)
{
    free(state);
}


/* EN/RU: Invalidate the test cursor after a failed publication; it has no buffered entries to discard.
 * Пометить test cursor после failed publication; buffered entries отсутствуют.
 */
static void abort_held(void *state)
{
    (void)state;
}
static const DVM_VfsOps held_ops = {.info = held_info, .list = held_list, .close = close_held, .abort_list = abort_held}; /* Immutable test held-object dispatch. */


/* EN/RU: Each open captures a different actual replacement object despite the identical original spelling.
 * Каждый open захватывает другой actual replacement object несмотря на одинаковое original name.
 */
static FVM_Result open_held(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)path;
    (void)mode;
    Held *held = calloc(1, sizeof(*held));
    if (!held)
    {
        return FVM_OUT_OF_MEMORY;
    }
    held->source = state;
    held->object = ++held->source->opens;
    output->ops = &held_ops;
    output->state = held;
    return FVM_OK;
}


/* EN/RU: Main owns stack factory state; final provider release must not free it.
 * Main владеет stack factory state; final provider release его не освобождает.
 */
static void destroy(void *state)
{
    (void)state;
}
static const DVM_ProviderOps operations = {info, open_held, destroy, 0, NULL}; /* Replacement-capable native factory. */


/* EN/RU: Distinguish two actual objects at one name, stop self-recursion before a second callback, then recover.
 * Различить два actual objects с одним именем, остановить self-recursion до повторного callback, затем recovery.
 */
int main(void)
{
    State state = {0};
    DVM_Provider *provider = dvm_provider_create(&operations, &state);
    DVM_VfsHandle first = {0};
    DVM_VfsHandle second = {0};
    check(provider && dvm_provider_open(provider, "same", "r", &first) == FVM_OK &&
        dvm_provider_open(provider, "same", "r", &second) == FVM_OK, "replacement opens");
    FVM_FileInfo metadata;
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    state.next = &second;
    check(dvm_vfs_info(&first, &metadata) == FVM_OK && metadata.identity.low == 2 && state.calls == 2 &&
        dvm_vfs_list(&first, &entries, &count) == FVM_OK && !entries && !count && state.calls == 4,
        "different held objects at same name were confused with a cycle");
    state.next = &first;
    check(dvm_vfs_info(&first, &metadata) == DVM_VFS_CYCLE && state.calls == 5 &&
        dvm_vfs_list(&first, &entries, &count) == DVM_VFS_CYCLE && state.calls == 6 && !walk_depth(),
        "self-recursive held object bypassed active identity guard");
    state.next = &second;
    check(dvm_vfs_info(&first, &metadata) == FVM_OK && !walk_depth(), "held history survived a completed operation");
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    dvm_provider_release(provider);
    puts("PASS replacement held-object identity, metadata/list cycles and subsequent recovery");
    return 0;
}
