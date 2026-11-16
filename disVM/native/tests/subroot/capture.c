/* Copyright (c) 2026 bhgv
 * EN: Real provider/view capture, stable-root reuse, policy rebasing and source-journal lifetime.
 * RU: Настоящий provider/view capture, reuse stable root, policy rebasing и lifetime source journal.
 */
#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: External source survives until every original/captured native factory is destroyed.
 * External source живёт до destruction всех original/captured native factories. */
typedef struct Source
{
    char current; /* Mutable mapping to demonstrate source-path replacement. */
    unsigned captures; /* Actual fresh node captures, excluding anchored dot reuse. */
    unsigned destroyed; /* All source/node state destructions. */
    int allowed; /* Dynamic access policy for prepared view. */
    char checked[DVM_NS_PATH_LIMIT]; /* Last policy virtual path, never a source I/O pathname. */
} Source;


/* EN/RU: Factory maps dot to a directory/file; anchored nodes fix their selected byte identity.
 * Factory отображает dot в directory/file; anchored nodes фиксируют выбранную byte identity. */
typedef struct Node
{
    Source *source; /* Borrowed external fixture storage through all owners. */
    char fixed; /* Zero for live source mapping; otherwise the captured object identity. */
    int directory; /* Root node kind. */
} Node;


/* EN/RU: Check contracts independently of compiler optimization flags.
 * Проверить contracts независимо от compiler optimization flags. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "provider capture: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Resolve dot/dir/item while preserving each anchored node's fixed identity.
 * Разрешить dot/dir/item с сохранением fixed identity anchored node. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    Node *node = opaque;
    int directory = node->directory && (!strcmp(path, ".") || !strcmp(path, "dir"));
    if (!directory && (node->directory ? strcmp(path, "item") && strcmp(path, "dir/item") && strcmp(path, "hide") && strcmp(path, "dir/hide") : strcmp(path, ".")))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = directory;
    out->mode = directory ? 0555 : 0444;
    out->identity.low = node->fixed ? node->fixed : node->source->current;
    strcpy(out->name, directory ? "dir" : "item");
    return FVM_OK;
}


/* EN/RU: Read one byte from captured state pinned by the outer provider wrapper.
 * Прочитать byte из captured state под outer provider lifetime pin. */
static FVM_Result readfile(void *opaque, void *bytes, size_t size, size_t *count)
{
    Node *node = opaque;
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = node->fixed ? node->fixed : node->source->current;
    }
    return FVM_OK;
}


/* EN/RU: Inner test stream borrows factory state; outer wrapper owns its lifetime.
 * Inner test stream заимствует factory state; outer wrapper владеет lifetime. */
static void closefile(void *opaque)
{
    (void)opaque;
}
static const DVM_VfsOps opened = {NULL, readfile, NULL, NULL, NULL, closefile}; /* Ordinary readonly fixture stream. */


/* EN/RU: Open only real files, checking current route before returning a native stream.
 * Открыть только real files, проверяя current route до выдачи native stream. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo meta;
    if (strcmp(mode, "r") || info(opaque, path, &meta) != FVM_OK || meta.directory)
    {
        return FVM_ERROR;
    }
    out->ops = &opened;
    out->state = opaque;
    return FVM_OK;
}


/* EN/RU: Count and free factory node; external source remains owned by main fixture.
 * Посчитать и освободить factory node; external source принадлежит main fixture. */
static void destroy(void *opaque)
{
    Node *node = opaque;
    node->source->destroyed++;
    free(node);
}
static const DVM_ProviderOps operations = {info, openfile, destroy, 0, NULL}; /* Dynamic relative paths require explicit capture. */


/* EN/RU: Return a fresh rooted node; never reuse current source pathname after capture.
 * Вернуть fresh rooted node; не использовать прежний source pathname после capture. */
static DVM_Provider *capture(void *opaque, const char *path)
{
    Node *node = opaque;
    FVM_FileInfo meta;
    if (info(opaque, path, &meta) != FVM_OK)
    {
        return NULL;
    }
    Node *fixed = calloc(1, sizeof(*fixed));
    if (!fixed)
    {
        return NULL;
    }
    *fixed = (Node){node->source, (char)meta.identity.low, meta.directory};
    DVM_Provider *out = dvm_provider_create_capture(&operations, fixed, capture, 1);
    if (!out)
    {
        free(fixed);
        return NULL;
    }
    node->source->captures++;
    return out;
}


/* EN/RU: Construct an ordinary live source with per-node capture, no global stable-bind promise.
 * Создать ordinary live source с per-node capture, без global stable-bind promise. */
