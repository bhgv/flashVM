#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b, unsigned int n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}
static void name(Bytes &b, const char *s)
{
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}
static void text(Bytes &b, const char *s)
{
    b.push_back(0x96);
    u16(b, (unsigned int)std::strlen(s) + 2);
    b.push_back(0);
    name(b, s);
}
static void integer(Bytes &b, unsigned long n)
{
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    for (unsigned int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
}
static void reg(Bytes &b, unsigned int n)
{
    b.push_back(0x96);
    u16(b, 2);
    b.push_back(4);
    b.push_back((unsigned char)n);
}
static void variable(Bytes &b, const char *s)
{
    text(b, s);
    b.push_back(0x1c);
}
static FVM_Object *define(FVM_Context *c, unsigned int count, unsigned int flags,
                          unsigned int params, const char *const *names,
                          const unsigned int *registers, const Bytes &body)
{
    Bytes header, code;
    name(header, "testFunction");
    u16(header, params);
    header.push_back((unsigned char)count);
    u16(header, flags);
    for (unsigned int i = 0; i < params; ++i)
    {
        header.push_back((unsigned char)registers[i]);
        name(header, names[i]);
    }
    u16(header, (unsigned int)body.size());
    code.push_back(0x8e);
    u16(code, (unsigned int)header.size());
    for (size_t i = 0; i < header.size(); ++i)
        code.push_back(header[i]);
    for (size_t i = 0; i < body.size(); ++i)
        code.push_back(body[i]);
    code.push_back(0);
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    assert(fvm_avm1_inspect(code.begin(), code.size(), &info) == FVM_OK);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
    FVM_Value fn;
    assert(fvm_get_global(c, "testFunction", &fn) == FVM_OK && fn.type == FVM_VALUE_OBJECT);
    return fn.object;
}
static void end(Bytes &b)
{
    b.push_back(0x3e);
    b.push_back(0);
}
int main()
{
    const unsigned int invalid_flags[] = {512, 1024, 65535};
    for (size_t i = 0; i < sizeof(invalid_flags) / sizeof(invalid_flags[0]); ++i)
    {
        Bytes code;
        code.push_back(0x8e);
        u16(code, 8);
        name(code, "");
        u16(code, 0);
        code.push_back(6);
        u16(code, invalid_flags[i]);
        u16(code, 1);
        code.push_back(0);
        code.push_back(0);
        FVM_AVM1Info info;
        info.struct_size = sizeof(info);
        assert(fvm_avm1_inspect(code.begin(), code.size(), &info) == FVM_INVALID_ARGUMENT);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *fn = 0, *self = 0, *root = 0;
    FVM_Value out, arg, args[2], copy;
    FVM_Channel *channel = 0;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &self) == FVM_OK);
    assert(fvm_gc_pin(c, self) == FVM_OK);
    const char *parameters[] = {"x", "y"};
    unsigned int registers[] = {3, 0};
    {
        Bytes body;
        reg(body, 3);
        end(body);
        fn = define(c, 3, 1 | 4, 2, parameters, registers, body);
        fvm_value_number(args, 42);
        fvm_value_number(args + 1, 7);
        assert(fvm_function_invoke(c, fn, self, 2, args, &out) == FVM_OK && out.number == 42);
        assert(fvm_channel_create(&channel) == FVM_OK);
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = fn;
        assert(fvm_channel_send_value(c, channel, &arg, 0) == FVM_OK);
        assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
        assert(fvm_function_invoke(other, copy.object, 0, 2, args, &out) == FVM_OK &&
               out.number == 42);
    }
    {
        Bytes body;
        reg(body, 1);
        end(body);
        fn = define(c, 1, 1, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, self, 0, 0, &out) == FVM_OK && out.object == self);
    }
    {
        Bytes body;
        variable(body, "this");
        end(body);
        fn = define(c, 0, 2 | 8 | 32, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, self, 0, 0, &out) == FVM_OK &&
               out.type == FVM_VALUE_UNDEFINED);
    }
    {
        Bytes body;
        reg(body, 1);
        text(body, "marker");
        body.push_back(0x4e);
        end(body);
        fvm_value_number(&arg, 123);
        assert(fvm_set_global(c, "marker", &arg) == FVM_OK);
        fn = define(c, 1, 256, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, self, 0, 0, &out) == FVM_OK && out.number == 123);
    }
    {
        assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK);
        fvm_value_string(args, "child");
        fvm_value_number(args + 1, 1);
        assert(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK);
        FVM_Object *child = out.object;
        Bytes body;
        reg(body, 2);
        end(body);
        fn = define(c, 2, 64 | 128, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, child, 0, 0, &out) == FVM_OK && out.object == root);
    }
    {
        /* The highest register remains private and is shared with try blocks. */
        Bytes body;
        integer(body, 99);
        body.push_back(0x87);
        u16(body, 1);
        body.push_back(255);
        body.push_back(0x17);
        reg(body, 255);
        end(body);
        fn = define(c, 255, 0, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, self, 0, 0, &out) == FVM_OK && out.number == 99);
    }
    {
        /* A preload deliberately overwrites a parameter using the same register. */
        const char *p[] = {"x"};
        unsigned int r[] = {1};
        Bytes body;
        reg(body, 1);
        end(body);
        fn = define(c, 1, 1, 1, p, r, body);
        fvm_value_number(&arg, 9);
        assert(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK && out.object == self);
    }
    {
        /* Named parameters and arguments indices remain linked in both directions. */
        unsigned int r[] = {0};
        const char *p[] = {"x"};
        Bytes body;
        text(body, "x");
        integer(body, 77);
        body.push_back(0x1d);
        variable(body, "arguments");
        integer(body, 0);
        body.push_back(0x4e);
        end(body);
        fn = define(c, 0, 0, 1, p, r, body);
        fvm_value_number(&arg, 9);
        assert(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK && out.number == 77);
        Bytes setter;
        variable(setter, "arguments");
        integer(setter, 0);
        integer(setter, 88);
        setter.push_back(0x4f);
        variable(setter, "x");
        end(setter);
        fn = define(c, 0, 0, 1, p, r, setter);
        assert(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK && out.number == 88);
        Bytes escaped;
        variable(escaped, "arguments");
        end(escaped);
        fn = define(c, 0, 0, 1, p, r, escaped);
        assert(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK &&
               out.type == FVM_VALUE_OBJECT);
        FVM_Object *arguments = out.object;
        assert(fvm_gc_pin(c, arguments) == FVM_OK);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        fvm_value_number(&arg, 55);
        assert(fvm_object_set(c, arguments, "0", &arg) == FVM_OK);
        assert(fvm_object_get(c, arguments, "0", &out) == FVM_OK && out.number == 55);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = arguments;
        assert(fvm_channel_send_value(c, channel, &arg, 0) == FVM_OK);
        assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
        fvm_value_number(&arg, 66);
        assert(fvm_object_set(other, copy.object, "0", &arg) == FVM_OK);
        assert(fvm_object_get(other, copy.object, "0", &out) == FVM_OK && out.number == 66);
        assert(fvm_gc_unpin(c, arguments) == FVM_OK);
        unsigned long reclaimed = 0;
        assert(fvm_gc_collect(c, &reclaimed) == FVM_OK && reclaimed >= 3);
    }
    {
        /* Duplicate parameter names alias only the last named occurrence. */
        const char *p[] = {"x", "x"};
        unsigned int r[] = {0, 0};
        Bytes body;
        text(body, "x");
        integer(body, 44);
        body.push_back(0x1d);
        variable(body, "arguments");
        integer(body, 0);
        body.push_back(0x4e);
        end(body);
        fn = define(c, 0, 0, 2, p, r, body);
        fvm_value_number(args, 1);
        fvm_value_number(args + 1, 2);
        assert(fvm_function_invoke(c, fn, self, 2, args, &out) == FVM_OK && out.number == 1);
    }
    {
        Bytes body;
        reg(body, 1);
        end(body);
        fn = define(c, 1, 16, 0, 0, 0, body);
        assert(fvm_function_invoke(c, fn, self, 0, 0, &out) == FVM_OK &&
               out.type == FVM_VALUE_UNDEFINED);
    }
    {
        const unsigned int paired[] = {3, 12, 48, 63, 63, 63};
        const unsigned int selected[] = {1, 1, 1, 1, 2, 3};
        fvm_value_number(&arg, 17);
        for (unsigned int i = 0; i < 6; ++i)
        {
            Bytes body;
            reg(body, selected[i]);
            end(body);
            fn = define(c, paired[i] == 63 ? 3 : 1, paired[i], 0, 0, 0, body);
            assert(fvm_gc_pin(c, fn) == FVM_OK);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            bool is_arguments = paired[i] == 12 || selected[i] == 2;
            assert(fvm_function_invoke(c, fn, self, 1, &arg, &out) == FVM_OK);
            if (is_arguments)
            {
                assert(out.type == FVM_VALUE_OBJECT);
                assert(fvm_table_get_index(c, out.object, 0, &copy) == FVM_OK && copy.number == 17);
            }
            else
                assert(out.type == FVM_VALUE_UNDEFINED);
            fvm_value_undefined(&copy); copy.type = FVM_VALUE_OBJECT; copy.object = fn;
            assert(fvm_channel_send_value(c, channel, &copy, 0) == FVM_OK);
            assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
            assert(fvm_function_invoke(other, copy.object, 0, 1, &arg, &out) == FVM_OK);
            if (is_arguments)
            {
                assert(out.type == FVM_VALUE_OBJECT);
                assert(fvm_table_get_index(other, out.object, 0, &copy) == FVM_OK && copy.number == 17);
            }
            else
                assert(out.type == FVM_VALUE_UNDEFINED);
            assert(fvm_gc_unpin(c, fn) == FVM_OK);
        }
    }
    assert(fvm_gc_unpin(c, self) == FVM_OK);
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
