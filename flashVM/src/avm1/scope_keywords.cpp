/* FlashVM: shared Flash case comparison and versioned implicit scope keywords. */
#include "scope_keywords.hpp"
#include "../core/string_internal.hpp"
#include "../standard/movieclip_state.hpp"
#include "../../external/flash_case/case_table.hpp"

namespace
{


/* Fold one UTF-16 unit through the same pinned table as loaded String casing. */
unsigned long lower_unit(unsigned long unit)
{
    size_t first = 0, last = sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]);
    while (first < last)
    {
        size_t middle = first + (last - first) / 2;
        if (fvm_flash_lowercase[middle][FVM_SCOPE_CASE_SOURCE_COLUMN] < unit)
        {
            first = middle + 1;
        }
        else
        {
            last = middle;
        }
    }
    if (first < sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]) &&
        fvm_flash_lowercase[first][FVM_SCOPE_CASE_SOURCE_COLUMN] == unit)
    {
        return fvm_flash_lowercase[first][FVM_SCOPE_CASE_RESULT_COLUMN];
    }
    return unit;
}
}


/* Compare counted names by individually folded UTF-16 units without allocating or invoking script. */
bool fvm_avm1_folded_name_equal(const fvm_detail::string &left, const fvm_detail::string &right)
{
    const char *text[FVM_SCOPE_NAME_OPERAND_COUNT] = {left.data(), right.data()};
    size_t length[FVM_SCOPE_NAME_OPERAND_COUNT] = {left.size(), right.size()};
    size_t offset[FVM_SCOPE_NAME_OPERAND_COUNT] = {0, 0};
    unsigned long pending[FVM_SCOPE_NAME_OPERAND_COUNT] = {0, 0};
    for (;;)
    {
        unsigned long unit[FVM_SCOPE_NAME_OPERAND_COUNT] = {0, 0};
        bool end[FVM_SCOPE_NAME_OPERAND_COUNT];
        for (unsigned int i = 0; i < FVM_SCOPE_NAME_OPERAND_COUNT; ++i)
        {
            end[i] = !pending[i] && offset[i] == length[i];
            if (end[i])
            {
                continue;
            }
            if (pending[i])
            {
                unit[i] = pending[i];
                pending[i] = 0;
            }
            else
            {
                if (fvm_string_read(text[i], length[i], offset[i], unit[i]) != FVM_OK)
                {
                    return false;
                }
                if (unit[i] >= FVM_UTF16_SUPPLEMENTARY_OFFSET)
                {
                    unit[i] -= FVM_UTF16_SUPPLEMENTARY_OFFSET;
                    pending[i] = FVM_UTF16_LOW_SURROGATE_BASE + (unit[i] & FVM_UTF16_PAIR_PAYLOAD_MASK);
                    unit[i] = FVM_UTF16_HIGH_SURROGATE_BASE + (unit[i] >> FVM_UTF16_PAIR_PAYLOAD_BITS);
                }
            }
        }
        if (end[0] || end[1])
        {
            return end[0] && end[1];
        }
        if (lower_unit(unit[0]) != lower_unit(unit[1]))
        {
            return false;
        }
    }
}


/* Match counted UCS2 keyword units using the pinned Flash lowercase map when needed. */
bool fvm_avm1_keyword_name(const fvm_detail::string &name, const char *expected, size_t units, bool sensitive)
{
    if (sensitive)
    {
        return name.size() == units && !std::memcmp(name.data(), expected, units);
    }
    size_t offset = 0;
    for (size_t i = 0; i < units; ++i)
    {
        unsigned long unit = 0;
        if (fvm_string_read(name.data(), name.size(), offset, unit) != FVM_OK)
        {
            return false;
        }
        unit = lower_unit(unit);
        if (unit != (unsigned char)expected[i])
        {
            return false;
        }
    }
    return offset == name.size();
}


/* Global namespace fallback starts in SWF6; unloaded host names retain exact spelling. */
bool fvm_avm1_global_keyword(FVM_Context *c, const fvm_detail::string &name)
{
    if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6)
    {
        return false;
    }
    return fvm_avm1_keyword_name(name,
                                 FVM_SCOPE_KEYWORD_GLOBAL_NAME,
                                 FVM_SCOPE_KEYWORD_GLOBAL_UNITS,
                                 !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_7);
}


/* Recognize virtual path membership without obtaining a value or traversing parent edges. */
unsigned int fvm_avm1_movieclip_path_kind(FVM_Context *c, FVM_Object *object, const fvm_detail::string &name)
{
    if (!c || !c->swf_loaded || c->swf_info.version < FVM_SWF_VERSION_5 || !object || object->owner != c ||
        object->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(object))
    {
        return FVM_SCOPE_PATH_NONE;
    }
    if (fvm_avm1_global_keyword(c, name))
    {
        return FVM_SCOPE_PATH_GLOBAL;
    }
    bool sensitive = c->swf_info.version >= FVM_SWF_VERSION_7;
    if (fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_ROOT_NAME, FVM_SCOPE_KEYWORD_ROOT_UNITS, sensitive))
    {
        return FVM_SCOPE_PATH_ROOT;
    }
    if (fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_PARENT_NAME, FVM_SCOPE_KEYWORD_PARENT_UNITS, sensitive))
    {
        return FVM_SCOPE_PATH_PARENT;
    }
    return FVM_SCOPE_PATH_NONE;
}


/* Resolve intrinsic display paths from private provenance, independently of replaceable fields. */
FVM_Result fvm_avm1_movieclip_path_property(
    FVM_Context *c, FVM_Object *object, const fvm_detail::string &name, FVM_Value *out, int *present)
{
    if (!c || !out || !present || (object && object->owner != c))
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned int kind = fvm_avm1_movieclip_path_kind(c, object, name);
    if (kind == FVM_SCOPE_PATH_NONE)
    {
        *present = 0;
        return FVM_OK;
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    if (kind == FVM_SCOPE_PATH_GLOBAL)
    {
        value.type = FVM_VALUE_OBJECT;
        value.object = c->global;
    }
    else
    {
        bool parent = kind == FVM_SCOPE_PATH_PARENT;
        FVM_Result status = fvm_movieclip_preload_display(c, object, parent, &value);
        if (status != FVM_OK)
        {
            return status;
        }
        /* The low-level provenance helper retains its host null sentinel; property reads
         * expose undefined for a parentless loaded clip. */
        if (parent && value.type == FVM_VALUE_NULL)
        {
            fvm_value_undefined(&value);
        }
    }
    FVM_Result validated = fvm_validate_value(c, &value);
    if (validated != FVM_OK)
    {
        return validated;
    }
    *out = value;
    *present = 1;
    return FVM_OK;
}
