#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/exception.h"
#include "flashvm/stack.h"
#include "flashvm/avm1.h"
#include "flashvm/worker.h"
#include <assert.h>
#include <string.h>
static void equal(const FVM_Value *v, const unsigned short *expected, size_t n)
{
    unsigned short output[16];
    size_t length = 0;
    assert(fvm_value_get_utf16(v, 0, 0, &length) == FVM_OK && length == n);
    assert(fvm_value_get_utf16(v, output, 16, &length) == FVM_OK && length == n);
    assert(!memcmp(output, expected, n * sizeof(unsigned short)));
}
static FVM_Result echo(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *user)
{
    (void)c;
    (void)user;
    *out = *input;
    return FVM_OK;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *s = 0, *array = 0;
    FVM_Channel *channel = 0;
    FVM_Stack *stack = 0;
    FVM_Value input, out, arg, copy, object;
    unsigned short units[] = {65, 0, 66, 0xd800, 0xdc00, 0xdfff};
    unsigned short lone[] = {0xd800};
    unsigned short suffix[] = {0, 66, 0xd800, 0xdc00, 0xdfff};
    unsigned short guard[2] = {111, 222};
    size_t length;
    double number;
    const char borrowed[] = {'x', 0, 'y'};
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_value_string_utf16(c, units, 6, &input) == FVM_OK);
    equal(&input, units, 6);
    assert(fvm_value_get_utf16(&input, guard, 2, &length) == FVM_INVALID_ARGUMENT);
    assert(length == 6 && guard[0] == 111 && guard[1] == 222);
    assert(fvm_construct(c, "String", 1, &input, &s) == FVM_OK);
    assert(fvm_object_get(c, s, "length", &out) == FVM_OK && out.number == 6);
    fvm_value_number(&arg, 3);
    assert(fvm_object_call(c, s, "charAt", 1, &arg, &out) == FVM_OK);
    equal(&out, lone, 1);
    fvm_value_number(&arg, 1);
    assert(fvm_object_call(c, s, "substring", 1, &arg, &out) == FVM_OK);
    equal(&out, suffix, 5);
    fvm_value_string(&arg, "");
    assert(fvm_object_call(c, s, "split", 1, &arg, &out) == FVM_OK);
    array = out.object;
    assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == 6);
    assert(fvm_object_call(c, array, "join", 1, &arg, &out) == FVM_OK);
    equal(&out, units, 6);
    assert(fvm_value_to_number(c, &input, &number) == FVM_OK && number != number);
    assert(fvm_value_string_bytes(&arg, borrowed, sizeof(borrowed)) == FVM_OK);
    assert(fvm_object_set(c, s, "borrowed", &arg) == FVM_OK);
    assert(fvm_object_get(c, s, "borrowed", &copy) == FVM_OK);
    assert(fvm_value_string_size(&copy) == 3 && !memcmp(copy.string, borrowed, 3));
    assert(fvm_stack_create(c, &stack) == FVM_OK);
    assert(fvm_stack_push(stack, &input) == FVM_OK);
    assert(fvm_stack_get(stack, 1, &copy) == FVM_OK);
    equal(&copy, units, 6);
    assert(fvm_exception_raise(c, &input) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &copy) == FVM_OK);
    equal(&copy, units, 6);
    assert(fvm_exception_clear(c) == FVM_OK);
    fvm_value_undefined(&object);
    object.type = FVM_VALUE_OBJECT;
    object.object = s;
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &object, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_object_call(other, copy.object, "valueOf", 0, 0, &out) == FVM_OK);
    equal(&out, units, 6);
    assert(fvm_channel_send_value(c, channel, &input, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    equal(&copy, units, 6);
    assert(fvm_set_global(c, "text", &input) == FVM_OK);
    {
        const unsigned char code[] = {0x96, 7, 0,   0,   't', 'r', 'u', 't',  'h',  0,    0x96, 6,
                                      0,    0, 't', 'e', 'x', 't', 0,   0x1c, 0x12, 0x12, 0x1d, 0};
        assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
        assert(fvm_get_global(c, "truth", &out) == FVM_OK && out.boolean);
    }
    {
        unsigned short source[] = {0x41, 0x410, 0xdf, 0x130, 0, 0xdfff};
        unsigned short upper[] = {0x41, 0x410, 0x53, 0x53, 0x130, 0, 0xdfff};
        unsigned short lower[] = {0x61, 0x430, 0xdf, 0x69, 0x307, 0, 0xdfff};
        unsigned short greek[] = {0x39f, 0x3a3, 0x20, 0x39f, 0x3a3, 0x391};
        unsigned short greek_lower[] = {0x3bf, 0x3c2, 0x20, 0x3bf, 0x3c3, 0x3b1};
        assert(fvm_value_string_utf16(c, source, 6, &input) == FVM_OK);
        assert(fvm_construct(c, "String", 1, &input, &s) == FVM_OK);
        assert(fvm_object_call(c, s, "toUpperCase", 0, 0, &out) == FVM_OK);
        equal(&out, upper, 7);
        assert(fvm_object_call(c, s, "toLowerCase", 0, 0, &out) == FVM_OK);
        equal(&out, lower, 7);
        assert(fvm_value_string_utf16(c, greek, 6, &input) == FVM_OK);
        assert(fvm_construct(c, "String", 1, &input, &s) == FVM_OK);
        assert(fvm_object_call(c, s, "toLowerCase", 0, 0, &out) == FVM_OK);
        equal(&out, greek_lower, 6);
    }
    {
        FVM_Worker *worker = 0;
        FVM_Result result;
        assert(fvm_value_string_utf16(c, units, 6, &input) == FVM_OK);
        result = fvm_worker_run(&cfg, echo, &input, 0, &worker);
        if (result == FVM_OK)
        {
            assert(fvm_worker_join(worker, &out) == FVM_OK);
            equal(&out, units, 6);
            fvm_worker_destroy(worker);
        }
        else
            assert(result == FVM_UNSUPPORTED);
    }
    {
        const unsigned short hi[] = {0xd800}, lo[] = {0xdc00};
        const unsigned short pair[] = {0xd800, 0xdc00};
        const unsigned short zero[] = {0};
        FVM_Object *boolean = 0;
        const unsigned char code[] = {0x96, 8,   0,   0,   'j', 'o', 'i',  'n',  'e',  'd',  0,
                                      0x96, 5,   0,   0,   'h', 'i', 'g',  0,    0x1c, 0x96, 5,
                                      0,    0,   'l', 'o', 'w', 0,   0x1c, 0x47, 0x1d, 0x96, 6,
                                      0,    0,   's', 'a', 'm', 'e', 0,    0x96, 8,    0,    0,
                                      'j',  'o', 'i', 'n', 'e', 'd', 0,    0x1c, 0x96, 6,    0,
                                      0,    'p', 'a', 'i', 'r', 0,   0x1c, 0x66, 0x1d, 0};
        assert(fvm_value_string_utf16(c, hi, 1, &input) == FVM_OK);
        assert(fvm_set_global(c, "hig", &input) == FVM_OK);
        assert(fvm_value_string_utf16(c, lo, 1, &input) == FVM_OK);
        assert(fvm_set_global(c, "low", &input) == FVM_OK);
        assert(fvm_value_string_utf16(c, pair, 2, &input) == FVM_OK);
        assert(fvm_set_global(c, "pair", &input) == FVM_OK);
        assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
        assert(fvm_get_global(c, "joined", &out) == FVM_OK);
        equal(&out, pair, 2);
        assert(fvm_get_global(c, "same", &out) == FVM_OK && out.boolean);
        assert(fvm_value_string_utf16(c, zero, 1, &input) == FVM_OK);
        assert(fvm_construct(c, "Boolean", 1, &input, &boolean) == FVM_OK);
        assert(fvm_object_call(c, boolean, "valueOf", 0, 0, &out) == FVM_OK && out.boolean);
    }
    fvm_channel_destroy(channel);
    fvm_stack_destroy(stack);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
