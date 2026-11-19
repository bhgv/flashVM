/* Copyright (c) 2026 bhgv
 * EN: Original raw Styx bytes survive the explicit S-record history envelope; framing checks never mutate the active search.
 * RU: Исходные raw Styx bytes сохраняются в S-record history envelope; framing checks не меняют active search.
 */
#include "packet.h"
#include "flashvm/srecord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Release-build invariant, message borrowed through failure reporting.
 * Инвариант в release build; message borrowed до failure reporting.
 */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "walk packet: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Round-trip captured native history and unchanged raw frames; validate truncations and scan one-bit mutations under the unchanged short-CRC contract.
 * Проверить captured history/raw frames, truncated prefixes и single-bit mutations при неизменном short-CRC contract.
 */
int main(void)
{
    unsigned long domain[4] = {11, 22, 33, 44};
    WalkEntity entity;
    WalkScope held = {0};
    check(walk_domain(domain) && walk_entity(&entity) &&
        walk_enter(&held, &entity, 0xfeedfaceUL, 0xabcdef01UL, WALK_OPEN, "dir/file", "r") == WALK_OK,
        "source state");
    const unsigned char frame[] = {9, 0, 0, 0, 108, 1, 0, 2, 0}; /* Unchanged original Tflush. */
    size_t length = 0;
    unsigned char *bytes = walk_packet_write(WALK_PACKET_REQUEST, 0xffffffffUL, NULL, frame, sizeof(frame), &length);
    WalkPacket packet;
    check(bytes && walk_packet_read(bytes, length, &packet) && packet.tag == 0xffffffffUL &&
        packet.kind == WALK_PACKET_REQUEST && packet.frame_size == sizeof(frame) &&
        !memcmp(frame, packet.frame, sizeof(frame)), "request envelope changed original frame");
    size_t original = walk_depth();
    for (size_t prefix = 0; prefix < length; prefix++)
    {
        memset(&packet, 0x5a, sizeof(packet));
        WalkPacket unchanged = packet;
        check(!walk_packet_read(bytes, prefix, &packet) && !memcmp(&packet, &unchanged, sizeof(packet)) &&
            walk_depth() == original, "truncated packet admitted/mutated output or search");
    }
    size_t rejected = 0; /* Truncated S-record CRC can collide; never strengthen it silently in one adapter. */
    for (size_t position = 0; position < length; position++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            bytes[position] ^= (unsigned char)(1U << bit);
            if (walk_packet_read(bytes, length, &packet))
            {
                FVM_SRecord control;
                FVM_SRecord data;
                size_t used;
                check(fvm_srecord_read(bytes + 4, length - 4, FVM_SRECORD_BODY_MAX, &control, &used) == FVM_S_OK &&
                    fvm_srecord_read(bytes + 4 + control.encoded_size, length - 4 - control.encoded_size,
                        FVM_SRECORD_BODY_MAX, &data, &used) == FVM_S_OK,
                    "adapter accepted a record rejected by the original shared CRC codec");
            }
            else
            {
                rejected++;
            }
            check(walk_depth() == original, "mutated packet changed active search");
            bytes[position] ^= (unsigned char)(1U << bit);
        }
    }
    check(rejected > length * 7, "mutation scan did not exercise CRC rejection");
    check(walk_packet_read(bytes, length, &packet) && walk_leave(&held), "restore valid source packet");
    WalkScope imported = {0};
    check(walk_import(&imported, packet.history, packet.history_size) == WALK_OK, "import copied envelope history");
    free(bytes);
    WalkScope repeated = {0};
    check(walk_enter(&repeated, &entity, 0xfeedfaceUL, 0xabcdef01UL, WALK_OPEN, "dir/file", "r") == WALK_CYCLE &&
        walk_leave(&imported) && !walk_depth(), "history aliased freed packet");
    bytes = walk_packet_write(WALK_PACKET_REPLY | WALK_PACKET_CYCLE, 9, &entity, frame, sizeof(frame), &length);
    check(bytes && walk_packet_read(bytes, length, &packet) && packet.kind == (WALK_PACKET_REPLY | WALK_PACKET_CYCLE) &&
        !memcmp(&packet.root, &entity, sizeof(entity)) && packet.history_size == 4,
        "fixed root/cycle reply acknowledgement");
    free(bytes);
    check(!walk_packet_write(WALK_PACKET_REQUEST | WALK_PACKET_CYCLE, 9, NULL, frame, sizeof(frame), &length) &&
        !walk_packet_write(WALK_PACKET_REQUEST, 0, NULL, frame, sizeof(frame), &length) &&
        !walk_packet_write(WALK_PACKET_REPLY, 9, NULL, frame, sizeof(frame), &length), "invalid control accepted");
    puts("PASS S-record envelope, original frames, truncation/mutation CRC checks and copied history");
    return 0;
}
