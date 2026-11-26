/* Copyright (c) 2026 bhgv
 * EN: Recognize the distinct numeric index namespace on native plain tables.
 * RU: Распознавать отдельное пространство числовых индексов нативных таблиц.
 */
#ifndef FVM_AVM1_MEMBER_KEY_HPP
#define FVM_AVM1_MEMBER_KEY_HPP
#include "../core/runtime.hpp"
#include "../core/numeric_constants.h"
#include <cmath>


/* EN: True when a validated owner and key use native table indexing instead of a string key.
 * RU: Проверить, что проверенные владелец и ключ требуют числового индекса нативной таблицы. */
inline bool fvm_avm1_raw_table_key(FVM_Context *context, const FVM_Value &owner, const FVM_Value &key)
{
    return owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind == FVM_RESOURCE_PLAIN_OBJECT &&
        owner.object != context->global && key.type == FVM_VALUE_NUMBER && key.number >= 0 &&
        key.number <= FVM_UINT32_MAX_NUMBER && key.number == std::floor(key.number);
}
#endif
