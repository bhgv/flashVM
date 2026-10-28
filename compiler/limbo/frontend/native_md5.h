/* RFC 1321 MD5 compression for bounded structural signatures, not authentication.
 * Implemented independently; native signature spelling/folding follows Inferno types.c. */
#ifndef LIMBO_NATIVE_MD5_H
#define LIMBO_NATIVE_MD5_H
#include <stdint.h>
#include <string.h>

/* Hash <=255 signature bytes and fold the four digest words exactly as Dis sign().
 * text/length are borrowed checked ASCII input; return the structural uint32 signature. */
static uint32_t native_signature(const unsigned char *text, size_t length)
{
    static const uint32_t constants[64] = {
        UINT32_C(0xd76aa478), UINT32_C(0xe8c7b756), UINT32_C(0x242070db), UINT32_C(0xc1bdceee), UINT32_C(0xf57c0faf),
        UINT32_C(0x4787c62a), UINT32_C(0xa8304613), UINT32_C(0xfd469501), UINT32_C(0x698098d8), UINT32_C(0x8b44f7af),
        UINT32_C(0xffff5bb1), UINT32_C(0x895cd7be), UINT32_C(0x6b901122), UINT32_C(0xfd987193), UINT32_C(0xa679438e),
        UINT32_C(0x49b40821), UINT32_C(0xf61e2562), UINT32_C(0xc040b340), UINT32_C(0x265e5a51), UINT32_C(0xe9b6c7aa),
        UINT32_C(0xd62f105d), UINT32_C(0x02441453), UINT32_C(0xd8a1e681), UINT32_C(0xe7d3fbc8), UINT32_C(0x21e1cde6),
        UINT32_C(0xc33707d6), UINT32_C(0xf4d50d87), UINT32_C(0x455a14ed), UINT32_C(0xa9e3e905), UINT32_C(0xfcefa3f8),
        UINT32_C(0x676f02d9), UINT32_C(0x8d2a4c8a), UINT32_C(0xfffa3942), UINT32_C(0x8771f681), UINT32_C(0x6d9d6122),
        UINT32_C(0xfde5380c), UINT32_C(0xa4beea44), UINT32_C(0x4bdecfa9), UINT32_C(0xf6bb4b60), UINT32_C(0xbebfbc70),
        UINT32_C(0x289b7ec6), UINT32_C(0xeaa127fa), UINT32_C(0xd4ef3085), UINT32_C(0x04881d05), UINT32_C(0xd9d4d039),
        UINT32_C(0xe6db99e5), UINT32_C(0x1fa27cf8), UINT32_C(0xc4ac5665), UINT32_C(0xf4292244), UINT32_C(0x432aff97),
        UINT32_C(0xab9423a7), UINT32_C(0xfc93a039), UINT32_C(0x655b59c3), UINT32_C(0x8f0ccc92), UINT32_C(0xffeff47d),
        UINT32_C(0x85845dd1), UINT32_C(0x6fa87e4f), UINT32_C(0xfe2ce6e0), UINT32_C(0xa3014314), UINT32_C(0x4e0811a1),
        UINT32_C(0xf7537e82), UINT32_C(0xbd3af235), UINT32_C(0x2ad7d2bb), UINT32_C(0xeb86d391),
    };
    static const unsigned shifts[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                        5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                                        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
    unsigned char padded[320]; /* Bounded profile signatures fit at most five MD5 blocks. */
    uint32_t state[4] = {UINT32_C(0x67452301), UINT32_C(0xefcdab89), UINT32_C(0x98badcfe), UINT32_C(0x10325476)};
    size_t size = (length + 9 + 63) / 64 * 64;
    size_t block;
    unsigned index;
    memset(padded, 0, sizeof(padded));
    memcpy(padded, text, length);
    padded[length] = 128;
    for (index = 0; index < 8; index++)
    {
        padded[size - 8 + index] = (unsigned char)(((uint64_t)length * 8) >> (index * 8));
    }
    for (block = 0; block < size; block += 64)
    {
        uint32_t words[16];
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        for (index = 0; index < 16; index++)
        {
            const unsigned char *p = padded + block + index * 4;
            words[index] = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
        }
        for (index = 0; index < 64; index++)
        {
            uint32_t f;
            uint32_t temporary;
            unsigned word;
            if (index < 16)
            {
                f = (b & c) | (~b & d);
                word = index;
            }
            else if (index < 32)
            {
                f = (d & b) | (~d & c);
                word = (5 * index + 1) % 16;
            }
            else if (index < 48)
            {
                f = b ^ c ^ d;
                word = (3 * index + 5) % 16;
            }
            else
            {
                f = c ^ (b | ~d);
                word = (7 * index) % 16;
            }
            temporary = a + f + constants[index] + words[word];
            a = d;
            d = c;
            c = b;
            b += (temporary << shifts[index]) | (temporary >> (32 - shifts[index]));
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
    }
    return state[0] ^ state[1] ^ state[2] ^ state[3];
}
#endif
