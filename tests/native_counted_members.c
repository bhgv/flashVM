/* Counted native methods/accessors preserve captures and dispatch across graphs. */
#include "flashvm/value.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/avm1.h"
#include <assert.h>
#include <string.h>
static const char method[] = {'m', 0, 'n'};
static const char accessor[] = {'a', 0, 'b'};
static FVM_Result compute(FVM_Context *c, FVM_Object *self, unsigned int n,
                          const FVM_Value *args, FVM_Value *out, void *raw)
{
    FVM_Value base, capture;
    (void)n;(void)args;
    assert(fvm_object_get(c, self, "base", &base) == FVM_OK);
    assert(fvm_object_get(c, (FVM_Object *)raw, "marker", &capture) == FVM_OK);
    fvm_value_number(out, base.number + capture.number);
    return FVM_OK;
}
static FVM_Result write(FVM_Context *c, FVM_Object *self, unsigned int n,
                        const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)self;
    assert(n == 1);
    fvm_value_undefined(out);
    return fvm_object_set(c, (FVM_Object *)raw, "marker", args);
}
static void check(FVM_Context *c, FVM_Object *object, double expected)
{
    FVM_Value out, first, second;
    assert(fvm_object_call_bytes(c, object, method, sizeof(method), 0, 0, &out) == FVM_OK);
    assert(out.number == expected);
    assert(fvm_object_get_bytes(c, object, accessor, sizeof(accessor), &out) == FVM_OK);
    assert(out.number == expected);
    assert(fvm_object_get_bytes(c, object, method, sizeof(method), &first) == FVM_OK);
    assert(fvm_object_get_bytes(c, object, method, sizeof(method), &second) == FVM_OK);
    assert(first.type == FVM_VALUE_OBJECT && first.object == second.object);
    assert(fvm_function_invoke(c, first.object, object, 0, 0, &out) == FVM_OK && out.number == expected);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c, *other;
    FVM_Object *object, *prototype, *capture;
    FVM_Value value, out, graph, key;
    FVM_Channel *queue;
    unsigned char *bytes;
    size_t size, i;
    int deleted;
    const char scalar[] = "\xf0\x90\x80\x80";
    const char pair[] = "\xed\xa0\x80\xed\xb0\x80";
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_object_create(c, &prototype) == FVM_OK);
    assert(fvm_object_create(c, &object) == FVM_OK);
    assert(fvm_object_create(c, &capture) == FVM_OK);
    assert(fvm_object_set_prototype(c, object, prototype) == FVM_OK);
    assert(fvm_gc_pin(c, object) == FVM_OK);
    fvm_value_number(&value, 10);assert(fvm_object_set(c, object, "base", &value) == FVM_OK);
    fvm_value_number(&value, 7);assert(fvm_object_set(c, capture, "marker", &value) == FVM_OK);
    assert(fvm_object_define_method_rooted_bytes(c, prototype, method, sizeof(method), compute,
                                                capture, capture) == FVM_OK);
    assert(fvm_object_define_accessor_rooted_bytes(c, prototype, accessor, sizeof(accessor),
                                                  compute, write, capture, capture) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, prototype, pair, compute, capture, capture) == FVM_OK);
    assert(fvm_object_call(c, object, scalar, 0, 0, &out) == FVM_OK && out.number == 17);
    assert(fvm_object_define_accessor_rooted(c, prototype, "\xed\xbf\xbf", compute, 0,
                                             capture, capture) == FVM_OK);
    assert(fvm_object_get(c, object, "\xed\xbf\xbf", &out) == FVM_OK && out.number == 17);
    assert(fvm_object_define_method_rooted_bytes(c, object, "\xff", 1, compute, capture,
                                                capture) == FVM_INVALID_ARGUMENT);
    assert(fvm_gc_collect(c, 0) == FVM_OK);check(c, object, 17);
    fvm_value_number(&value, 99);
    assert(fvm_object_set_bytes(c, object, method, sizeof(method), &value) == FVM_OK);
    assert(fvm_object_call_bytes(c, object, method, sizeof(method), 0, 0, &out) == FVM_UNSUPPORTED);
    assert(fvm_object_delete_bytes(c, object, method, sizeof(method), &deleted) == FVM_OK && deleted);
    check(c, object, 17);
    fvm_value_number(&value, 12);
    assert(fvm_object_set_bytes(c, object, accessor, sizeof(accessor), &value) == FVM_OK);
    check(c, object, 22);
    fvm_value_undefined(&graph);graph.type = FVM_VALUE_OBJECT;graph.object = object;
    assert(fvm_set_global(c, "obj", &graph) == FVM_OK);
    assert(fvm_value_string_bytes(&key, method, sizeof(method)) == FVM_OK);
    assert(fvm_set_global(c, "key", &key) == FVM_OK);
    {
        const unsigned char code[] = {0x96,8,0,0,'r','e','s','u','l','t',0,
            0x96,5,0,7,0,0,0,0,0x96,5,0,0,'o','b','j',0,0x1c,
            0x96,5,0,0,'k','e','y',0,0x1c,0x52,0x1d,0};
        assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
        assert(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 22);
    }
    assert(fvm_serialization_register_callback(c, "counted.compute", compute) == FVM_OK);
    assert(fvm_serialization_register_callback(c, "counted.write", write) == FVM_OK);
    assert(fvm_serialization_register_callback(other, "counted.compute", compute) == FVM_OK);
    assert(fvm_serialization_register_callback(other, "counted.write", write) == FVM_OK);
    assert(fvm_value_serialize(c, &graph, &bytes, &size) == FVM_OK);
    assert(fvm_value_deserialize(other, bytes, size, &out) == FVM_OK);
    check(other, out.object, 22);fvm_serialized_free(bytes);
    assert(fvm_channel_create(&queue) == FVM_OK);
    assert(fvm_channel_send_value(c, queue, &graph, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, queue, &out, 0) == FVM_OK);check(other, out.object, 22);
    /* The size-aware table view exposes an interior NUL without an enumeration copy. */
    fvm_value_number(&value, 42);assert(fvm_object_set_bytes(c, object, "q\0r", 3, &value) == FVM_OK);
    assert(fvm_table_named_count(c, object, &size) == FVM_OK);
    for (i = 0; i < size; ++i)
    {
        const char *name;
        size_t length;
        assert(fvm_table_named_at_bytes(c, object, i, &name, &length, &out) == FVM_OK);
        if (length == 3 && !memcmp(name, "q\0r", 3))break;
    }
    assert(i < size);
    fvm_channel_release(queue);assert(fvm_gc_unpin(c, object) == FVM_OK);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
