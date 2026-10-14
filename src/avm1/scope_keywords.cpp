/* FlashVM: shared version/case recognition for implicit scope namespace keywords. */
#include "scope_keywords.hpp"
#include "../core/string_internal.hpp"
#include "../standard/movieclip_state.hpp"
#include "../../external/flash_case/case_table.hpp"
/* Match counted UCS2 keyword units using the pinned Flash lowercase map when needed. */
bool fvm_avm1_keyword_name(const fvm_detail::string &name, const char *expected, size_t units,
                           bool sensitive)
{
    if (sensitive)
        return name.size() == units && !std::memcmp(name.data(), expected, units);
    size_t offset = 0;
    for (size_t i = 0; i < units; ++i)
    {
        unsigned long unit = 0;
        if (fvm_string_read(name.data(), name.size(), offset, unit) != FVM_OK)
            return false;
        size_t first = 0, last = sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]);
        while (first < last)
        {
            size_t middle = first + (last - first) / 2;
            if (fvm_flash_lowercase[middle][FVM_SCOPE_KEYWORD_CASE_SOURCE_COLUMN] < unit)
                first = middle + 1;
            else
                last = middle;
        }
        if (first < sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]) &&
            fvm_flash_lowercase[first][FVM_SCOPE_KEYWORD_CASE_SOURCE_COLUMN] == unit)
            unit = fvm_flash_lowercase[first][FVM_SCOPE_KEYWORD_CASE_RESULT_COLUMN];
        if (unit != (unsigned char)expected[i])
            return false;
    }
    return offset == name.size();
}
/* Global namespace fallback starts in SWF6; unloaded host names retain exact spelling. */
bool fvm_avm1_global_keyword(FVM_Context *c, const fvm_detail::string &name)
{
    if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6)
        return false;
    return fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_GLOBAL_NAME,
                                 FVM_SCOPE_KEYWORD_GLOBAL_UNITS,
                                 !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_7);
}

/* Recognize virtual path membership without obtaining a value or traversing parent edges. */
unsigned int fvm_avm1_movieclip_path_kind(FVM_Context *c, FVM_Object *object,
                                          const fvm_detail::string &name)
{
    if (!c || !c->swf_loaded || c->swf_info.version < FVM_SWF_VERSION_5 || !object ||
        object->owner != c || object->resource_kind != FVM_RESOURCE_MOVIECLIP ||
        fvm_movieclip_is_class_prototype(object))
        return FVM_SCOPE_PATH_NONE;
    if (fvm_avm1_global_keyword(c, name))
        return FVM_SCOPE_PATH_GLOBAL;
    bool sensitive = c->swf_info.version >= FVM_SWF_VERSION_7;
    if (fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_ROOT_NAME, FVM_SCOPE_KEYWORD_ROOT_UNITS,
                              sensitive))
        return FVM_SCOPE_PATH_ROOT;
    if (fvm_avm1_keyword_name(name, FVM_SCOPE_KEYWORD_PARENT_NAME, FVM_SCOPE_KEYWORD_PARENT_UNITS,
                              sensitive))
        return FVM_SCOPE_PATH_PARENT;
    return FVM_SCOPE_PATH_NONE;
}

/* Resolve intrinsic display paths from private provenance, independently of replaceable fields. */
FVM_Result fvm_avm1_movieclip_path_property(FVM_Context *c, FVM_Object *object,
                                            const fvm_detail::string &name, FVM_Value *out,
                                            int *present)
{
    if (!c || !out || !present || (object && object->owner != c))
        return FVM_INVALID_ARGUMENT;
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
            return status;
        /* The low-level provenance helper retains its host null sentinel; property reads
         * expose undefined for a parentless loaded clip. */
        if (parent && value.type == FVM_VALUE_NULL)
            fvm_value_undefined(&value);
    }
    FVM_Result validated = fvm_validate_value(c, &value);
    if (validated != FVM_OK)
        return validated;
    *out = value;
    *present = 1;
    return FVM_OK;
}
