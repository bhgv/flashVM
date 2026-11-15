/* Copyright (c) 2026 bhgv
 * EN: Event-file controls, independent FDs, access revocation and ticket races.
 * RU: Event-file controls, независимые FD, access revocation и ticket races.
 */
#include "eventfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Main-owned source policy, retained until all projection handles close.
 * Source policy принадлежит main и живёт до закрытия projection handles. */
typedef struct Source
{
    int allowed; /* Current effective READ permission, checked on each operation. */
    unsigned destroyed; /* Final source factory cleanup count. */
} Source;


/* EN/RU: Explicit test failure with native label, independent of assertion modes.
 * Явный test failure с label, без assertion modes. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "eventfs: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Only source root READ authority is usable by the projection.
 * Projection использует только source root READ authority. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    Source *source = state;
    require(!strcmp(path, ".") && access == FVM_ACCESS_READ, "source write authority requested");
    return source->allowed ? FVM_OK : FVM_ERROR;
}


/* EN/RU: No source metadata is needed for event delivery; forbid hidden stat.
 * Для event delivery source metadata не нужна; запретить скрытый stat. */
static FVM_Result info_refused(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    (void)path;
    (void)output;
    require(0, "event operation queried source metadata");
    return FVM_ERROR;
}


/* EN/RU: Source I/O is never opened by projection construction or observer I/O.
 * Projection не открывает source I/O при construction/observer I/O. */
static FVM_Result open_refused(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)state;
    (void)path;
    (void)mode;
    (void)output;
    require(0, "event operation opened source file");
    return FVM_ERROR;
}


/* EN/RU: Count final factory release without freeing main-owned storage.
 * Посчитать final release, не освобождая main-owned storage. */
static void destroy(void *state)
{
    Source *source = state;
    source->destroyed++;
}
static const DVM_ProviderOps source_ops = {info_refused, open_refused, destroy, 0, access_check}; /* Deliberately no source I/O. */


/* EN/RU: Read exactly one complete bounded frame and return it as native text.
 * Прочитать одну complete bounded frame; вернуть native text без ack. */
static void read_frame(DVM_VfsHandle *handle, char *text)
{
    DVM_IoResult result = dvm_vfs_read_chunk(handle, text, DVM_EVENT_FRAME_BYTES - 1);
    require(result.status == FVM_OK && result.count > 0 && result.count < DVM_EVENT_FRAME_BYTES && (result.flags & DVM_IO_RECORD), "frame read");
    text[result.count] = 0;
}


/* EN/RU: Extract FD-local token from a trusted generated event frame.
 * Извлечь FD-local token из trusted generated frame. */
static void acknowledge(DVM_VfsHandle *handle, const char *text, int success)
{
    char command[32];
    DVM_IoResult result;
    require(!strncmp(text, "event ", 6), "expected event frame");
    memcpy(command, "ack ", 4);
    memcpy(command + 4, text + 6, DVM_EVENT_TOKEN_HEX);
    command[4 + DVM_EVENT_TOKEN_HEX] = '\n';
    result = dvm_vfs_write_chunk(handle, command, 5 + DVM_EVENT_TOKEN_HEX);
    require((result.status == FVM_OK) == success, "ack result");
}


/* EN/RU: Publish contiguous source event, keeping raw pointer owned by source ref.
 * Опубликовать contiguous event; source ref удерживает raw pointer journal. */
static void notify(DVM_Provider *source, uint64_t generation, unsigned kinds)
{
    DVM_ChangeEvent event = {1, generation, kinds, DVM_CHANGE_RESOURCE, 0};
    require(dvm_changes_notify(dvm_provider_changes(source), &event) == 1, "notify");
}


/* EN/RU: Exercise the actual factory and generic I/O paths, not a copied protocol.
 * Проверить actual factory/generic I/O, не копию protocol implementation. */
