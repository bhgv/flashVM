/* Copyright (c) 2026 bhgv
 * EN: Frozen export mapping state and per-open policy-enforcing streams.
 * RU: Frozen export mapping state и per-open policy-enforcing streams.
 */
#ifndef DVM_VFS_VIEW_INTERNAL_H
#define DVM_VFS_VIEW_INTERNAL_H
#include "export.h"
#include "channel_sync.h"
#define DVM_VIEW_RULE_LIMIT 256U /* Explicit mappings bound tree-composition storage. */


/* EN/RU: Immutable frozen mapping; a NULL provider is an explicit hiding rule.
 * Immutable mapping; NULL provider — explicit hiding rule. */
typedef struct DVM_ViewRule
{
    struct DVM_ViewRule *next; /* Owned builder/factory list. */
    DVM_NoticeTap *tap; /* Optional source change forwarding subscription, detached at factory close. */
    DVM_Provider *source; /* Retained delegated factory, NULL hides. */
    char path[DVM_NS_PATH_LIMIT]; /* Canonical absolute virtual prefix. */
    char relative[DVM_NS_PATH_LIMIT]; /* Canonical source-relative base. */
    unsigned access; /* Per-node access ceiling. */
    unsigned flags; /* TREE delegates descendants, otherwise exact only. */
} DVM_ViewRule;


/* EN/RU: Builder becomes immutable factory state after freeze; policy context may
 * independently change its access decisions using its own native synchronization.
 * Builder становится immutable factory state; policy синхронизирует mutable decisions сама. */
struct DVM_View
{
    FVM_PortSync *lock; /* Protects refs, closed flag and output-journal lifetime. */
    size_t references; /* Factory/builder plus each admitted source subscription context. */
    int closed; /* Prevents late admitted source callbacks from accessing retired journal. */
    DVM_Changes *journal; /* Borrowed from live factory, cleared before factory destruction. */
    DVM_ViewRule *rules; /* Owned mappings. */
    size_t count; /* Number of mapping nodes, bounded by RULE_LIMIT. */
    DVM_ViewPolicy policy; /* Optional per-operation policy, no VM callbacks. */
    void *context; /* Policy state owned until release. */
    void (*release)(void *); /* Final context cleanup outside namespace/provider locks. */
};


/* EN/RU: Canonical relative/absolute path validation; root dot/slash supported.
 * Проверить canonical path, dot/slash root;1/0, no allocation. */
int dvm_view_path(const char *path, int absolute);


/* EN/RU: Find most-specific visible route; return1 mapped,2 synthetic dir,0 hidden.
 * Native-only no I/O. Underlying relative output is bounded; rule borrowed.
 * Найти visible route:1 mapping,2 synthetic dir,0 hidden; bounded relative output. */
int dvm_view_route(DVM_View *view, const char *path, unsigned access, DVM_ViewRule **rule, char *relative);


/* EN/RU: Test whether explicit positive mappings require a virtual parent directory.
 * Проверить, нужны ли virtual parent directory для positive mappings; no I/O. */
int dvm_view_children(DVM_View *view, const char *path);


/* EN/RU: Mask captured metadata without resolving its old source path again.
 * Маскировать захваченные metadata без повторного разрешения source path. */
int viewmask(DVM_View *view, const char *path, FVM_FileInfo *output);


/* EN/RU: Query mapped/synthetic metadata with live policy and masked mode.
 * Запросить metadata через live policy, маскируя mode; native I/O вне locks. */
FVM_Result dvm_view_info(DVM_View *view, const char *path, FVM_FileInfo *output);


/* EN/RU: Open independent stream enforcing policy before each operation.
 * Открыть stream с policy check перед каждой операцией; output empty. */
FVM_Result dvm_view_open(DVM_View *view, const char *path, const char *mode, DVM_VfsHandle *output);


/* EN/RU: Build one filtered directory batch, overlaying explicit mapped children.
 * Построить filtered directory batch с explicit mapped children; native owned entries. */
FVM_Result dvm_view_list(DVM_View *view, const char *path, DVM_VfsHandle *inner, int explicit_done,
                         FVM_FileInfo **entries, size_t *count);


/* EN/RU: Drop one native view-state reference; no provider pointer required by policy proxy.
 * Поглотить native view-state reference; policy proxy не требует provider pointer. */
void dvm_view_drop(void *state);


/* EN/RU: Capture callback rebases a subtree, anchors its selected source and preserves explicit rules/policy.
 * Capture callback переносит subtree, удерживает selected source и сохраняет explicit rules/policy. */
DVM_Provider *dvm_view_capture(void *state, const char *relative);
#endif
