#ifndef FVM_STANDARD_STRING_ORDER_HPP
#define FVM_STANDARD_STRING_ORDER_HPP
#include "flashvm/utf8.h"
#include "flashvm/flashvm.h"
#include <cstring>
/* Compare scalar UTF-8 storage by AVM1 UTF-16 code-unit order, without allocating
 * temporary strings or using host locale. Supplementary characters contribute
 * two units; U+10000 consequently sorts before U+E000. */
inline FVM_Result fvm_string_order(const char *left, const char *right, int &order)
{
    const char *text[2] = {left, right};
    size_t length[2] = {std::strlen(left), std::strlen(right)}, offset[2] = {0, 0};
    unsigned long pending[2] = {0, 0};
    for (;;)
    {
        unsigned long unit[2] = {0, 0};
        bool end[2];
        for (unsigned int i = 0; i < 2; ++i)
        {
            end[i] = !pending[i] && offset[i] == length[i];
            if (end[i])
                continue;
            if (pending[i])
            {
                unit[i] = pending[i];
                pending[i] = 0;
            }
            else
            {
                if (fvm_utf8_read(text[i], length[i], offset + i, unit + i) != FVM_UTF8_OK)
                    return FVM_INVALID_ARGUMENT;
                if (unit[i] >= 0x10000)
                {
                    unit[i] -= 0x10000;
                    pending[i] = 0xdc00 + (unit[i] & 1023);
                    unit[i] = 0xd800 + (unit[i] >> 10);
                }
            }
        }
        if (end[0] || end[1] || unit[0] != unit[1])
        {
            order = end[0] ? (end[1] ? 0 : -1) : end[1] ? 1 : unit[0] < unit[1] ? -1 : 1;
            return FVM_OK;
        }
    }
}
#endif
