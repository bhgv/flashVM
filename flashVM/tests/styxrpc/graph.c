/* Copyright (c) 2026 bhgv
 * EN: Accept native positional graph updates and preserved return aliases.
 * RU: Проверка нативных позиционных изменений графа и алиасов в результате.
 */
#include "graph-test.h"
#include "managed-test.h"
#include "flashvm/execution.h"
#include "flashvm/exception.h"
#include "flashvm/function.h"
#include "flashvm/gc_roots.h"
#include "flashvm/property.h"
#include "drivers/posix/fd.h"
#include <stdio.h>

#define NESTED_SIGNATURE UINT32_C(0xe74e7a73) /* fn(ref Box): ref Leaf. */
#define REPLACE_SIGNATURE UINT32_C(0xa3c1acac) /* fn(ref Box, int): ref Leaf. */
#define MAKE_SIGNATURE UINT32_C(0x7e526b47)   /* fn(int): ref Leaf. */
#define FAN_SIGNATURE UINT32_C(0x9a25779e) /* fn(array of ref Leaf): ref Leaf. */
#define TUPLE_SIGNATURE UINT32_C(0xa07475bb) /* fn(ref TupleBox, int): ref Leaf. */
#define GRAPH_WAIT_MS 3000U /* Bounded result polling. */

#define ACTION_END 0x00           /* End an AVM1 function body. */
#define ACTION_ADD 0x0a           /* Numeric increment body. */
#define ACTION_GET_VARIABLE 0x1c  /* Resolve one local name. */
#define ACTION_SET_VARIABLE 0x1d  /* Bind the temporary child. */
#define ACTION_THROW 0x2a         /* Raise after changing a child. */
#define ACTION_RETURN 0x3e        /* Return the existing or fresh child. */
#define ACTION_INIT_ARRAY 0x42    /* Construct the new ref representation. */
#define ACTION_GET_MEMBER 0x4e    /* Read a native Array slot. */
#define ACTION_SET_MEMBER 0x4f    /* Write a native Array slot. */
#define ACTION_PUSH 0x96          /* Header for typed stack inputs. */
#define PUSH_NAME(name) ACTION_PUSH, 3, 0, 0, name, 0 /* Push one-letter name. */
#define GET(name) PUSH_NAME(name), ACTION_GET_VARIABLE /* Read one-letter local. */
#define PUSH_INT(value) ACTION_PUSH, 5, 0, 7, value, 0, 0, 0 /* Push small nonnegative int. */
#define LOAD_ITEM(index) PUSH_NAME('l'), GET('b'), PUSH_INT(index), ACTION_GET_MEMBER, ACTION_SET_VARIABLE /* l = b[index]. */
#define LOAD_CHILD LOAD_ITEM(0)
#define INCREMENT GET('l'), PUSH_INT(0), GET('l'), PUSH_INT(0), ACTION_GET_MEMBER, PUSH_INT(1), ACTION_ADD, ACTION_SET_MEMBER /* l[0]++. */
#define NEW_LEAF GET('v'), PUSH_INT(1), ACTION_INIT_ARRAY /* [v]. */

static const StyxRpcSchema nested_shape = {"(r[r[i]a[y]])", "(r[i])"}; /* Nested original. */
static const StyxRpcSchema replace_shape = {"(r[r[i]a[y]]i)", "(r[i])"}; /* Fresh child and result. */
static const StyxRpcSchema make_shape = {"(i)", "(r[i])"}; /* Result-only graph. */
static const StyxRpcSchema fan_shape = {"(a[r[i]])", "(r[i])"}; /* Array of original refs. */
static const StyxRpcSchema tuple_shape = {"(r[(ir[i])a[y]]i)", "(r[i])"}; /* Sparse tuple lane. */


/* EN: Create a callable AVM1 body and root it under the named global.
 * RU: Создать функцию AVM1 и удержать её как именованный глобал. */
static int define(FVM_Context *context, const char *name, const unsigned char *actions, size_t size,
    unsigned count, const char *const *params)
{
    FVM_Object *function = NULL;
    FVM_Value value;
    if (fvm_function_create_avm1(context, actions, size, count, params, NULL, &function) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, name, &value) == FVM_OK;
}


