/* Copyright (c) 2026 bhgv
 * EN: Provider-independent dispatch, unsupported operations and linear close oracle.
 * RU: Provider-independent dispatch, unsupported operations и linear close oracle.
 */
#include "handle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Provider state owned by main; counters expose exact dispatch/close.
 * State принадлежит main; counters показывают точные dispatch/close. */
typedef struct Probe
{
    unsigned reads; /* Successful provider calls, distinct from rejected dispatch. */
    unsigned closes; /* Consuming calls; must equal one after move/repeated close. */
} Probe;


/* EN/RU: Fail condition with label; no cleanup is needed for stack-only state.
 * При отказе condition завершить с label; stack-only state не требует cleanup. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "VFS handle: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Borrow Probe and output; write one byte and acknowledge one read.
 * Заимствовать Probe/output; записать один byte, учесть один read. */
static FVM_Result read_probe(void *state, void *bytes, size_t size, size_t *received)
{
    Probe *probe = state;
    if (!bytes || !size || !received)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *(char *)bytes = 'P';
    *received = 1;
    probe->reads++;
    return FVM_OK;
}


/* EN/RU: Consume the logical provider owner, leaving stack storage to main.
 * Поглотить logical owner provider; stack storage остаётся у main. */
static void close_probe(void *state)
{
    Probe *probe = state;
    probe->closes++;
}

static const DVM_VfsOps probe_ops = {NULL, read_probe, NULL, NULL, NULL, close_probe}; /* Trusted private test provider. */


/* EN/RU: Require real callback dispatch without any file/directory type knowledge.
 * Validate refused operations preserve outputs and ownership; zero on success.
 * Проверить dispatch без знания file/directory type, сохранность outputs и
 * владения при отказах. Ноль при успехе; внешних ресурсов нет. */
int main(void)
{
    Probe probe = {0, 0};
    DVM_VfsHandle source = {&probe_ops, &probe};
    DVM_VfsHandle target = {0};
    DVM_VfsHandle invalid = {&probe_ops, NULL};
    FVM_FileInfo info;
    FVM_FileInfo before;
    size_t received = 99;
    long position = 77;
    char byte = 'X';
    memset(&info, 0x5a, sizeof(info));
    before = info;
    require(dvm_vfs_info(&source, &info) == FVM_UNSUPPORTED && !memcmp(&info, &before, sizeof(info)),
            "unsupported stat changed output");
    require(dvm_vfs_seek(&source, 0, 0, &position) == FVM_UNSUPPORTED && position == 77,
            "unsupported seek changed output");
    require(dvm_vfs_read(&invalid, &byte, 1, &received) == FVM_INVALID_ARGUMENT && received == 99 && byte == 'X',
            "invalid dispatch changed output");
    require(!dvm_vfs_move(&source, &target) && source.state == &probe && !target.state,
            "occupied destination accepted");
    require(!dvm_vfs_move(&target, &invalid) && !target.state && invalid.ops == &probe_ops,
            "invalid source moved");
    require(dvm_vfs_move(&target, &source) && !source.ops && !source.state, "source not cleared");
    require(dvm_vfs_read(&target, &byte, 1, &received) == FVM_OK && byte == 'P' && received == 1 && probe.reads == 1,
            "provider read dispatch");
    require(!dvm_vfs_file(&target) && !dvm_vfs_directory(&target), "custom state exposed as native handle");
    dvm_vfs_close(&source);
    dvm_vfs_close(&target);
    dvm_vfs_close(&target);
    require(probe.closes == 1 && !target.ops && !target.state, "duplicate or missing close");
    puts("PASS VFS provider dispatch, unsupported outputs, move refusal and exactly-once close");
    return 0;
}
