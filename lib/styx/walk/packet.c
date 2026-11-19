/* Copyright (c) 2026 bhgv
 * EN: Tag0 negotiates a direction-local Styx/history purpose; CRC-checked data retains the original frame codec.
 * RU: Tag0 согласует direction-local Styx/history purpose; CRC-checked data сохраняет исходный frame codec.
 */
#include "packet.h"
#include "internal.h"
#include "flashvm/srecord.h"
#include <stdlib.h>
#include <string.h>
#define WALK_CONTROL_BYTES 42U /* Magic/version/kind, assigned tag and eight identity limbs. */

/* EN/RU: Write one unsigned32 value explicitly little-endian into four caller bytes.
 * Записать unsigned32 явно little-endian в четыре caller bytes.
 */


/* [standalone-contract:put]
 * EN: put: checked ordinal prefixes around unchanged S-record bytes.
 * RU: put: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * bytes (unsigned char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * value (unsigned long value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void put(unsigned char *bytes, unsigned long value)
{
    for (unsigned i = 0; i < 4; i++)
    {
        bytes[i] = (unsigned char)(value >> (i * 8));
    }
}

/* EN/RU: Read unsigned32 little-endian from four validated caller bytes.
 * Прочитать unsigned32 little-endian из четырёх validated caller bytes.
 */


/* [standalone-contract:get]
 * EN: get: checked ordinal prefixes around unchanged S-record bytes.
 * RU: get: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * bytes (const unsigned char *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: unsigned long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned long get(const unsigned char *bytes)
{
    return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 | (unsigned long)bytes[2] << 16 |
           (unsigned long)bytes[3] << 24;
}

/* EN/RU: See packet.h; all temporary buffers are native scalar copies, freed before return on every outcome.
 * См. packet.h; temporary buffers — native scalar copies, освобождаются до return при любом исходе.
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
    unsigned kind, unsigned long tag, const WalkEntity *root, const void *frame, size_t frame_size, size_t *length)
{
    unsigned cycle = kind & WALK_PACKET_CYCLE;
    kind &= ~WALK_PACKET_CYCLE;
    if ((cycle && kind != WALK_PACKET_REPLY) || !frame || frame_size < 7 || frame_size > 1048576U || !length || !tag ||
        tag > 0xffffffffUL || (kind != WALK_PACKET_REQUEST && kind != WALK_PACKET_REPLY) || get(frame) != frame_size ||
        (kind == WALK_PACKET_REPLY && (!root || !walk_words(root->domain, 1) || !walk_words(root->object, 1))))
    {
        return NULL;
    }
    size_t measured = kind == WALK_PACKET_REPLY ? 4 : walk_export_size();
    if (!measured)
    {
        return NULL;
    }
    size_t capacity = frame_size + measured + 128U;
    unsigned char *control = calloc(1, WALK_CONTROL_BYTES + measured);
    unsigned char *packet = malloc(capacity);
    size_t history = 0;
    if (!control || !packet)
    {
        free(control);
        free(packet);
        return NULL;
    }
    memcpy(control, "VWH1", 4);
    control[4] = (unsigned char)kind;
    control[5] = cycle != 0;
    put(control + 6, tag);
    if (kind == WALK_PACKET_REPLY)
    {
        for (unsigned i = 0; i < 4; i++)
        {
            put(control + 10 + i * 4, root->domain[i]);
            put(control + 26 + i * 4, root->object[i]);
        }
        control[WALK_CONTROL_BYTES] = 1; /* Empty scalar history v1, never unrelated server thread state. */
        history = 4;
    }
    else if (!walk_export(control + WALK_CONTROL_BYTES, measured, &history))
    {
        free(control);
        free(packet);
        return NULL;
    }
    size_t first = 0;
    size_t second = 0;
    int valid =
        fvm_srecord_write(0, control, WALK_CONTROL_BYTES + history, packet + 4, capacity - 4, &first) == FVM_S_OK &&
        fvm_srecord_write(tag, frame, frame_size, packet + 4 + first, capacity - 4 - first, &second) == FVM_S_OK;
    free(control);
    if (!valid)
    {
        free(packet);
        return NULL;
    }
    put(packet, 4 + first + second);
    *length = 4 + first + second;
    return packet;
}

/* EN/RU: See packet.h; bytewise geometry/CRC checks reject malformed controls instead of scanning or falling back to raw 9P.
 * См. packet.h; bytewise geometry/CRC checks отказывают malformed controls без scanning/raw 9P fallback.
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
int walk_packet_read(const void *input, size_t length, WalkPacket *output)
{
    const unsigned char *bytes = input;
    if (!bytes || !output || length < 4 || length > 1048576U + WALK_PACKET_OVERHEAD || get(bytes) != length)
    {
        return 0;
    }
    FVM_SRecord control;
    FVM_SRecord data;
    size_t consumed = 0;
    if (fvm_srecord_read(bytes + 4, length - 4, FVM_SRECORD_BODY_MAX, &control, &consumed) != FVM_S_OK || control.tag ||
        control.size < WALK_CONTROL_BYTES + 4 || memcmp(control.payload, "VWH1", 4) || control.payload[5] > 1 ||
        (control.payload[4] != WALK_PACKET_REQUEST && control.payload[4] != WALK_PACKET_REPLY) ||
        fvm_srecord_read(bytes + 4 + control.encoded_size,
                         length - 4 - control.encoded_size,
                         FVM_SRECORD_BODY_MAX,
                         &data,
                         &consumed) != FVM_S_OK ||
        control.encoded_size + data.encoded_size != length - 4 || data.size < 7 || data.size > 1048576U ||
        get(data.payload) != data.size || !data.tag || data.tag != get(control.payload + 6))
    {
        return 0;
    }
    WalkPacket packet = {0};
    packet.tag = data.tag;
    packet.kind = control.payload[4];
    if (control.payload[5])
    {
        if (packet.kind != WALK_PACKET_REPLY)
        {
            return 0;
        }
        packet.kind |= WALK_PACKET_CYCLE;
    }
    for (unsigned i = 0; i < 4; i++)
    {
        packet.root.domain[i] = get(control.payload + 10 + i * 4);
        packet.root.object[i] = get(control.payload + 26 + i * 4);
    }
    if ((packet.kind & ~WALK_PACKET_CYCLE) == WALK_PACKET_REPLY &&
        (!walk_words(packet.root.domain, 1) || !walk_words(packet.root.object, 1)))
    {
        return 0;
    }
    packet.history = control.payload + WALK_CONTROL_BYTES;
    packet.history_size = control.size - WALK_CONTROL_BYTES;
    if (packet.history_size > WALK_WIRE_LIMIT || ((packet.kind & ~WALK_PACKET_CYCLE) == WALK_PACKET_REPLY &&
                                                  (packet.history_size != 4 || memcmp(packet.history, "\1\0\0\0", 4))))
    {
        return 0;
    }
    packet.frame = data.payload;
    packet.frame_size = data.size;
    *output = packet;
    return 1;
}
