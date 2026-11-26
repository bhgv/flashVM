/* Copyright (c) 2026 bhgv
 * EN: Exercise the traced queue and alias owners at native collection boundaries.
 * RU: Проверить удержание владельца очередью и привязкой на границе сборки мусора.
 */
#include "flashvm/extensions.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "flashvm/table.h"
#include <stdio.h>


/* EN: Keep an owner reachable first from a queued ref and then from its alias.
 * RU: Удержать владельца сначала через ссылку в очереди, затем через привязку. */
static int exercise(FVM_Context *context)
{
    FVM_Value channel, queue, origin, args[2], ref, result;
    fvm_value_undefined(&origin);
    fvm_value_string(&args[0], "ref");
    fvm_value_number(&args[1], 1);
    if (fvm_extensions_install(context) != FVM_OK ||
        fvm_get_global(context, "channel", &channel) != FVM_OK ||
        fvm_object_call(context, channel.object, "create", 2, args, &queue) != FVM_OK ||
        fvm_set_global(context, "queue", &queue) != FVM_OK ||
        fvm_object_create(context, &origin.object) != FVM_OK)
    {
        return 0;
    }
    origin.type = FVM_VALUE_OBJECT;
    fvm_value_number(&args[1], 41);
    if (fvm_object_set(context, origin.object, "count", &args[1]) != FVM_OK)
    {
        return 0;
    }
    args[0] = origin;
    fvm_value_string(&args[1], "count");
    if (fvm_object_call(context, channel.object, "ref", 2, args, &ref) != FVM_OK ||
        fvm_object_call(context, queue.object, "send", 1, &ref, &result) != FVM_OK ||
        result.type != FVM_VALUE_BOOLEAN || !result.boolean || fvm_gc_collect(context, NULL) != FVM_OK ||
        fvm_object_call(context, queue.object, "recv", 0, NULL, &ref) != FVM_OK ||
        fvm_set_global(context, "alias", &ref) != FVM_OK ||
        fvm_gc_collect(context, NULL) != FVM_OK ||
        fvm_get_global(context, "alias", &result) != FVM_OK ||
        result.type != FVM_VALUE_NUMBER || result.number != 41)
    {
        return 0;
    }
    fvm_value_number(&result, 52);
    return fvm_set_global(context, "alias", &result) == FVM_OK &&
        fvm_object_get(context, origin.object, "count", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 52;
}


/* EN: Bind a native numeric entry, then remove it across a GC while retaining the named key.
 * RU: Привязать числовую запись и удалить её со сборкой мусора, сохранив строковый ключ. */
static int indexed(FVM_Context *context)
{
    FVM_Value origin, channel, key, value, args[2], ref, result;
    unsigned long seen;
    size_t count;
    unsigned char *wire = NULL;
    size_t wire_size = 0;
    fvm_value_undefined(&origin);
    if (fvm_object_create(context, &origin.object) != FVM_OK)
    {
        return 0;
    }
    origin.type = FVM_VALUE_OBJECT;
    fvm_value_number(&value, 41);
    if (fvm_set_global(context, "rawtable", &origin) != FVM_OK ||
        fvm_table_set_index(context, origin.object, 7, &value) != FVM_OK ||
        fvm_get_global(context, "channel", &channel) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, 200);
    if (fvm_object_set(context, origin.object, "7", &value) != FVM_OK ||
        fvm_object_set(context, origin.object, "8", &value) != FVM_OK)
    {
        return 0;
    }
    args[0] = origin;
    fvm_value_number(&args[1], 8);
    if (fvm_object_call(context, channel.object, "ref", 2, args, &ref) != FVM_NOT_FOUND)
    {
        return 0;
    }
    fvm_value_number(&args[1], 7);
    if (fvm_object_call(context, channel.object, "ref", 2, args, &ref) != FVM_OK ||
        fvm_table_set_index(context, origin.object, 7, &ref) != FVM_INVALID_ARGUMENT ||
        fvm_table_set_index(context, origin.object, 8, &ref) != FVM_OK ||
        fvm_gc_collect(context, NULL) != FVM_OK ||
        fvm_table_indexed_count(context, origin.object, &count) != FVM_OK || count != 2 ||
        fvm_table_indexed_at(context, origin.object, 1, &seen, &result) != FVM_OK ||
        seen != 8 || result.type != FVM_VALUE_NUMBER || result.number != 41)
    {
        return 0;
    }
    /* A graph snapshot must never transport the private backing object. */
    if (fvm_value_serialize(context, &origin, &wire, &wire_size) != FVM_UNSUPPORTED)
    {
        fvm_serialized_free(wire);
        return 0;
    }
    fvm_value_number(&value, 52);
    fvm_value_number(&key, 8);
    return fvm_table_set_index(context, origin.object, 8, &value) == FVM_OK &&
        fvm_table_get_index(context, origin.object, 7, &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 52 &&
        fvm_exec_delete_member(context, &origin, &key, &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean &&
        fvm_gc_collect(context, NULL) == FVM_OK &&
        fvm_table_get_index(context, origin.object, 8, &result) == FVM_OK &&
        result.type == FVM_VALUE_UNDEFINED &&
        fvm_object_get(context, origin.object, "8", &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 200;
}


/* EN: Run the native-root check in a fresh runtime and destroy all test state.
 * RU: Проверить корни в новой среде и освободить состояние теста. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    int ok = fvm_config_init(&config) == FVM_OK &&
        fvm_runtime_create(&config, &runtime) == FVM_OK &&
        fvm_context_create(runtime, &context) == FVM_OK && exercise(context) && indexed(context);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (!ok)
    {
        fputs("variable reference GC contract failed\n", stderr);
        return 1;
    }
    return 0;
}
