/* Copyright (c) 2026 bhgv
 * EN: OS-free original 9P codec acceptance with high64 offsets/qids, counted strings and
 * truncation.
 * RU: Проверка исходного 9P codec без OS с high64 offset/qid, counted strings и усечением.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#include "styx/codec.h"
#include <string.h>


/* EN: Exercise the original codec with caller-owned stack buffers: serialize a high64 Tread, reject
 * every truncated extent, then round-trip a high64 Dir. Fcall strings may borrow mutable wire
 * bytes; decoded Dir strings borrow strings[]. Return 1..4 for the corresponding failed check,
 * otherwise 0. No allocation, file, network or thread is used.
 * RU: Проверить исходный codec на stack buffers: Tread с high64 offset, все усечённые размеры,
 * затем Dir с high64 Qid. Fcall strings могут ссылаться на mutable wire bytes, decoded Dir strings
 * — на strings[]. Вернуть 1..4 по номеру отказа, иначе 0. Без allocation, файлов, сети и потоков.
 */
int main(void)
{
    Fcall input;
    Fcall output;
    unsigned char bytes[512];
    memset(&input, 0, sizeof(input));
    input.type = Tread;
    input.tag = 17;
    input.fid = 0xfedcba98U;
    input.offset = UINT64_C(0xfedcba9876543210);
    input.count = 127;
    uint size = convS2M(&input, bytes, sizeof(bytes));
    if (size != 23 || bytes[4] != Tread || GBIT64(bytes + 11) != input.offset)
    {
        return 1;
    }
    for (uint truncated = 0; truncated < size; truncated++)
    {
        if (convM2S(bytes, truncated, &output))
        {
            return 2;
        }
    }
    if (convM2S(bytes, size, &output) != size || output.offset != input.offset || output.fid != input.fid)
    {
        return 3;
    }
    Dir row;
    Dir decoded;
    char strings[512];
    memset(&row, 0, sizeof(row));
    row.qid.path = UINT64_C(0xf123456789abcdef);
    row.mode = 0444;
    row.name = "temperature";
    row.uid = "device";
    row.gid = "device";
    row.muid = "";
    size = convD2M(&row, bytes, sizeof(bytes));
    if (!size || statcheck(bytes, size) || convM2D(bytes, size, &decoded, strings) != size ||
        decoded.qid.path != row.qid.path || strcmp(decoded.name, row.name))
    {
        return 4;
    }
    return 0;
}
