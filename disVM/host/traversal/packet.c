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
static unsigned long get(const unsigned char *bytes)
{
    return (unsigned long)bytes[0] | (unsigned long)bytes[1] << 8 |
        (unsigned long)bytes[2] << 16 | (unsigned long)bytes[3] << 24;
}


/* EN/RU: See packet.h; all temporary buffers are native scalar copies, freed before return on every outcome.
 * См. packet.h; temporary buffers — native scalar copies, освобождаются до return при любом исходе.
 */
void *walk_packet_write(unsigned kind, unsigned long tag, const WalkEntity *root,
    const void *frame, size_t frame_size, size_t *length)
{
    unsigned cycle = kind & WALK_PACKET_CYCLE;
    kind &= ~WALK_PACKET_CYCLE;
    if ((cycle && kind != WALK_PACKET_REPLY) || !frame || frame_size < 7 || frame_size > 1048576U || !length || !tag || tag > 0xffffffffUL ||
        (kind != WALK_PACKET_REQUEST && kind != WALK_PACKET_REPLY) || get(frame) != frame_size ||
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
    int valid = fvm_srecord_write(0, control, WALK_CONTROL_BYTES + history, packet + 4, capacity - 4, &first) == FVM_S_OK &&
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
        fvm_srecord_read(bytes + 4 + control.encoded_size, length - 4 - control.encoded_size,
            FVM_SRECORD_BODY_MAX, &data, &consumed) != FVM_S_OK ||
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
    if ((packet.kind & ~WALK_PACKET_CYCLE) == WALK_PACKET_REPLY && (!walk_words(packet.root.domain, 1) || !walk_words(packet.root.object, 1)))
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