int main(void)
{
    Source state = {1, 0};
    DVM_Provider *source = dvm_provider_create(&source_ops, &state);
    DVM_Provider *projection = dvm_provider_eventfs(source);
    DVM_VfsHandle first = {0}, second = {0}, partial = {0}, directory = {0}, refused = {0};
    DVM_IoResult result;
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    long position;
    char text[DVM_EVENT_FRAME_BYTES], saved[DVM_EVENT_FRAME_BYTES], prefix[5];
    require(source && projection && dvm_provider_bindable(projection), "projection construction");
    require(dvm_provider_open(projection, "events", "r", &refused) == FVM_UNSUPPORTED && !refused.ops, "read-only control FD accepted");
    require(dvm_provider_open(projection, "events", "rw", &first) == FVM_OK &&
            dvm_provider_open(projection, "events", "rw", &second) == FVM_OK &&
            dvm_provider_open(projection, "events", "rw", &partial) == FVM_OK, "independent observers");
    require(dvm_provider_open(projection, ".", "r", &directory) == FVM_OK &&
            dvm_vfs_list(&directory, &entries, &count) == FVM_OK && count == 1 && !strcmp(entries[0].name, "events"), "root listing");
    fvm_port_directory_entries_free(entries);
    entries = NULL;
    require(dvm_vfs_list(&directory, &entries, &count) == FVM_OK && !count && !entries, "root EOF");
    read_frame(&first, text);
    require(!strcmp(text, "event 0000000000000001 0000000000000001 0000000000000001 63 0 0000000000000000\n"), "initial wire record");
    strcpy(saved, text);
    acknowledge(&first, text, 1);
    result = dvm_vfs_read_chunk(&first, text, sizeof(text));
    require(result.status != FVM_OK && !result.count && result.stop == DVM_IO_AGAIN, "empty observer became EOF/event");
    read_frame(&second, text);
    require(!strcmp(text, saved), "one observer cleared another");
    result = dvm_vfs_read_chunk(&partial, prefix, sizeof(prefix));
    require(result.status != FVM_OK && !result.count, "short buffer split/consumed record");
    acknowledge(&partial, saved, 0);
    require(dvm_vfs_seek(&partial, 0, FVM_FILE_SEEK_START, &position) == FVM_OK, "partial rewind");
    read_frame(&partial, text);
    require(!strcmp(text, saved), "rewind replaced pending ticket");
    acknowledge(&partial, text, 1);
    notify(source, 2, DVM_CHANGE_RIGHTS);
    read_frame(&first, text);
    strcpy(saved, text);
    notify(source, 3, DVM_CHANGE_CONTENT);
    acknowledge(&first, text, 1);
    read_frame(&first, text);
    require(strstr(text, " 4 1 ") != NULL, "old ack swallowed new event");
    acknowledge(&first, saved, 0);
    result = dvm_vfs_write_chunk(&first, "mask 0\n", 7);
    require(result.status != FVM_OK, "invalid mask accepted");
    acknowledge(&first, text, 1);
    result = dvm_vfs_write_chunk(&first, "mask 1\n", 7);
    require(result.status == FVM_OK && result.count == 7, "mask control");
    result = dvm_vfs_read_chunk(&first, text, sizeof(text));
    require(result.status != FVM_OK && !result.count && result.stop == DVM_IO_AGAIN, "mask discarded acknowledged state");
    notify(source, 4, DVM_CHANGE_RIGHTS);
    read_frame(&first, text);
    state.allowed = 0;
    result = dvm_vfs_read_chunk(&first, prefix, sizeof(prefix));
    require(result.status != FVM_OK && result.stop == DVM_IO_DENIED && !result.count, "revoked read succeeded");
    acknowledge(&first, text, 0);
    state.allowed = 1;
    require(dvm_changes_reconnect(dvm_provider_changes(source), 1) == 2, "new epoch");
    acknowledge(&first, text, 0);
    read_frame(&first, text);
    require(strstr(text, " 0000000000000002 0000000000000001 1 0 ") != NULL, "stale ack cleared new epoch");
    acknowledge(&first, text, 1);
    dvm_provider_release(projection);
    dvm_provider_release(source);
    require(!state.destroyed, "source destroyed while observers live");
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    dvm_vfs_close(&partial);
    dvm_vfs_close(&directory);
    require(state.destroyed == 1, "source lifetime leak");
    puts("PASS event files independent/partial/ack/mask/race/revocation/reconnect/lifetime and no source stat");
    return 0;
}
