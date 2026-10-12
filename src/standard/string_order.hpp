/* FlashVM: Built-in ActionScript objects: string order.
 * File: src/standard/string_order.hpp.
 */

#ifndef FVM_STANDARD_STRING_ORDER_HPP
/* Define FVM STANDARD STRING ORDER HPP constant or compilation switch. */
#define FVM_STANDARD_STRING_ORDER_HPP
#include "../core/string_internal.hpp"
#include "flashvm/flashvm.h"
#include <cstring>
/* Compare UTF-8/WTF-8 storage by AVM1 UTF-16 code-unit order, without allocating
 * temporary strings or using host locale. Supplementary characters contribute
 * two units; U+10000 consequently sorts before U+E000. */
/* Compute and return string order. */
inline FVM_Result fvm_string_order(const FVM_Value &left, const FVM_Value &right, int &order)
{

    /* Initialization / Инициализация локального состояния. */
    const char *text[2] = {left.string, right.string};
    size_t length[2] = {fvm_string_size(left), fvm_string_size(right)}, offset[2] = {0, 0};
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
                if (fvm_string_read(text[i], length[i], offset[i], unit[i]) != FVM_OK)
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

/* Compute and return string order. */
inline FVM_Result fvm_string_order(const char *left, size_t left_size, const char *right,
                                   size_t right_size, int &order)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value a, b;
    FVM_Result r = fvm_value_string_bytes(&a, left, left_size);
    if (r == FVM_OK)
        r = fvm_value_string_bytes(&b, right, right_size);
    return r == FVM_OK ? fvm_string_order(a, b, order) : r;
}

/* Compute and return string order. */
inline FVM_Result fvm_string_order(const char *left, const char *right, int &order)
{
    return fvm_string_order(left, std::strlen(left), right, std::strlen(right), order);
}
#endif
