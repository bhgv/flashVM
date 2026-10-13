/* sortOn retains complete field names and slice bounds depend on the selected range. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/table.h"
#include <cassert>
static FVM_Value ref(FVM_Object *object)
{
    FVM_Value value;fvm_value_undefined(&value);value.type = FVM_VALUE_OBJECT;value.object = object;
    return value;
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *array, *record, *generic, *prototype;
    FVM_Value value, out, options[2], call[3], method;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
    for (unsigned long i = 0; i < 2; ++i)
    {
        assert(fvm_construct(c, "Object", 0, 0, &record) == FVM_OK);
        fvm_value_number(&value, (double)i*100);
        assert(fvm_object_set(c, record, "r", &value) == FVM_OK);
        fvm_value_number(&value, 2-(double)i);
        assert(fvm_object_set_bytes(c, record, "r\0x", 3, &value) == FVM_OK);
        value = ref(record);assert(fvm_table_set_index(c, array, i, &value) == FVM_OK);
    }
    assert(fvm_value_string_bytes(options, "r\0x", 3) == FVM_OK);
    fvm_value_number(options+1, 16);
    assert(fvm_object_call(c, array, "sortOn", 2, options, &out) == FVM_OK);
    assert(fvm_table_get_index(c, array, 0, &out) == FVM_OK);
    assert(fvm_object_get_bytes(c, out.object, "r\0x", 3, &value) == FVM_OK && value.number == 1);
    assert(fvm_construct(c, "Object", 0, 0, &generic) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &prototype) == FVM_OK);
    assert(fvm_object_set_prototype(c, generic, prototype) == FVM_OK);
    fvm_value_number(&value, 4294967295.0);
    assert(fvm_object_set(c, generic, "length", &value) == FVM_OK);
    fvm_value_number(&value, 42);
    assert(fvm_object_set(c, prototype, "4294967294", &value) == FVM_OK);
    assert(fvm_object_get(c, array, "slice", &method) == FVM_OK);
    call[0] = ref(generic);
    fvm_value_number(call+1, 4294967294.0);fvm_value_number(call+2, 4294967295.0);
    assert(fvm_object_call(c, method.object, "call", 3, call, &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 1);
    assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.number == 42);
    /* Negative endpoints use the full uint32 source length, retaining prototype slots. */
    fvm_value_number(call+1, -1);fvm_value_number(call+2, 4294967295.0);
    assert(fvm_object_call(c, method.object, "call", 3, call, &out) == FVM_OK);
    assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.number == 42);
    fvm_value_number(call+1, 10);fvm_value_number(call+2, 10);
    assert(fvm_object_call(c, method.object, "call", 3, call, &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 0);
    /* Large result intervals retain sparse holes without the former materialization cap. */
    fvm_value_number(call+1, 0);fvm_value_number(call+2, 65537);
    assert(fvm_object_call(c, method.object, "call", 3, call, &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 65537);
    assert(fvm_object_get(c, generic, "length", &value) == FVM_OK && value.number == 4294967295.0);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
