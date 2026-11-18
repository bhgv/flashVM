/* Copyright (c) 2026 bhgv
 * EN: One-request resource reply state, reusable after either Styx framing or Unibus typed routing.
 * RU: Состояние ответа одного ресурса после Styx framing либо типизированной маршрутизации Unibus.
 */
#ifndef STYX_MOVIE_REPLY_H
#define STYX_MOVIE_REPLY_H
#include "protocol.h"
#include "managed_source.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MOVIE_REPLY_MORE 1 /* One accepted BEGIN/BYTES, successful END still required. */
#define MOVIE_REPLY_DONE 2 /* Explicit successful END accepted and resolved path published. */
#define MOVIE_REPLY_FAILED 0 /* Sticky malformed reply, failed END or sink refusal. */


/* EN/RU: Caller-owned single-native-owner state; do not copy/edit fields after initialization.
 * No wire struct, allocation or VM entry. Sink/output storage lives through all accept calls.
 * Состояние одного native owner, не копировать/менять после init; не wire struct, без allocation/VM.
 */
typedef struct MovieReply
{
    FVM_DesktopWrite write; /* Borrowed native staging sink; never a foreign VM callback. */
    void *state; /* Borrowed sink context, unchanged through final accept. */
    char *resolved; /* Borrowed output, published only after successful END. */
    size_t capacity; /* Writable output capacity including trailing NUL. */
    size_t bytes; /* Accepted payload bytes, overflow checked before calling sink. */
    size_t path_size; /* Private canonical absolute origin length, excluding trailing NUL. */
    char path[MOVIE_PATH]; /* Private bounded origin, not published before successful END. */
    int stage; /* 0 waiting BEGIN, 1 streaming, 2 complete, -1 failed. */
    FVM_Result result; /* Sink failure preserved; otherwise ERROR until complete. */
} MovieReply;


/* EN: Initialize borrowed reply/sink/state/output; 1 success/0 invalid, no output mutation or I/O.
 * Sink must stage bytes: accepting a chunk is not committing the resource. Caller owns total-size
 * quota and rollback. This state imposes per-chunk and path bounds, not an application size limit.
 * State/output/input buffers must not overlap. A returned origin is metadata, not a VFS grant.
 * RU: Инициализировать заимствованные reply/sink/state/output; 1/0, без I/O и изменения output.
 * Sink сохраняет staging, а не публикует ресурс; общий quota и откат принадлежат caller.
 */
int movie_reply_init(MovieReply *reply, FVM_DesktopWrite write, void *state, char *resolved, size_t capacity);


/* EN: Consume exactly one borrowed MOVIE_TYPE/MOVIE_VERSION payload on the sink's native owner.
 * Caller already validated framing/checksum and agreed type/version; no tags or bindings inferred.
 * Return MORE/DONE/FAILED; no I/O except synchronous sink callback for a valid BYTES record.
 * Enforce BEGIN/BYTES/END ordering, canonical bounded path, 1..MOVIE_CHUNK data and END 0/1.
 * No callback on malformed input; failures are sticky, sink errors preserved, no replay/retry.
 * Calling again after DONE refuses and invalidates success; consumers must stop on terminal result.
 * The already published origin is not erased if a caller violates that terminal-input rule.
 * RU: Принять один payload проверенного типа/версии на native owner sink. Вернуть MORE/DONE/FAILED.
 * Проверить порядок, путь, размер чанка и END; malformed не вызывает sink, отказ sticky, без replay.
 * После DONE новые записи отклоняются; caller прекращает приём при terminal result.
 */
int movie_reply_accept(MovieReply *reply, const void *bytes, size_t size);


/* EN/RU: Return OK only after successful END; preserve sink errors, ERROR for pending/invalid/failure.
 * Без I/O: OK только после успешного END, ошибки sink сохраняются; pending/invalid даёт ERROR.
 */
FVM_Result movie_reply_result(const MovieReply *reply);
#ifdef __cplusplus
}
#endif
#endif
