/* Copyright (c) 2026 bhgv
 * EN: Actual pinned Inferno C codecs against independent 9P2000 wire vectors.
 * RU: Actual pinned Inferno C codecs против independent 9P2000 wire vectors.
 */
#include "lib9.h"
#include "fcall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Fail a wire contract explicitly, preserving every tested call.
 * Явно завершить wire contract test; все вызовы выполняются всегда. */
static void require(int ok, const char *text)
{
    if (!ok)
    {
        fprintf(stderr, "Inferno C codec: %s\n", text);
        exit(2);
    }
}


/* EN/RU: Full32-bit fids and UTF-8 names must match fixed independent wire bytes.
 * Full32-bit fids и UTF-8 names должны совпасть с fixed wire bytes. */
static void walk_vector(void)
{
    static const uchar expected[] = {
        34, 0, 0, 0, 110, 0x34, 0x12, 0x67, 0x45, 0x23, 0xf1,
        1, 0, 1, 0x80, 2, 0, 5, 0, 'f', 'i', 'r', 's', 't',
        8, 0, 'l', 'a', 't', 'e', 'r', '-', 0xce, 0xbb
    }; /* 9P2000 Twalk, not a serialization of a file tree. */
    uchar wire[128], copy[128];
    Fcall input = {0}, decoded;
    input.type = Twalk;
    input.tag = 0x1234;
    input.fid = 0xf1234567U;
    input.newfid = 0x80010001U;
    input.nwname = 2;
    input.wname[0] = "first";
    input.wname[1] = "later-\xce\xbb";
    require(sizeS2M(&input) == sizeof(expected) && convS2M(&input, wire, sizeof(wire)) == sizeof(expected) &&
            !memcmp(wire, expected, sizeof(expected)), "Twalk wire encoding");
    memcpy(copy, wire, sizeof(expected));
    require(convM2S(copy, sizeof(expected), &decoded) == sizeof(expected) && decoded.type == Twalk &&
            decoded.tag == input.tag && decoded.fid == input.fid && decoded.newfid == input.newfid &&
            decoded.nwname == 2 && !strcmp(decoded.wname[0], "first") &&
            !strcmp(decoded.wname[1], "later-\xce\xbb"), "Twalk decoding lost fid/name bits");
    for (uint size = 0; size < sizeof(expected); size++)
    {
        memcpy(copy, wire, sizeof(expected));
        require(convM2S(copy, size, &decoded) == 0, "truncated Twalk accepted");
    }
}


/* EN/RU: Tread uses existing fid, full64-bit offset and count, without any walk payload.
 * Tread использует existing fid, full64-bit offset/count, без walk payload. */
static void read_vector(void)
{
    uchar wire[64];
    Fcall input = {0}, decoded;
    input.type = Tread;
    input.tag = 7;
    input.fid = 0x80010001U;
    input.offset = 0x123456789abcdefLL;
    input.count = 0x10203040U;
    require(convS2M(&input, wire, sizeof(wire)) == 23 && wire[4] == Tread &&
            GBIT32(wire + 7) == input.fid && GBIT64(wire + 11) == (uvlong)input.offset &&
            GBIT32(wire + 19) == input.count, "Tread fixed wire fields");
    require(convM2S(wire, 23, &decoded) == 23 && decoded.type == Tread && decoded.fid == input.fid &&
            decoded.offset == input.offset && decoded.count == input.count, "Tread roundtrip");
}


/* EN/RU: A directory Rread carries complete stat records with64-bit qids and metadata.
 * Directory Rread содержит complete stat records с64-bit qids и metadata. */
static void directory_record(void)
{
    uchar record[512], packet[1024];
    char strings[512];
    Dir input = {0}, decoded;
    Fcall read = {0}, response;
    input.type = 0x1234;
    input.dev = 0x89abcdefU;
    input.qid.path = 0xfedcba9876543210ULL;
    input.qid.vers = 0xabcdef12U;
    input.qid.type = QTDIR;
    input.mode = DMDIR | 0750;
    input.atime = 0xf1234567U;
    input.mtime = 0x81234567U;
    input.length = 0x123456789abcdefLL;
    input.name = "folder-\xce\xbb";
    input.uid = "alice";
    input.gid = "group";
    input.muid = "bob";
    uint size = convD2M(&input, record, sizeof(record));
    require(size == sizeD2M(&input) && size > STATFIXLEN && statcheck(record, size) == 0, "directory stat encoding");
    require(GBIT64(record + 13) == input.qid.path && GBIT32(record + 21) == (u32int)input.mode,
            "stat wire offsets/width");
    read.type = Rread;
    read.tag = 9;
    read.count = size;
    read.data = (char *)record;
    uint bytes = convS2M(&read, packet, sizeof(packet));
    require(bytes == size + 11 && convM2S(packet, bytes, &response) == bytes && response.count == size,
            "directory Rread envelope");
    require(convM2D((uchar *)response.data, response.count, &decoded, strings) == size &&
            decoded.qid.path == input.qid.path && decoded.qid.vers == input.qid.vers &&
            decoded.mode == input.mode && decoded.dev == input.dev && decoded.type == input.type &&
            decoded.atime == input.atime && decoded.mtime == input.mtime && decoded.length == input.length &&
            !strcmp(decoded.name, input.name) && !strcmp(decoded.uid, "alice") &&
            !strcmp(decoded.gid, "group") && !strcmp(decoded.muid, "bob"), "directory attributes changed");
    require(convM2D(record, size - 1, &decoded, strings) == 0, "partial stat record accepted");
}


/* EN/RU: Use original linked C library entry points, never a replacement codec.
 * Использовать original C entry points, не replacement codec. */
int main(void)
{
    walk_vector();
    read_vector();
    directory_record();
    puts("PASS original Inferno C Styx codecs full-width fids qids stat UTF-8");
    return 0;
}
