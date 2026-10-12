#include "../core/runtime.hpp"
#include "../core/native_call_guard.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include <cstdio>
#include <new>
namespace
{
void add(FVM_Object *object, const fvm_detail::string &name,
         fvm_detail::map<fvm_detail::string, bool> &seen,
         fvm_detail::vector<fvm_detail::string> &keys)
{
    if (seen.find(name) != seen.end())
        return;
    /* Nonenumerable own properties still hide inherited names. */
    seen[name] = true;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
        object->attributes.find(name);
    if (flag == object->attributes.end() || !(flag->second & FVM_PROPERTY_DONT_ENUM))
        keys.push_back(name);
    if (seen.size() > 65536)
        throw std::bad_alloc();
}
} // namespace
extern "C" FVM_Result fvm_object_enumerate(FVM_Context *c, FVM_Object *object,
                                           int include_prototypes, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !object || object->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_NativeCallGuard snapshot(c);
        fvm_detail::map<fvm_detail::string, bool> seen;
        fvm_detail::vector<fvm_detail::string> keys;
        unsigned int depth = 0;
        FVM_Object *cursor = object;
        while (cursor && depth++ < 1024)
        {
            for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = cursor->indexed.begin();
                 i != cursor->indexed.end(); ++i)
            {
                char name[32];
                std::sprintf(name, "%lu", i->first);
                add(cursor, fvm_detail::string(name), seen, keys);
            }
            for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i =
                     cursor->properties.begin();
                 i != cursor->properties.end(); ++i)
                add(cursor, i->first, seen, keys);
            for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                     cursor->methods.begin();
                 i != cursor->methods.end(); ++i)
                add(cursor, i->first, seen, keys);
            for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                     cursor->accessors.begin();
                 i != cursor->accessors.end(); ++i)
                add(cursor, i->first, seen, keys);
            cursor = include_prototypes ? cursor->prototype : 0;
        }
        if (cursor)
            return FVM_ERROR;
        FVM_Object *result = 0;
        FVM_Result r = fvm_array_create_like(c, 0, (unsigned long)keys.size(), &result);
        if (r != FVM_OK)
            return r;
        for (size_t i = 0; i < keys.size(); ++i)
        {
            FVM_Value value;
            fvm_value_string(&value, keys[i].c_str());
            r = fvm_table_set_index(c, result, (unsigned long)i, &value);
            if (r != FVM_OK)
                return r;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
