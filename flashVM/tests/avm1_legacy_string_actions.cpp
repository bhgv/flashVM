/* Counted UTF-16 legacy string actions, integer wrapping and collection during coercion. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"

static unsigned calls, order[2];


static FVM_Result ordered(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    order[calls++] = id;
    return fvm_value_string(out, id == 1 ? "a" : "b");
}


static FVM_Value unary(FVM_Context *c, unsigned opcode, const FVM_Value &value)
{
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "input");
    b.push_back((unsigned char)opcode);
    return run(c, b);
}


static FVM_Value binary(FVM_Context *c, unsigned opcode, const FVM_Value &left, const FVM_Value &right)
{
    if (!(fvm_set_global(c, "left", &left) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "right", &right) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "left");
    global(b, "right");
    b.push_back((unsigned char)opcode);
    return run(c, b);
}


static FVM_Value extract(FVM_Context *c, unsigned opcode, const FVM_Value &value, double start, double count)
{
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value number;
    fvm_value_number(&number, start);
    if (!(fvm_set_global(c, "start", &number) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&number, count);
    if (!(fvm_set_global(c, "count", &number) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "input");
    global(b, "start");
    global(b, "count");
    b.push_back((unsigned char)opcode);
    return run(c, b);
}


static void script_method(FVM_Context *c, FVM_Object *owner, Bytes body)
{
    body.push_back(0x3e);
    body.push_back(0);
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = fn;
    if (!(fvm_object_set(c, owner, "toString", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 0x08, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value a, b, result;
    fvm_value_string_bytes(&a, "a\0b", 3);
    fvm_value_string_bytes(&b, "c\0d", 3);
    result = binary(c, 0x21, a, b);
    if (!(fvm_value_string_size(&result) == 6 && !std::memcmp(result.string, "a\0bc\0d", 6)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(unary(c, 0x14, a).number == 3 && unary(c, 0x31, a).number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    result = extract(c, 0x15, a, 2, 1);
    if (!(fvm_value_string_size(&result) == 1 && result.string[0] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_bytes(&a, "\xf0\x9f\x98\x80", 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char pair[] = {char(0xed), char(0xa0), char(0xbd), char(0xed), char(0xb8), char(0x80)};
    if (!(fvm_value_string_bytes(&b, pair, sizeof(pair)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(binary(c, 0x13, a, b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(unary(c, 0x14, a).number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    result = extract(c, 0x35, a, 1, 1);
    unsigned short unit = 0;
    size_t length = 0;
    if (!(fvm_value_get_utf16(&result, &unit, 1, &length) == FVM_OK && length == 1 && unit == 0xd83d))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&a, "abcd");
    if (!(!std::strcmp(extract(c, 0x15, a, 2, -1).string, "bcd")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(extract(c, 0x15, a, -2, 2).string, "ab")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    result = extract(c, 0x15, a, 10, 2);
    if (!(fvm_value_string_size(&result) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(extract(c, 0x15, a, 4294967298.0, 1).string, "b")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&a, "10");
    fvm_value_string(&b, "2");
    if (!(binary(c, 0x29, a, b).boolean && !binary(c, 0x68, a, b).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&a, 4294967295.0);
    if (!(unary(c, 0x18, a).number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&a, -1.9);
    if (!(unary(c, 0x18, a).number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&a, std::numeric_limits<double>::infinity());
    if (!(unary(c, 0x18, a).number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *ordered_left = 0, *ordered_right = 0;
    if (!(fvm_construct(c, "Object", 0, 0, &ordered_left) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &ordered_right) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned ids[] = {1, 2};
    if (!(fvm_object_define_method_rooted(c, ordered_left, "toString", ordered, ids, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, ordered_right, "toString", ordered, ids + 1, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&a);
    a.type = FVM_VALUE_OBJECT;
    a.object = ordered_left;
    b = a;
    b.object = ordered_right;
    const unsigned comparisons[] = {0x13, 0x21, 0x29, 0x68};
    for (unsigned i = 0; i < 4; ++i)
    {
        calls = 0;
        binary(c, comparisons[i], a, b);
        if (!(calls == 2 && order[0] == (comparisons[i] == 0x68 ? 1U : 2U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* The second script conversion collects temporary strings from the first. */
    FVM_Object *left = 0, *right = 0;
    if (!(fvm_construct(c, "Object", 0, 0, &left) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &right) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body;
    text(body, "collected");
    integer(body, 0);
    global(body, "gc");
    text(body, "collect");
    body.push_back(0x52);
    body.push_back(0x1d);
    text(body, "LEFT");
    script_method(c, left, body);
    body.clear();
    text(body, "prefix");
    text(body, "suffix");
    body.push_back(0x21);
    script_method(c, right, body);
    fvm_value_undefined(&a);
    a.type = FVM_VALUE_OBJECT;
    a.object = left;
    b = a;
    b.object = right;
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 20; ++i)
    {
        result = binary(c, 0x21, a, b);
        if (!(!std::strcmp(result.string, "LEFTprefixsuffix")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value collected;
        if (!(fvm_get_global(c, "collected", &collected) == FVM_OK && collected.number >= 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!binary(c, 0x13, a, b).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(binary(c, 0x29, a, b).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(binary(c, 0x68, b, a).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const unsigned ops[] = {0x13, 0x14, 0x15, 0x18, 0x21, 0x29, 0x31, 0x35, 0x68};
    for (unsigned i = 0; i < sizeof(ops) / sizeof(*ops); ++i)
    {
        unsigned char empty[] = {(unsigned char)ops[i], 0};
        if (!(fvm_avm1_execute(c, empty, sizeof(empty)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Context *legacy = 0;
    if (!(fvm_context_create(rt, &legacy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char older[sizeof(swf)];
    std::memcpy(older, swf, sizeof(swf));
    older[3] = 5;
    if (!(fvm_load_memory(legacy, older, sizeof(older)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(legacy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&a, "abcd");
    if (!(unary(legacy, 0x14, a).number == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&a, "\xc3\xa9");
    if (!(fvm_set_global(legacy, "input", &a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes unsupported;
    global(unsupported, "input");
    unsupported.push_back(0x14);
    unsupported.push_back(0);
    if (!(fvm_avm1_execute(legacy, unsupported.begin(), unsupported.size()) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(legacy);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
