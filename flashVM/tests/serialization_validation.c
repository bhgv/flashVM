#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include "zlib.h"
#include <assert.h>
#include <string.h>
#include <math.h>

static void u32(unsigned char *p, unsigned long n)
{
    p[0] = (unsigned char)(n >> 24);
    p[1] = (unsigned char)(n >> 16);
    p[2] = (unsigned char)(n >> 8);
    p[3] = (unsigned char)n;
}
static void repair(unsigned char *data, size_t size)
{
    u32(data + 12, (unsigned long)crc32(0, data + 16, (uInt)(size - 16)));
}
static FVM_Result noop(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *u)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)u;
    fvm_value_undefined(out);
    return FVM_OK;
}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *c, *dst;
    FVM_Object *root, *child;
    FVM_Value value, copy;
    unsigned char *data;
    size_t size, i;
    unsigned long before, after;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_context_create(runtime, &dst) == FVM_OK);
    assert(fvm_object_create(c, &root) == FVM_OK);
    assert(fvm_object_create(c, &child) == FVM_OK);
    assert(fvm_object_set_prototype(c, root, child) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = root;
    assert(fvm_value_serialize(c, &value, &data, &size) == FVM_OK);
    assert(fvm_gc_object_count(dst, &before) == FVM_OK);
    for (i = 0; i < size; ++i)
    {
        assert(fvm_value_deserialize(dst, data, i, &copy) != FVM_OK);
        assert(copy.type == FVM_VALUE_UNDEFINED);
    }
    /* First record: kind, state(undefined+meta), prototype, meta, five counts.
       A valid first prototype must not lead to an unchecked second reference. */
    assert(size == 187);
    u32(data + 29 + 79 + 9, 100000);
    repair(data, size);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_INVALID_ARGUMENT);
    u32(data + 29 + 79 + 9, 0xffffffffUL);
    /* First object -> itself is an invalid prototype cycle. */
    u32(data + 29 + 9, 0);
    repair(data, size);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_INVALID_ARGUMENT);
    u32(data + 29 + 9, 1);
    repair(data, size);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK);
    /* A version-1 object record lacks attributes/function/cache trailers.
     * Strip the two empty 38-byte trailers to form a genuine legacy graph. */
    {
        unsigned char legacy[103];
        memcpy(legacy, data, 29);
        memcpy(legacy + 29, data + 29, 17);
        memcpy(legacy + 46, data + 50, 20);
        memcpy(legacy + 66, data + 108, 17);
        memcpy(legacy + 83, data + 129, 20);
        legacy[7] = 1;
        u32(legacy + 8, sizeof(legacy) - 16);
        repair(legacy, sizeof(legacy));
        assert(fvm_value_deserialize(dst, legacy, sizeof(legacy), &copy) == FVM_OK);
        assert(copy.type == FVM_VALUE_OBJECT);
    }
    {
        unsigned char previous[171];
        memcpy(previous, data, 29);
        memcpy(previous + 29, data + 29, 17);
        memcpy(previous + 46, data + 50, 54);
        memcpy(previous + 100, data + 108, 17);
        memcpy(previous + 117, data + 129, 54);
        previous[7] = 2;
        u32(previous + 8, sizeof(previous) - 16);
        repair(previous, sizeof(previous));
        assert(fvm_value_deserialize(dst, previous, sizeof(previous), &copy) == FVM_OK);
    }
    {
        unsigned char previous[179];
        memcpy(previous, data, 29);
        memcpy(previous + 29, data + 29, 17);
        memcpy(previous + 46, data + 50, 58);
        memcpy(previous + 104, data + 108, 17);
        memcpy(previous + 121, data + 129, 58);
        u32(previous + 8, sizeof(previous) - 16);
        repair(previous, sizeof(previous));
        for (i = 3; i <= 5; ++i)
        {
            previous[7] = (unsigned char)i;
            assert(fvm_value_deserialize(dst, previous, sizeof(previous), &copy) == FVM_OK);
        }
    }
    data[7] = 6;
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK);
    data[7] = 7;
    fvm_serialized_free(data);
    /* Decode rollback does not keep partly allocated objects or invoke user code. */
    assert(fvm_serialization_register_callback(c, "test.noop", noop) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, root, "noop", noop, 0, 0) == FVM_OK);
    assert(fvm_value_serialize(c, &value, &data, &size) == FVM_OK);
    assert(fvm_gc_object_count(dst, &before) == FVM_OK);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_UNSUPPORTED);
    assert(fvm_gc_object_count(dst, &after) == FVM_OK && before == after);
    assert(fvm_serialization_register_callback(dst, "test.noop", noop) == FVM_OK);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK);
    assert(fvm_object_call(dst, copy.object, "noop", 0, 0, &copy) == FVM_OK);
    fvm_serialized_free(data);
    /* Signed zero and IEEE non-finite values keep their exact wire bits. */
    fvm_value_number(&value, -0.0);
    assert(fvm_value_serialize(c, &value, &data, &size) == FVM_OK);
    assert(data[16] == FVM_VALUE_NUMBER && data[17] == 0x80);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK);
    assert(!memcmp(&value.number, &copy.number, sizeof(double)));
    data[17] = 0x7f;
    data[18] = 0xf8;
    repair(data, size);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK && copy.number != copy.number);
    fvm_serialized_free(data);
    fvm_value_number(&value, HUGE_VAL);
    assert(fvm_value_serialize(c, &value, &data, &size) == FVM_OK);
    assert(fvm_value_deserialize(dst, data, size, &copy) == FVM_OK && copy.number == value.number);
    fvm_serialized_free(data);
    fvm_value_undefined(&value);
    value.type = (FVM_ValueType)99;
    assert(fvm_value_serialize(c, &value, &data, &size) == FVM_INVALID_ARGUMENT && !data && !size);
    fvm_context_destroy(dst);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
