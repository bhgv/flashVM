/* UTF-16-equivalent duplicate formals share one lexical binding and only the last alias. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/debug.h"
#include "flashvm/serialization.h"
static void finish(Bytes &body)
{
    body.push_back(0x3e);body.push_back(0);
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c, *other;
    FVM_Object *fn;
    FVM_Value args[2], out, packet;
    const char pair[] = "\xed\xa0\x80\xed\xb0\x80";
    const char scalar[] = "\xf0\x90\x80\x80";
    const char *parameters[] = {pair, scalar};
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(args, 42);fvm_value_number(args+1, 7);
    Bytes body;
    global(body, "arguments");integer(body, 0);integer(body, 99);body.push_back(0x4f);
    global(body, scalar);finish(body);
    assert(fvm_function_create_avm1(c, body.begin(), body.size(), 2, parameters, 0, &fn) == FVM_OK);
    assert(fvm_function_invoke(c, fn, 0, 2, args, &out) == FVM_OK);
    assert(out.number == 7);
    {
        Bytes body;
        text(body, scalar);integer(body, 123);body.push_back(0x1d);
        global(body, "arguments");finish(body);
        assert(fvm_function_create_avm1(c, body.begin(), body.size(), 2, parameters, 0, &fn) == FVM_OK);
        assert(fvm_function_invoke(c, fn, 0, 2, args, &out) == FVM_OK);
        FVM_Object *arguments = out.object;
        assert(fvm_gc_pin(c, arguments) == FVM_OK);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_table_get_index(c, arguments, 0, &out) == FVM_OK && out.number == 42);
        assert(fvm_table_get_index(c, arguments, 1, &out) == FVM_OK && out.number == 123);
        assert(fvm_gc_unpin(c, arguments) == FVM_OK);
    }
    {
        FVM_Value parameter;
        assert(fvm_value_string_bytes(&parameter, "p\0x", 3) == FVM_OK);
        assert(fvm_set_global(c, "parameterKey", &parameter) == FVM_OK);
        Bytes body;
        global(body, "arguments");integer(body, 0);integer(body, 99);body.push_back(0x4f);
        global(body, "parameterKey");body.push_back(0x1c);finish(body);
        assert(fvm_function_create_avm1_bytes(c, body.begin(), body.size(), 1, &parameter, 0, &fn) == FVM_OK);
        assert(fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_function_invoke(c, fn, 0, 1, args, &out) == FVM_OK && out.number == 99);
        fvm_value_undefined(&packet);packet.type = FVM_VALUE_OBJECT;packet.object = fn;
        assert(fvm_set_global_bytes(c, "f\0n", 3, &packet) == FVM_OK);
        assert(fvm_avm1_call_bytes(c, "f\0n", 3, 0, 1, args, &out) == FVM_OK && out.number == 99);
        assert(fvm_avm1_call(c, "f", 0, 1, args, &out) == FVM_NOT_FOUND);
        fvm_value_undefined(&packet);packet.type = FVM_VALUE_OBJECT;packet.object = fn;
        unsigned char *bytes;
        size_t size;
        assert(fvm_value_serialize(c, &packet, &bytes, &size) == FVM_OK);
        assert(fvm_value_deserialize(other, bytes, size, &out) == FVM_OK);
        FVM_Object *copy = out.object;
        assert(fvm_gc_pin(other, copy) == FVM_OK);
        assert(fvm_set_global(other, "parameterKey", &parameter) == FVM_OK);
        assert(fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_function_invoke(other, copy, 0, 1, args, &out) == FVM_OK && out.number == 99);
        fvm_serialized_free(bytes);
        assert(fvm_gc_unpin(other, copy) == FVM_OK);
        assert(fvm_gc_unpin(c, fn) == FVM_OK);
        fvm_value_number(&parameter, 1);
        assert(fvm_function_create_avm1_bytes(c, body.begin(), body.size(), 1, &parameter, 0, &fn) == FVM_INVALID_ARGUMENT && !fn);
        assert(fvm_function_create_avm1(c, body.begin(), body.size(), 65536, parameters, 0, &fn) == FVM_INVALID_ARGUMENT && !fn);
    }
    {
        Bytes function_body, record, code;
        integer(function_body, 11);finish(function_body);
        for (size_t i = 0; i < sizeof(pair); ++i)record.push_back((unsigned char)pair[i]);
        record.push_back(0);record.push_back(0);
        record.push_back((unsigned char)function_body.size());record.push_back(0);
        for (size_t i = 0; i < function_body.size(); ++i)record.push_back(function_body[i]);
        code.push_back(0x9b);code.push_back((unsigned char)record.size());code.push_back(0);
        for (size_t i = 0; i < record.size(); ++i)code.push_back(record[i]);
        code.push_back(0);
        assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
        assert(fvm_avm1_call(c, scalar, 0, 0, 0, &out) == FVM_OK && out.number == 11);
        size_t count;
        assert(fvm_debug_script_function_count(c, &count) == FVM_OK);
        bool found = false;
        for (size_t i = 0; i < count; ++i)
        {
            const char *name;
            size_t size;
            assert(fvm_debug_script_function_at_bytes(c, i, &name, &size) == FVM_OK);
            if (size == sizeof(scalar)-1 && !std::memcmp(name, scalar, size))found = true;
        }
        assert(found);
    }
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
