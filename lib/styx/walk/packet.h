/* Copyright (c) 2026 bhgv
 * EN: Explicit S-record traversal transport envelope around unchanged original Styx frames.
 * RU: Явный S-record traversal transport envelope вокруг неизменённых исходных Styx frames.
 */
#ifndef VFS_WALK_PACKET_H
#define VFS_WALK_PACKET_H
#include "walk.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define WALK_PACKET_OVERHEAD (WALK_WIRE_LIMIT + 128U) /* Bounded control history plus S-record overhead. */
#define WALK_PACKET_REQUEST 1U /* Tag0 assigns the selected direction-local data tag and carries active history. */
#define WALK_PACKET_CYCLE 256U /* Reply-only proven cycle annotation, independent of textual Rerror vocabulary. */
#define WALK_PACKET_REPLY 2U   /* Tag0 acknowledges assignment and advertises this granted server root identity. */

/* EN/RU: Verified borrowed views; original bytes live through use/import, no native structure on wire.
 * Проверенные borrowed views; original bytes живут до use/import, native structure не передаётся.
 */
typedef struct WalkPacket
{
    unsigned long tag;   /* Direction-local data tag assigned/acknowledged by this packet's tag0 control record. */
    WalkEntity root;     /* Reply's trusted granted-root description, never an authorization credential. */
    const void *history; /* Borrowed validated scalar-history byte span; import separately before provider callbacks. */
    size_t history_size; /* Exact history extent. */
    const void *frame;   /* Borrowed original 9P frame, unchanged. */
    size_t frame_size;   /* Complete frame length including original 9P size header. */
    unsigned kind;       /* REQUEST or REPLY, never inferred from user payload. */
} WalkPacket;

/* EN/RU: Build owned transport envelope/NULL with little-endian size and two checked S-records; caller frees result.
 * Request captures this caller's active history now; reply advertises borrowed nonzero root and empty history.
 * Frame/tag/root inputs borrowed through return; no provider callbacks/VM/code execution. length written on success only.
 * Создать owned envelope/NULL с size и двумя checked S-records; caller освобождает. Request снимает active history,
 * reply объявляет nonzero root и пустую history; inputs borrowed до return, length лишь при success.
 */


/* [standalone-contract:walk_packet_write]
 * EN: walk_packet_write: checked ordinal prefixes around unchanged S-record bytes.
 * RU: walk_packet_write: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * kind (unsigned kind): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * root (const WalkEntity *root): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame (const void *frame): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * frame_size (size_t frame_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * length (size_t *length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: void *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *walk_packet_write(
    unsigned kind, unsigned long tag, const WalkEntity *root, const void *frame, size_t frame_size, size_t *length);


/* EN/RU: Verify complete envelope/control/data CRCs, canonical record boundaries and bounded raw 9P geometry;1/0.
 * input/output borrowed through return; output unchanged on failure. No history scope or operation is admitted here.
 * Проверить envelope/control/data CRC и canonical boundaries/raw 9P geometry;1/0, output неизменен при failure.
 */


/* [standalone-contract:walk_packet_read]
 * EN: walk_packet_read: checked ordinal prefixes around unchanged S-record bytes.
 * RU: walk_packet_read: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * length (size_t length): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * output (WalkPacket *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int walk_packet_read(const void *input, size_t length, WalkPacket *output);
#ifdef __cplusplus
}
#endif
#endif
