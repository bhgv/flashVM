/* FlashVM: common parameter/name decoding and function-body framing normalization. */
#include "function_definition.hpp"
#include "block.hpp"
#include "action_codes.h"
#include "../core/property_key.hpp"
/* Caller validates the record bounds; allocation failures unwind through its entry guard. */
FVM_Result fvm_avm1_prepare_function(const unsigned char *data, const FVM_FunctionRecord &record,
                                     bool loaded,
                                     const fvm_detail::vector<fvm_detail::string> *constants,
                                     FVM_ScriptFunction &fn)
{
    fn.function2 = record.function2;
    fn.register_count = record.register_count;
    fn.flags = record.flags;
    if (constants)
        fn.constants = *constants;
    FVM_Result status =
        fvm_property_key((const char *)data + record.name, record.name_size, fn.name);
    if (status != FVM_OK)
        return status;
    size_t position = record.parameters;
    for (unsigned int parameter = 0; parameter < record.count; ++parameter)
    {
        if (record.function2)
            fn.parameter_registers.push_back(data[position++]);
        size_t begin = position;
        while (data[position])
            ++position;
        fvm_detail::string name;
        status = fvm_property_key((const char *)data + begin, position - begin, name);
        if (status != FVM_OK)
            return status;
        fn.parameters.push_back(name);
        ++position;
    }
    fn.body.assign(data + record.body, data + record.body + record.size);
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    if ((loaded ? fvm_avm1_inspect_loaded : fvm_avm1_inspect)(fn.body.begin(), fn.body.size(),
                                                            &info) != FVM_OK)
    {
        fn.body.push_back(FVM_AVM1_ACTION_END);
        if ((loaded ? fvm_avm1_inspect_loaded : fvm_avm1_inspect)(fn.body.begin(), fn.body.size(),
                                                                &info) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}