/* EN/RU: See graph-test.h. / См. graph-test.h. */
int test_graph_serve(FVM_Context *context, int fd)
{
    static const unsigned char nested[] = {LOAD_CHILD, INCREMENT, GET('l'), ACTION_RETURN, ACTION_END};
    static const unsigned char fail[] = {LOAD_CHILD, INCREMENT, GET('l'), ACTION_THROW, ACTION_END};
    static const unsigned char replace[] = {
        PUSH_NAME('l'), NEW_LEAF, ACTION_SET_VARIABLE,
        GET('b'), PUSH_INT(0), GET('l'), ACTION_SET_MEMBER, GET('l'), ACTION_RETURN, ACTION_END
    };
    static const unsigned char make[] = {NEW_LEAF, ACTION_RETURN, ACTION_END};
    static const unsigned char fan[] = {
        LOAD_ITEM(12), INCREMENT, GET('b'), PUSH_INT(0), ACTION_GET_MEMBER, ACTION_RETURN, ACTION_END
    };
    static const unsigned char tupleedit[] = {
        LOAD_CHILD, GET('l'), PUSH_INT(0), GET('v'), ACTION_SET_MEMBER,
        GET('l'), PUSH_INT(1), ACTION_GET_MEMBER, ACTION_RETURN, ACTION_END
    };
    const char *box_params[] = {"b", "v"};
    const char *make_params[] = {"v"};
    unsigned char buffer[STYXRPC_MANAGED_LIMIT];
    FVM_StyxRpcManagedExport bindings[] = {
        {context, "nested", nested_shape, buffer, sizeof(buffer)},
        {context, "replace", replace_shape, buffer, sizeof(buffer)},
        {context, "make", make_shape, buffer, sizeof(buffer)},
        {context, "fail", nested_shape, buffer, sizeof(buffer)},
        {context, "fan", fan_shape, buffer, sizeof(buffer)},
        {context, "tupleedit", tuple_shape, buffer, sizeof(buffer)}
    };
    StyxRpcGraphExport exports[sizeof(bindings) / sizeof(bindings[0])];
    StyxRpcGraphModule module;
    if (!define(context, "nested", nested, sizeof(nested), 1, box_params) ||
        !define(context, "replace", replace, sizeof(replace), 2, box_params) ||
        !define(context, "make", make, sizeof(make), 1, make_params) ||
        !define(context, "fail", fail, sizeof(fail), 1, box_params) ||
        !define(context, "fan", fan, sizeof(fan), 1, box_params) ||
        !define(context, "tupleedit", tupleedit, sizeof(tupleedit), 2, box_params) ||
        fvm_styxrpc_export_graph(bindings, "nested", NESTED_SIGNATURE, exports) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 1, "replace", REPLACE_SIGNATURE, exports + 1) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 2, "make", MAKE_SIGNATURE, exports + 2) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 3, "fail", NESTED_SIGNATURE, exports + 3) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 4, "fan", FAN_SIGNATURE, exports + 4) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 5, "tupleedit", TUPLE_SIGNATURE, exports + 5) != FVM_OK ||
        styxrpc_graph_module_init(&module, "flash-graph.dis", exports, sizeof(exports) / sizeof(exports[0])) != STYXRPC_OK)
    {
        fputs("FlashVM graph export setup failed\n", stderr);
        return 0;
    }
    RemotePhy *phy = phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    StyxRpcServer *server = styxrpc_server_create(phy, &styxrpc_graph_module_tree, &module);
    while (styxrpc_server_step(server))
    {
    }
    int ok = server != NULL;
    styxrpc_server_destroy(server);
    return ok;
}


/* EN: Read one field, checking exact object identity.
 * RU: Прочитать поле и сверить точную идентичность объекта. */
static int same_child(FVM_Context *context, const FVM_Value *box, const char *key, const FVM_Value *child)
{
    FVM_Value value;
    return child->type == FVM_VALUE_OBJECT && fvm_object_get(context, box->object, key, &value) == FVM_OK &&
        value.type == FVM_VALUE_OBJECT && value.object == child->object;
}


/* EN: Check one field of a returned native Leaf.
 * RU: Проверить поле возвращённого нативного Leaf. */