static DVM_Provider *source(Source *state)
{
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "source allocation");
    *node = (Node){state, 0, 1};
    DVM_Provider *out = dvm_provider_create_capture(&operations, node, capture, 0);
    check(out != NULL, "source factory");
    return out;
}


/* EN/RU: Read ordinary provider path; a Root capability is not a copy of its bytes.
 * Прочитать ordinary provider path; Root capability не копирует bytes. */
static char value(DVM_Provider *provider, const char *path)
{
    DVM_VfsHandle fd = {0};
    char byte = 0;
    check(dvm_provider_open(provider, path, "r", &fd) == FVM_OK, "ordinary captured open");
    DVM_IoResult result = dvm_vfs_read_chunk(&fd, &byte, 1);
    check(result.status == FVM_OK && result.count == 1, "ordinary captured read");
    dvm_vfs_close(&fd);
    return byte;
}


/* EN/RU: Policy records original virtual scope and may revoke existing captured aliases.
 * Policy записывает original virtual scope и может отозвать existing captured aliases. */
static int policy(void *opaque, const char *path, unsigned access)
{
    Source *state = opaque;
    (void)access;
    strcpy(state->checked, path);
    return state->allowed;
}


/* EN/RU: Test anchored dot reuse, fresh captures after path replacement and prepared view semantics.
 * Проверить anchored dot reuse, fresh capture после path replacement и prepared view semantics. */
int main(void)
{
    Source a = {'A', 0, 0, 1, ""}, b = {'C', 0, 0, 1, ""};
    DVM_Provider *raw = source(&a), *other = source(&b);
    DVM_Provider *fixed = dvm_provider_capture(raw, "dir");
    check(fixed && !dvm_provider_bindable(raw) && dvm_provider_capturable(raw) &&
          dvm_provider_bindable_at(fixed, ".") && !dvm_provider_bindable_at(fixed, "item"), "route capability distinctions");
    DVM_Provider *same = dvm_provider_capture(fixed, ".");
    check(same == fixed && a.captures == 1 && !dvm_provider_capture(fixed, "missing"), "anchored dot reuse/failure");
    dvm_provider_release(same);
    DVM_View *builder = dvm_view_create(policy, &a, NULL);
    check(builder && dvm_view_map(builder, "/tree", raw, "dir", FVM_ACCESS_READ|FVM_ACCESS_EXEC, DVM_VIEW_TREE) &&
          dvm_view_map(builder, "/tree/hide", NULL, ".", 0, 0) &&
          dvm_view_map(builder, "/tree/swap", other, "dir/item", FVM_ACCESS_READ, 0), "prepared overlay rules");
    DVM_Provider *view = dvm_view_freeze(builder);
    DVM_Provider *chosen = dvm_provider_capture(view, "tree");
    check(chosen && dvm_provider_bindable_at(chosen, "."), "prepared subtree capture");
    a.current = 'B';
    check(value(raw, "dir/item") == 'B' && value(fixed, "item") == 'A' &&
          value(chosen, "item") == 'A' && !strcmp(a.checked, "/tree/item"), "capture reopened replaced original path");
    check(value(chosen, "swap") == 'C', "explicit source substitution");
    b.current = 'D';
    check(value(chosen, "swap") == 'D', "explicit overlay definition froze live source");
    FVM_FileInfo meta;
    check(dvm_provider_info(chosen, "hide", &meta) != FVM_OK &&
          dvm_provider_access(chosen, "item", FVM_ACCESS_WRITE) != FVM_OK, "hidden node/rights escaped capture");
    a.allowed = 0;
    DVM_VfsHandle denied = {0};
    check(dvm_provider_open(chosen, "item", "r", &denied) != FVM_OK && !denied.state, "dynamic policy revocation lost");
    a.allowed = 1;
    DVM_Changes *journal = dvm_provider_changes(chosen);
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
          dvm_changes_ack(journal, &cursor, &batch) == FVM_OK, "initial captured journal");
    DVM_Changes *origin = dvm_provider_changes(raw);
    check(changesretain(origin), "independent source journal pin");
    dvm_provider_release(raw);
    dvm_provider_release(other);
    dvm_provider_release(view);
    check(!a.destroyed && !b.destroyed, "captured roots dropped original policy/feed owners");
    check(changespost(origin, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 7) &&
          dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
          (batch.event.kinds & DVM_CHANGE_CONTENT) && !batch.event.flags, "source invalidation forwarding/scope");
    check(value(chosen, "item") == 'A', "source owner release broke captured subtree");
    dvm_changes_destroy(origin);
    dvm_provider_release(chosen);
    dvm_provider_release(fixed);
    check(a.destroyed == a.captures+1 && b.destroyed == 1, "capture factory/policy lifetime leak");
    puts("PASS provider capture root reuse, replacement, composed views, live policy and source events");
    return 0;
}
