/* Copyright (c) 2026 bhgv
 * EN: Explicit S-record traversal transport envelope around unchanged original Styx frames.
 * RU: Явный S-record traversal transport envelope вокруг неизменённых исходных Styx frames.
 */
#ifndef VFS_WALK_PACKET_H
#define VFS_WALK_PACKET_H
#include "walk.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WALK_PACKET_OVERHEAD (WALK_WIRE_LIMIT + 128U) /* Bounded control history plus S-record overhead. */
#define WALK_PACKET_REQUEST 1U /* Tag0 assigns the selected direction-local data tag and carries active history. */
#define WALK_PACKET_CYCLE 256U /* Reply-only proven cycle annotation, independent of textual Rerror vocabulary. */
#define WALK_PACKET_REPLY 2U /* Tag0 acknowledges assignment and advertises this granted server root identity. */


/* EN/RU: Verified borrowed views; original bytes live through use/import, no native structure on wire.
 * Проверенные borrowed views; original bytes живут до use/import, native structure не передаётся.
 */
typedef struct WalkPacket
{
    unsigned long tag; /* Direction-local data tag assigned/acknowledged by this packet's tag0 control record. */
    WalkEntity root; /* Reply's trusted granted-root description, never an authorization credential. */
    const void *history; /* Borrowed validated scalar-history byte span; import separately before provider callbacks. */
    size_t history_size; /* Exact history extent. */
    const void *frame; /* Borrowed original 9P frame, unchanged. */
    size_t frame_size; /* Complete frame length including original 9P size header. */
    unsigned kind; /* REQUEST or REPLY, never inferred from user payload. */
} WalkPacket;


/* EN/RU: Build owned transport envelope/NULL with little-endian size and two checked S-records; caller frees result.
 * Request captures this caller's active history now; reply advertises borrowed nonzero root and empty history.
 * Frame/tag/root inputs borrowed through return; no provider callbacks/VM/code execution. length written on success only.
 * Создать owned envelope/NULL с size и двумя checked S-records; caller освобождает. Request снимает active history,
 * reply объявляет nonzero root и пустую history; inputs borrowed до return, length лишь при success.
 */
void *walk_packet_write(unsigned kind, unsigned long tag, const WalkEntity *root,
    const void *frame, size_t frame_size, size_t *length);


/* EN/RU: Verify complete envelope/control/data CRCs, canonical record boundaries and bounded raw 9P geometry;1/0.
 * input/output borrowed through return; output unchanged on failure. No history scope or operation is admitted here.
 * Проверить envelope/control/data CRC и canonical boundaries/raw 9P geometry;1/0, output неизменен при failure.
 */
int walk_packet_read(const void *input, size_t length, WalkPacket *output);
#ifdef __cplusplus
}
#endif
#endif