static int leaf_is(FVM_Context *context, const FVM_Value *leaf, int count)
{
    FVM_Value value;
    return leaf->type == FVM_VALUE_OBJECT && fvm_object_get(context, leaf->object, "0", &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == count;
}


/* EN: Use the bound import with one exact compiled offer and selected arguments.
 * RU: Вызвать привязанный импорт с точным компилированным предложением. */
static FVM_Result call(FVM_Context *context, StyxRpcManagedTarget *target, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, unsigned count, const FVM_Value *values, FVM_Value *result)
{
    target->name = name;
    target->signature = signature;
    target->schema = *schema;
    return fvm_call_bytes(context, "graph", 5, count, values, result);
}


/* EN: Preserve an array alias and resolve a sparse ID beyond the ordinary ref limit.
 * RU: Сохранить алиас массива и разрешить разреженный ID за лимитом обычного ref. */
static int test_fan(FVM_Context *context, StyxRpcManagedTarget *target)
{
    FVM_Value leaves[13], count, array, result;
    FVM_GCRootScope *roots = NULL;
    unsigned i;
    int ok = fvm_gc_roots_begin(context, &roots) == FVM_OK;
    for (i = 0; ok && i < 13; ++i)
    {
        fvm_value_number(&count, i);
        ok = fvm_exec_literal_array(context, 1, &count, leaves + i) == FVM_OK &&
            fvm_gc_roots_add(roots, leaves + i) == FVM_OK;
    }
    if (ok)
    {
        leaves[11] = leaves[0];
        ok = fvm_exec_literal_array(context, 13, leaves, &array) == FVM_OK &&
            fvm_gc_roots_add(roots, &array) == FVM_OK &&
            call(context, target, "fan", FAN_SIGNATURE, &fan_shape, 1, &array, &result) == FVM_OK &&
            result.type == FVM_VALUE_OBJECT && result.object == leaves[0].object &&
            same_child(context, &array, "11", &result) && leaf_is(context, leaves, 0) &&
            leaf_is(context, leaves + 12, 13);
    }
    if (roots)
    {
        fvm_gc_roots_end(roots);
    }
    return ok;
}


/* EN: Rebuild a changed tuple path while keeping the nested ref and sibling array.
 * RU: Пересобрать изменённый путь кортежа, сохранив вложенный ref и соседний массив. */
static int test_tuple(FVM_Context *context, StyxRpcManagedTarget *target, const FVM_Value *fields)
{
    FVM_Value pair[2], box[2], args[2], result, updated;
    fvm_value_number(pair, 3);
    pair[1] = fields[0];
    box[1] = fields[1];
    fvm_value_number(args + 1, 52);
    return fvm_exec_literal_array(context, 2, pair, box) == FVM_OK &&
        fvm_set_global(context, "originalPair", box) == FVM_OK &&
        fvm_exec_literal_array(context, 2, box, args) == FVM_OK &&
        fvm_set_global(context, "originalTupleBox", args) == FVM_OK &&
        call(context, target, "tupleedit", TUPLE_SIGNATURE, &tuple_shape, 2, args, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && result.object == fields[0].object &&
        fvm_object_get(context, args->object, "0", &updated) == FVM_OK &&
        leaf_is(context, &updated, 52) && same_child(context, &updated, "1", fields) &&
        leaf_is(context, box, 3) && same_child(context, args, "1", fields + 1);
}


/* EN/RU: See graph-test.h. / См. graph-test.h. */
int test_graph_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    FVM_Value count, fields[2], args[2], result, error;
    target->module = "mounted/graphs.dis";
    target->name = "nested";
    target->signature = NESTED_SIGNATURE;
    target->schema = nested_shape;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = GRAPH_WAIT_MS;
    fvm_value_number(&count, 8);
    fvm_value_number(args + 1, 37);
    int ok = fvm_styxrpc_import_graph(context, "graph", target) == FVM_OK &&
        fvm_exec_literal_array(context, 1, &count, fields) == FVM_OK &&
        fvm_set_global(context, "originalLeaf", fields) == FVM_OK &&
        fvm_exec_literal_array(context, 1, &count, fields + 1) == FVM_OK &&
        fvm_set_global(context, "originalBytes", fields + 1) == FVM_OK &&
        fvm_exec_literal_array(context, 2, fields, args) == FVM_OK &&
        fvm_set_global(context, "originalBox", args) == FVM_OK &&
        call(context, target, "nested", NESTED_SIGNATURE, &nested_shape, 1, args, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && result.object == fields[0].object &&
        leaf_is(context, &result, 9) && same_child(context, args, "1", fields + 1) &&
        call(context, target, "fail", NESTED_SIGNATURE, &nested_shape, 1, args, &result) == FVM_EXCEPTION &&
        fvm_exception_clear(context) == FVM_OK && leaf_is(context, fields, 9);
    if (ok)
    {
        ok = fvm_object_set_property_flags(context, args->object, "0", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK &&
            call(context, target, "replace", REPLACE_SIGNATURE, &replace_shape, 2, args, &result) == FVM_EXCEPTION &&
            fvm_exception_clear(context) == FVM_OK && same_child(context, args, "0", fields) &&
            fvm_object_set_property_flags(context, args->object, "0", 0, FVM_PROPERTY_READ_ONLY) == FVM_OK &&
            call(context, target, "replace", REPLACE_SIGNATURE, &replace_shape, 2, args, &result) == FVM_OK &&
            leaf_is(context, &result, 37) && same_child(context, args, "0", &result) &&
            same_child(context, args, "1", fields + 1) && leaf_is(context, fields, 9) &&
            call(context, target, "make", MAKE_SIGNATURE, &make_shape, 1, &count, &result) == FVM_OK &&
            leaf_is(context, &result, 8) && result.object != fields[0].object &&
            test_fan(context, target) && test_tuple(context, target, fields);
    }
    if (!ok && fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "FlashVM graph import %s failed: %s\n", target->name, error.string);
    }
    target->session = NULL;
    return ok;
}
