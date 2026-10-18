#include "flashvm/avm1.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
static int getters;
static FVM_Result get(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *u)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)u;
    ++getters;
    fvm_value_number(out, 1);
    return FVM_OK;
}
static void key(FVM_Context *c, FVM_Object *keys, unsigned long index, const char *expected)
{
    FVM_Value v;
    assert(fvm_table_get_index(c, keys, index, &v) == FVM_OK && !strcmp(v.string, expected));
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *o = 0, *proto = 0, *keys = 0;
    FVM_Channel *channel = 0;
    FVM_Value value, out, copy;
    int deleted;
    const unsigned char loop[] = {
        0x96, 8,    0,    0,   'o',  'b',  'j',  'e',  'c',  't',  0,    0x1c, 0x55, 0x96, 7,   0,
        0,    'f',  'i',  'r', 's',  't',  0,    0x4d, 0x1d, 0x96, 8,    0,    0,    's',  'e', 'c',
        'o',  'n',  'd',  0,   0x4d, 0x1d, 0x96, 7,    0,    0,    't',  'h',  'i',  'r',  'd', 0,
        0x4d, 0x1d, 0x96, 5,   0,    0,    'e',  'n',  'd',  0,    0x4d, 0x1d, 0};
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
    assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
    fvm_value_number(&value, 1);
    assert(fvm_object_set(c, o, "a", &value) == FVM_OK);
    assert(fvm_object_set(c, o, "shadow", &value) == FVM_OK);
    assert(fvm_object_set_property_flags(c, o, "shadow", FVM_PROPERTY_DONT_ENUM, 0) == FVM_OK);
    assert(fvm_object_set(c, proto, "shadow", &value) == FVM_OK);
    assert(fvm_object_set(c, proto, "z", &value) == FVM_OK);
    assert(fvm_table_set_index(c, o, 2, &value) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, o, "computed", get, 0, 0, 0) == FVM_OK);
    assert(fvm_object_enumerate(c, o, 1, &keys) == FVM_OK);
    key(c, keys, 0, "2");
    key(c, keys, 1, "a");
    key(c, keys, 2, "computed");
    key(c, keys, 3, "z");
    assert(fvm_object_get(c, keys, "length", &out) == FVM_OK && out.number == 4);
    assert(getters == 0);
    assert(fvm_object_enumerate(c, o, 0, &keys) == FVM_OK);
    assert(fvm_object_get(c, keys, "length", &out) == FVM_OK && out.number == 3);
    assert(fvm_object_delete(c, o, "computed", &deleted) == FVM_OK && deleted);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    assert(fvm_set_global(c, "object", &value) == FVM_OK);
    assert(fvm_avm1_execute(c, loop, sizeof(loop)) == FVM_OK);
    assert(fvm_get_global(c, "first", &out) == FVM_OK && !strcmp(out.string, "2"));
    assert(fvm_get_global(c, "second", &out) == FVM_OK && !strcmp(out.string, "a"));
    assert(fvm_get_global(c, "third", &out) == FVM_OK && !strcmp(out.string, "z"));
    assert(fvm_get_global(c, "end", &out) == FVM_OK && out.type == FVM_VALUE_NULL);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_object_enumerate(other, copy.object, 1, &keys) == FVM_OK);
    assert(fvm_object_get(other, keys, "length", &out) == FVM_OK && out.number == 3);
    key(other, keys, 0, "2");
    key(other, keys, 1, "a");
    key(other, keys, 2, "z");
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
