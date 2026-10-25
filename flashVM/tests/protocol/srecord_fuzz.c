/* Deterministic bounded malformed-input/mutation stress; also a standalone sanitizer entrypoint. */
#include "flashvm/sbindings.h"
#include "flashvm/sstream.h"
#include <stdio.h>
#include <string.h>


/* Keep pseudo-random arithmetic explicitly 32-bit on both ILP32 and LP64. */
static unsigned long random_word(unsigned long *state)
{
    *state = (*state * 1664525UL + 1013904223UL) & FVM_SRECORD_U32;
    return *state;
}


/* Explore malformed prefixes and CRC-valid arbitrary metadata without performing VM operations. */
int main(void)
{
    unsigned char input[512], receive[512], wire[1024], payload[128];
    FVM_SStream stream;
    FVM_SRecord record;
    FVM_SBindings table;
    FVM_SBinding slots[4];
    FVM_SInteger number;
    FVM_SResult result;
    unsigned long seed = 725;
    size_t iteration, i, size, used, frame_size, position;
    for (iteration = 0; iteration < 50000; ++iteration)
    {
        size = (size_t)(random_word(&seed) % sizeof(input));
        for (i = 0; i < size; ++i)
        {
            input[i] = (unsigned char)(random_word(&seed) >> 19);
        }
        fvm_sinteger_read(input, size, &number, &used);
        fvm_srecord_read(input, size, 500, &record, &used);
        if (fvm_sbindings_init(&table, slots, 4) != FVM_S_OK ||
            fvm_sstream_init(&stream, receive, sizeof(receive), 500) != FVM_S_OK)
        {
            return 1;
        }
        position = 0;
        while (position < size)
        {
            result = fvm_sstream_push(&stream, input + position, 1, &record, &used);
            position += used;
            if (result != FVM_S_MORE)
            {
                break;
            }
            if (!used)
            {
                return 1;
            }
        }
        fvm_sbindings_apply(&table, input, size, &used);
        /* CRC-valid metadata bypasses the checksum barrier and exercises payload validation. */
        size = (size_t)(random_word(&seed) % sizeof(payload));
        for (i = 0; i < size; ++i)
        {
            payload[i] = (unsigned char)(random_word(&seed) >> 19);
        }
        if (fvm_srecord_write(0, payload, size, wire, sizeof(wire), &frame_size) != FVM_S_OK)
        {
            return 1;
        }
        fvm_sbindings_apply(&table, wire, frame_size, &used);
        if (frame_size)
        {
            wire[random_word(&seed) % frame_size] ^= (unsigned char)(1U << (random_word(&seed) % 8U));
            fvm_srecord_read(wire, frame_size, 500, &record, &used);
        }
    }
    puts("PASS 50000 bounded malformed/mutation cases and CRC-valid metadata bodies");
    return 0;
}
