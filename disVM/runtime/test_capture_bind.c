/* Copyright (c) 2026 bhgv
 * EN: Private real-guest capture provider; source spelling changes while captured nodes keep identity.
 * RU: Private provider настоящего guest capture; source spelling меняется, captured nodes сохраняют identity.
 */
#include "vfs/boot.h"
#include "vfs/export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char current = 'A'; /* Native fixture's live source mapping, changed only by serialized ctl writes. */
static unsigned captures; /* Fresh captured factories created by actual guest bind/capture. */
static unsigned destroyed; /* Original plus every captured factory must retire. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: Fail every native fixture contract explicitly without build-mode assertions.
 * Явно завершить каждый native fixture contract без build-mode assertions. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "guest capture provider: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Original node follows current mapping; captured nodes fix the chosen object identity.
 * Original node следует current mapping; captured nodes фиксируют выбранную object identity. */
typedef struct Node
{
    char fixed; /* Zero for original source, otherwise captured byte identity. */
    int directory; /* Root node kind. */
} Node;


/* EN/RU: File/directory metadata for real SDK operations, including a writable original-only ctl.
 * File/directory metadata для real SDK operations, включая writable original-only ctl. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    Node *node = opaque;
    int ctl = !node->fixed && !strcmp(path, "ctl");
    int directory = node->directory && (!strcmp(path, ".") || !strcmp(path, "dir") || (!node->fixed && !strcmp(path, "flip")));
    int file = node->directory ? !strcmp(path, "item") || !strcmp(path, "dir/item") ||
        !strcmp(path, "hidden") || !strcmp(path, "dir/hidden") : !strcmp(path, ".");
    if (!ctl && !directory && !file)
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = directory;
    out->mode = ctl ? 0222 : directory ? 0555 : 0444;
    out->identity.low = ctl ? 90 : node->fixed ? node->fixed : current;
    strcpy(out->name, ctl ? "ctl" : directory ? "dir" : "item");
    return FVM_OK;
}


/* EN/RU: Read from pinned node state; changing the original source never changes a captured identity.
 * Прочитать pinned node state; original source changes не меняют captured identity. */
static FVM_Result readfile(void *opaque, void *bytes, size_t size, size_t *count)
{
    Node *node = opaque;
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = node->fixed ? node->fixed : current;
    }
    return FVM_OK;
}


/* EN/RU: Original-only fixture control replaces source mapping; never returns a provider pointer.
 * Original-only fixture control меняет source mapping; provider pointer не возвращается. */
static FVM_Result replace(void *opaque, const void *bytes, size_t size)
{
    Node *node = opaque;
    if (node->fixed || size != 1 || *(const char *)bytes != 'B')
    {
        return FVM_ERROR;
    }
    current = 'B';
    return FVM_OK;
}


/* EN/RU: Inner stream borrows its factory; ordinary wrapper holds the lifetime reference.
 * Inner stream заимствует factory; ordinary wrapper удерживает lifetime reference. */
static void closefile(void *opaque)
{
    (void)opaque;
}
static const DVM_VfsOps readonly = {NULL, readfile, NULL, NULL, NULL, closefile}; /* Ordinary byte endpoint. */
static const DVM_VfsOps control = {NULL, NULL, replace, NULL, NULL, closefile}; /* Native fixture control only. */


/* EN/RU: Expose only real files; source ctl is not inherited by captured directory roots.
 * Выдать только real files; source ctl не наследуется captured directory roots. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    Node *node = opaque;
    FVM_FileInfo meta;
    int ctl = !node->fixed && !strcmp(path, "ctl");
    if (info(opaque, path, &meta) != FVM_OK || meta.directory || strcmp(mode, ctl ? "wo" : "r"))
    {
        return FVM_ERROR;
    }
    out->ops = ctl ? &control : &readonly;
    out->state = opaque;
    return FVM_OK;
}


/* EN/RU: Count actual factory retirement after every namespace/capability/FD borrower leaves.
 * Посчитать actual factory retirement после всех namespace/capability/FD borrowers. */
static void destroy(void *opaque)
{
    destroyed++;
    free(opaque);
}
static const DVM_ProviderOps operations = {info, openfile, destroy, 0, NULL}; /* Per-node capture, no global stable routing promise. */


/* EN/RU: Fresh capture is native-only and fixes the selected root before guest publication.
 * Fresh capture native-only и фиксирует выбранный root до guest publication. */
static DVM_Provider *capture(void *opaque, const char *path)
{
    FVM_FileInfo meta;
    if (info(opaque, path, &meta) != FVM_OK || !strcmp(path, "ctl"))
    {
        return NULL;
    }
    Node *node = calloc(1, sizeof(*node));
    if (!node)
    {
        return NULL;
    }
    node->fixed = (char)meta.identity.low;
    node->directory = !strcmp(path, "flip") ? !meta.directory : meta.directory;
    /* Deliberate path-kind replacement at the actual native capture boundary.
     * Намеренная замена kind пути на actual native capture boundary. */
    DVM_Provider *root = dvm_provider_create_capture(&operations, node, capture, 1);
    if (!root)
    {
        free(node);
        return NULL;
    }
    captures++;
    return root;
}


/* EN/RU: Install a real dynamic native provider in the guest namespace; no builtin substitution.
 * Установить настоящий dynamic native provider в namespace guest; без builtin substitution. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "source allocation");
    node->directory = 1;
    DVM_Provider *provider = dvm_provider_create_capture(&operations, node, capture, 0);
    DVM_NamespacePlan target;
    check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "remote", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/capture-source") == 1, "native provider installation");
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: After real GC/native close and startup namespace drains, every captured root and its pinned origin must retire.
 * После real GC/native close и startup namespace drain все captured roots и pinned origins должны завершиться. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(captures >= 4 && destroyed == captures+1, "captured provider/origin lifetime leak");
    puts("PASS guest captured provider native lifetimes drained");
}
