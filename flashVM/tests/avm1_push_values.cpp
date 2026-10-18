#include "flashvm/avm1.h"
#include "flashvm/extensions.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
#include <cmath>
#include <limits>

namespace
{
typedef fvm_detail::vector<unsigned char> Bytes;
void text(Bytes &bytes, const char *s)
{
    do
    {
        bytes.push_back((unsigned char)*s);
    } while (*s++);
}
void action(Bytes &bytes, unsigned char opcode, const Bytes &payload)
{
    bytes.push_back(opcode);
    bytes.push_back((unsigned char)payload.size());
    bytes.push_back((unsigned char)(payload.size() >> 8));
    for (size_t i = 0; i < payload.size(); ++i)
        bytes.push_back(payload[i]);
}
void push_text(Bytes &bytes, const char *s)
{
    Bytes payload;
    payload.push_back(0);
    text(payload, s);
    action(bytes, 0x96, payload);
}
void push_integer(Bytes &bytes, unsigned int n)
{
    Bytes payload;
    payload.push_back(7);
    for (unsigned int i = 0; i < 4; ++i)
        payload.push_back((unsigned char)(n >> (i * 8)));
    action(bytes, 0x96, payload);
}
} // namespace
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Value out;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_extensions_install(context) == FVM_OK);

    Bytes bytes, payload;
    payload.push_back(2);
    payload.push_back(0);
    text(payload, "float");
    text(payload, "double");
    action(bytes, 0x88, payload);
    payload.clear();
    payload.push_back(8);
    payload.push_back(0); /* Constant8 key. */
    payload.push_back(1); /* IEEE float 1.5, little endian. */
    payload.push_back(0);
    payload.push_back(0);
    payload.push_back(0xc0);
    payload.push_back(0x3f);
    action(bytes, 0x96, payload);
    bytes.push_back(0x1d);
    payload.clear();
    payload.push_back(9);
    payload.push_back(1);
    payload.push_back(0);
    payload.push_back(6); /* IEEE double -2.5: high word before low word. */
    payload.push_back(0);
    payload.push_back(0);
    payload.push_back(4);
    payload.push_back(0xc0);
    for (unsigned int i = 0; i < 4; ++i)
        payload.push_back(0);
    action(bytes, 0x96, payload);
    bytes.push_back(0x1d);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
    assert(fvm_get_global(context, "float", &out) == FVM_OK && out.number == 1.5);
    assert(fvm_get_global(context, "double", &out) == FVM_OK && out.number == -2.5);

    /* Exercise IEEE special values, word order and denormal reconstruction. */
    struct DoubleCase
    {
        unsigned long high, low;
        double expected;
        int nan;
    };
    const DoubleCase cases[] = {{0x7ff00000UL, 0, std::numeric_limits<double>::infinity(), 0},
                                {0xfff00000UL, 0, -std::numeric_limits<double>::infinity(), 0},
                                {0x7ff80000UL, 1, 0, 1},
                                {0, 1, std::numeric_limits<double>::denorm_min(), 0},
                                {0x80000000UL, 0, -0.0, 0}};
    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); ++c)
    {
        bytes.clear();
        payload.clear();
        push_text(bytes, "edge");
        payload.push_back(6);
        for (unsigned int i = 0; i < 4; ++i)
            payload.push_back((unsigned char)(cases[c].high >> (i * 8)));
        for (unsigned int i = 0; i < 4; ++i)
            payload.push_back((unsigned char)(cases[c].low >> (i * 8)));
        action(bytes, 0x96, payload);
        bytes.push_back(0x1d);
        bytes.push_back(0);
        assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
        assert(fvm_get_global(context, "edge", &out) == FVM_OK);
        if (cases[c].nan)
            assert(out.number != out.number);
        else
            assert(out.number == cases[c].expected);
        if (c == 4)
            assert(1.0 / out.number == -std::numeric_limits<double>::infinity());
    }
    /* NaN is false for both Not and conditional branches. */
    bytes.clear();
    push_text(bytes, "nanFalse");
    push_text(bytes, "edge");
    bytes.push_back(0x1c); /* Replace edge with NaN first. */
    FVM_Value nanValue;
    fvm_value_number(&nanValue, std::numeric_limits<double>::quiet_NaN());
    assert(fvm_set_global(context, "edge", &nanValue) == FVM_OK);
    bytes.push_back(0x12);
    bytes.push_back(0x1d);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
    assert(fvm_get_global(context, "nanFalse", &out) == FVM_OK && out.boolean);
    bytes.clear();
    push_text(bytes, "edge");
    bytes.push_back(0x1c);
    payload.clear();
    payload.push_back(1);
    payload.push_back(0);
    action(bytes, 0x9d, payload);
    bytes.push_back(0x17);
    bytes.push_back(0);
    /* Taking the branch would skip the underflowing Pop; NaN must not take it. */
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_ERROR);

    /* The function retains its defining pool after the original buffer dies. */
    bytes.clear();
    payload.clear();
    payload.push_back(1);
    payload.push_back(0);
    text(payload, "captured");
    action(bytes, 0x88, payload);
    Bytes body;
    payload.clear();
    payload.push_back(8);
    payload.push_back(0);
    action(body, 0x96, payload);
    body.push_back(0x3e);
    body.push_back(0);
    payload.clear();
    text(payload, "poolFunction");
    payload.push_back(0);
    payload.push_back(0); /* No parameters. */
    payload.push_back((unsigned char)body.size());
    payload.push_back(0);
    for (size_t i = 0; i < body.size(); ++i)
        payload.push_back(body[i]);
    action(bytes, 0x9b, payload);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
    bytes.clear();
    assert(fvm_avm1_call(context, "poolFunction", 0, 0, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && std::strcmp(out.string, "captured") == 0);

    /* A bytecode literal stored globally survives buffer reuse. */
    push_text(bytes, "text");
    push_text(bytes, "persistent");
    bytes.push_back(0x1d);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
    std::memset(&bytes[0], 0xff, bytes.size());
    assert(fvm_get_global(context, "text", &out) == FVM_OK);
    assert(std::strcmp(out.string, "persistent") == 0);

    /* Store an object in register 1, clear operands, collect, and recover it. */
    bytes.clear();
    push_integer(bytes, 0);
    push_text(bytes, "Object");
    bytes.push_back(0x40);
    payload.clear();
    payload.push_back(1);
    action(bytes, 0x87, payload);
    bytes.push_back(0x17);
    push_integer(bytes, 0);
    push_text(bytes, "gc");
    bytes.push_back(0x1c);
    push_text(bytes, "collect");
    bytes.push_back(0x52);
    bytes.push_back(0x17);
    push_text(bytes, "registered");
    payload.clear();
    payload.push_back(4);
    payload.push_back(1);
    action(bytes, 0x96, payload);
    bytes.push_back(0x1d);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_OK);
    assert(fvm_get_global(context, "registered", &out) == FVM_OK);
    assert(out.type == FVM_VALUE_OBJECT);
    FVM_Value value;
    fvm_value_number(&value, 7);
    assert(fvm_object_set(context, out.object, "alive", &value) == FVM_OK);

    bytes.clear();
    payload.clear();
    payload.push_back(8);
    payload.push_back(255);
    action(bytes, 0x96, payload);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_INVALID_ARGUMENT);
    bytes.clear();
    payload.clear();
    payload.push_back(4);
    payload.push_back(4);
    action(bytes, 0x96, payload);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_INVALID_ARGUMENT);
    bytes.clear();
    payload.clear();
    payload.push_back(6);
    payload.push_back(0);
    action(bytes, 0x96, payload);
    bytes.push_back(0);
    assert(fvm_avm1_execute(context, &bytes[0], bytes.size()) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
