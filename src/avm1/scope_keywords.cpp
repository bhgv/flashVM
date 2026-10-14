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
        return name == fvm_detail::string(expected);
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

/* A concrete loaded MovieClip supplies its namespace after own slots and before prototypes. */
bool fvm_avm1_movieclip_global(FVM_Context *c, FVM_Object *object, const fvm_detail::string &name)
{
    return c->swf_loaded && object && object->resource_kind == FVM_RESOURCE_MOVIECLIP &&
           !fvm_movieclip_is_class_prototype(object) && fvm_avm1_global_keyword(c, name);
}
