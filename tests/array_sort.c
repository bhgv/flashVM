#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
static void item(FVM_Context *c, FVM_Object *a, unsigned long index, double expected)
{
    FVM_Value v;
    assert(fvm_table_get_index(c, a, index, &v) == FVM_OK && v.number == expected);
}
static void object(FVM_Value *v, FVM_Object *o)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = o;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *a = 0, *records = 0, *record = 0;
    FVM_Channel *ch = 0;
    FVM_Value values[3], out, flags, copy, options[2];
    unsigned int i;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(values, 100);
    fvm_value_number(values + 1, 2);
    fvm_value_number(values + 2, 20);
    assert(fvm_construct(c, "Array", 3, values, &a) == FVM_OK);
    fvm_value_number(&flags, 24); /* Numeric index preview does not modify the source. */
    assert(fvm_object_call(c, a, "sort", 1, &flags, &out) == FVM_OK);
    item(c, a, 0, 100);
    item(c, out.object, 0, 1);
    item(c, out.object, 1, 2);
    item(c, out.object, 2, 0);
    fvm_value_number(&flags, 16);
    assert(fvm_object_call(c, a, "sort", 1, &flags, &out) == FVM_OK);
    item(c, a, 0, 2);
    item(c, a, 1, 20);
    item(c, a, 2, 100);
    assert(fvm_table_set_index(c, a, 2, values + 1) == FVM_OK);
    fvm_value_number(&flags, 20);
    assert(fvm_object_call(c, a, "sort", 1, &flags, &out) == FVM_OK && out.number == 0);
    item(c, a, 0, 2);
    item(c, a, 1, 20);
    item(c, a, 2, 2);
    fvm_value_string(values, "23");
    fvm_value_string(values + 1, "5");
    assert(fvm_construct(c, "Array", 2, values, &a) == FVM_OK);
    fvm_value_number(&flags, 16);
    assert(fvm_object_call(c, a, "sort", 1, &flags, &out) == FVM_OK);
    assert(fvm_table_get_index(c, a, 0, &out) == FVM_OK && !strcmp(out.string, "23"));
    fvm_value_string(values, "\356\200\200");
    fvm_value_string(values + 1, "\360\220\200\200");
    assert(fvm_construct(c, "Array", 2, values, &a) == FVM_OK);
    assert(fvm_object_call(c, a, "sort", 0, 0, &out) == FVM_OK);
    assert(fvm_table_get_index(c, a, 0, &out) == FVM_OK && !strcmp(out.string, "\360\220\200\200"));
    assert(fvm_construct(c, "Array", 0, 0, &records) == FVM_OK);
    for (i = 0; i < 3; ++i)
    {
        assert(fvm_construct(c, "Object", 0, 0, &record) == FVM_OK);
        fvm_value_number(&out, 3 - i);
        assert(fvm_object_set(c, record, "rank", &out) == FVM_OK);
        object(&out, record);
        assert(fvm_table_set_index(c, records, i, &out) == FVM_OK);
    }
    fvm_value_string(options, "rank");
    fvm_value_number(options + 1, 16);
    assert(fvm_object_call(c, records, "sortOn", 2, options, &out) == FVM_OK &&
           out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_table_get_index(c, records, 0, &out) == FVM_OK);
    assert(fvm_object_get(c, out.object, "rank", &out) == FVM_OK && out.number == 1);
    /* Indexed attributes survive moves; readonly length rejects extension. */
    fvm_value_number(values, 1);
    fvm_value_number(values + 1, 2);
    assert(fvm_construct(c, "Array", 2, values, &a) == FVM_OK);
    assert(fvm_object_set_property_flags(c, a, "0", 4, 0) == FVM_OK);
    assert(fvm_object_call(c, a, "reverse", 0, 0, &out) == FVM_OK);
    item(c, a, 0, 1);
    item(c, a, 1, 1);
    assert(fvm_object_set_property_flags(c, a, "1", 2, 0) == FVM_OK);
    fvm_value_number(&out, 0);
    assert(fvm_object_set(c, a, "length", &out) == FVM_OK);
    assert(fvm_object_get(c, a, "length", &out) == FVM_OK && out.number == 2);
    assert(fvm_object_set_property_flags(c, a, "length", 4, 0) == FVM_OK);
    fvm_value_number(&out, 9);
    assert(fvm_table_set_index(c, a, 2, &out) == FVM_OK);
    assert(fvm_table_get_index(c, a, 2, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_channel_create(&ch) == FVM_OK);
    object(&out, records);
    assert(fvm_channel_send_value(c, ch, &out, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK);
    fvm_value_number(options + 1, 18);
    assert(fvm_object_call(other, copy.object, "sortOn", 2, options, &out) == FVM_OK);
    assert(fvm_table_get_index(other, copy.object, 0, &out) == FVM_OK);
    assert(fvm_object_get(other, out.object, "rank", &out) == FVM_OK && out.number == 3);
    fvm_channel_destroy(ch);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
