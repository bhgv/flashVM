/* Shared input encoding check, independent of either generated parser. */
#include <stddef.h>
/* UTF-8 validation rejects overlong sequences, surrogates and values above
 * U+10FFFF. Comments may contain Unicode too; NUL is never an EOF shortcut. */
int fir_utf8(const unsigned char *text, size_t n)
{
    size_t i;
    int remaining = 0;
    unsigned low = 0x80, high = 0xbf;
    for (i = 0; i < n; ++i) {
        unsigned c = text[i];
        if (remaining) {
            if (c < low || c > high) return 0;
            --remaining; low = 0x80; high = 0xbf;
        } else if (!c) return 0;
        else if (c < 0x80) continue;
        else if (c >= 0xc2 && c <= 0xdf) remaining = 1;
        else if (c >= 0xe0 && c <= 0xef) {
            remaining = 2;
            low = c == 0xe0 ? 0xa0 : 0x80; high = c == 0xed ? 0x9f : 0xbf;
        } else if (c >= 0xf0 && c <= 0xf4) {
            remaining = 3;
            low = c == 0xf0 ? 0x90 : 0x80; high = c == 0xf4 ? 0x8f : 0xbf;
        } else return 0;
    }
    return !remaining;
}
