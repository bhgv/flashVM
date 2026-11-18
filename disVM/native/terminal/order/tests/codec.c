/* Copyright (c) 2026 bhgv
 * EN: Atomic prefix/packet refusal and incremental original-S-record compatibility.
 * RU: Атомарность отказов префикса/пакета и совместимость с исходными S-записями.
 */
#include "check.h"


/* EN/RU: Test every byte width, truncation, data-bit corruption and caller-output preservation.
 * Проверить все длины, усечения, повреждения битов и сохранность результата при отказе.
 */
static void prefixes(void)
{
    for (unsigned width = 1; width <= 8; width++)
    {
        unsigned long long number = width == 8 ? ~0ULL : (1ULL << (width * 8)) - 1; /* Width endpoint. */
        unsigned char bytes[9]; /* Exact encoded prefix. */
        FVM_SOrdinal value = {(unsigned long)(number >> 32), (unsigned long)(number & 0xffffffffULL)}; /* Source. */
        size_t used = 444; /* Failure sentinel. */
        CHECK(fvm_sordinal_write(&value, bytes, width, &used) == FVM_S_LIMIT && used == 444);
        CHECK(fvm_sordinal_write(&value, bytes, sizeof(bytes), &used) == FVM_S_OK && used == width + 1);
        for (size_t length = 0; length < used; length++)
        {
            FVM_SOrdinal result = {123, 456}; /* Must remain unchanged for MORE. */
            size_t read = 555; /* Exact-read sentinel. */
            CHECK(fvm_sordinal_read(bytes, length, &result, &read) == FVM_S_MORE);
            CHECK(result.high == 123 && result.low == 456 && read == 555);
        }
        for (unsigned bit = 3; bit < used * 8; bit++)
        {
            unsigned char corrupt[9]; /* One flipped CRC/data bit, length unchanged. */
            FVM_SOrdinal result = {123, 456}; /* Failure sentinel. */
            size_t read = 555; /* Failure extent sentinel. */
            memcpy(corrupt, bytes, used);
            corrupt[bit / 8] ^= (unsigned char)(1U << (bit % 8));
            CHECK(fvm_sordinal_read(corrupt, used, &result, &read) == FVM_S_CHECKSUM);
            CHECK(result.high == 123 && result.low == 456 && read == 555);
        }
    }
}


/* EN/RU: Fragment every split point, retain one view, then explicitly reset for coalesced tail.
 * Разбить пакет в каждой позиции, сохранить view, затем явно сбросить перед следующим пакетом.
 */
static void fragments(void)
{
    unsigned char bytes[512]; /* Two complete packets back-to-back. */
    size_t size = packet(7, ~0ULL, "payload", bytes); /* First packet. */
    size_t tail = packet(9, 0, "next", bytes + size); /* Second packet. */
    for (size_t split = 0; split < size; split++)
    {
        unsigned char storage[256]; /* Caller-owned original S-stream storage. */
        FVM_SOrderedStream stream; /* Private incremental descriptor. */
        FVM_SOrderedRecord view; /* Borrowed only until reset/storage mutation. */
        size_t used = 999; /* Actual consumed input count. */
        CHECK(fvm_sordered_stream_init(&stream, storage, sizeof(storage), 128) == FVM_S_OK);
        CHECK(fvm_sordered_stream_push(&stream, bytes, split, &view, &used) == FVM_S_MORE && used == split);
        CHECK(fvm_sordered_stream_push(&stream, bytes + split, size + tail - split, &view, &used) == FVM_S_OK);
        CHECK(used == size - split && view.ordinal.high == 0xffffffffUL && view.ordinal.low == 0xffffffffUL);
        CHECK(view.record.tag == 7 && view.record.size == 7 && !memcmp(view.record.payload, "payload", 7));
        CHECK(view.encoded_size == size);
        CHECK(fvm_sordered_stream_push(&stream, bytes + size, tail, &view, &used) == FVM_S_INVALID && !used);
        CHECK(fvm_sordered_stream_reset(&stream) == FVM_S_OK);
        for (size_t at = 0; at < tail; at++)
        {
            FVM_SResult expected = at + 1 == tail ? FVM_S_OK : FVM_S_MORE; /* Final-byte completion. */
            CHECK(fvm_sordered_stream_push(&stream, bytes + size + at, 1, &view, &used) == expected && used == 1);
        }
        CHECK(!view.ordinal.high && !view.ordinal.low && view.record.tag == 9);
    }
}


/* EN/RU: Verify body CRC still applies and malformed prefix latches until known-boundary reset.
 * Проверить исходный CRC тела и сохранение ошибки префикса до явного reset на известной границе.
 */
static void errors(void)
{
    unsigned char bytes[256]; /* Full packet and one malformed prefix. */
    unsigned char storage[256]; /* Original decoder storage. */
    FVM_SOrderedStream stream; /* Native decoder without transport ownership. */
    FVM_SOrderedRecord view; /* Output untouched on errors. */
    size_t size = packet(7, 0, "body", bytes); /* Valid packet before corruption. */
    size_t used = 555; /* Atomic full-read sentinel. */
    const unsigned char redundant[] = {0x79, 0, 0}; /* Independent polynomial oracle: valid CRC, forbidden leading zero. */
    FVM_SOrdinal ordinal = {123, 456}; /* Invalid canonicality preserves both limbs. */
    CHECK(fvm_sordinal_read(redundant, sizeof(redundant), &ordinal, &used) == FVM_S_INVALID);
    CHECK(ordinal.high == 123 && ordinal.low == 456 && used == 555);
    bytes[size - 1] ^= 1;
    CHECK(fvm_sordered_read(bytes, size, 128, &view, &used) == FVM_S_CHECKSUM && used == 555);
    bytes[0] ^= 8;
    CHECK(fvm_sordered_stream_init(&stream, storage, sizeof(storage), 128) == FVM_S_OK);
    CHECK(fvm_sordered_stream_push(&stream, bytes, size, &view, &used) == FVM_S_CHECKSUM && used == 2);
    CHECK(fvm_sordered_stream_push(&stream, NULL, 0, &view, &used) == FVM_S_CHECKSUM && !used);
    CHECK(fvm_sordered_stream_reset(&stream) == FVM_S_OK);
    size = packet(7, 0, "body", bytes);
    CHECK(fvm_sordered_stream_push(&stream, bytes, size, &view, &used) == FVM_S_OK && used == size);
}


/* EN/RU: Run codec and stream contracts; queue helpers also prove ordinary engine compatibility.
 * Выполнить проверки кодека/потока; helpers дополнительно проверяют совместимость с очередью.
 */
int main(void)
{
    StyxOrder *order = styxordercreate(128, 8); /* Owned minimal queue for shared helpers. */
    CHECK(order);
    prefixes();
    fragments();
    errors();
    CHECK(offer(order, 7, 0, "codec") == STYX_ORDER_ACCEPTED);
    take(order, 7, 0, "codec");
    styxorderfree(order);
    return 0;
}
