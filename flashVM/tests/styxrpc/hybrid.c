/* Copyright (c) 2026 bhgv
 * EN: Verify table graph identity, exact big values and atomic mixed updates against Dis.
 * RU: Проверка идентичности, точного big и атомарных смешанных обновлений с Dis.
 */
#include "hybrid-test.h"
#include "managed-test.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/property.h"
#include "flashvm/styxrpc_hybrid.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <string.h>

#define CHANGE_SIGNATURE UINT32_C(0x32bb77a4) /* Compiled (h,a[h],r[hhi]) -> (h,h). */
#define EMPTY_SIGNATURE UINT32_C(0xf9bff31f) /* Compiled void -> h. */
#define HYBRID_WAIT_MS 3000U /* Bounded reply wait for the fixture. */

#define ACTION_END 0x00           /* End an AVM1 body. */
#define ACTION_POP 0x17           /* Drop a completed method result. */
#define ACTION_GET_VARIABLE 0x1c  /* Resolve an argument or local. */
#define ACTION_SET_VARIABLE 0x1d  /* Bind the fresh table. */
#define ACTION_THROW 0x2a         /* Reject after the edits. */
#define ACTION_CALL_FUNCTION 0x3d /* Call the table constructor. */
#define ACTION_RETURN 0x3e        /* Return the result Array. */
#define ACTION_INIT_ARRAY 0x42    /* Construct the two-element result. */
#define ACTION_GET_MEMBER 0x4e    /* Resolve one Array slot. */
#define ACTION_SET_MEMBER 0x4f    /* Write the stamp in the ADT Array. */
#define ACTION_CALL_METHOD 0x52   /* Invoke the checked table receiver. */
#define ACTION_PUSH 0x96          /* Header for AVM1 literal operands. */

#define STR(length, ...) ACTION_PUSH, (length) + 2, 0, 0, __VA_ARGS__, 0 /* Counted push header. */
#define INT(n) ACTION_PUSH, 5, 0, 7, n, 0, 0, 0 /* One small nonnegative int. */
#define GET(n) STR(1, n), ACTION_GET_VARIABLE /* Resolve a one-letter local. */
#define NEW_TABLE INT(0), STR(8, 'n','e','w','T','a','b','l','e'), ACTION_CALL_FUNCTION
#define SET_INDEX STR(8, 's','e','t','I','n','d','e','x'), ACTION_CALL_METHOD, ACTION_POP
#define SET_TEXT STR(7, 's','e','t','T','e','x','t'), ACTION_CALL_METHOD, ACTION_POP
#define TEXT_ONE STR(1, '1')
#define TABLE_EDITS \
    INT(1), INT(1), GET('p'), INT(0), ACTION_GET_MEMBER, \
        STR(11, 'd','e','l','e','t','e','I','n','d','e','x'), ACTION_CALL_METHOD, ACTION_POP, \
    INT(41), INT(2), INT(2), GET('t'), SET_INDEX, \
    TEXT_ONE, INT(1), GET('t'), STR(10, 'd','e','l','e','t','e','T','e','x','t'), ACTION_CALL_METHOD, ACTION_POP, \
    INT(29), TEXT_ONE, INT(2), GET('t'), SET_TEXT, \
    INT(52), STR(3, 'n','e','w'), INT(2), GET('t'), SET_TEXT, \
    INT(91), STR(7, 'u','p','d','a','t','e','d'), INT(2), GET('b'), INT(1), ACTION_GET_MEMBER, SET_TEXT, \
    GET('b'), INT(2), INT(19), ACTION_SET_MEMBER /* Mutate tables and bag stamp on the same owner. */

static const StyxRpcSchema change_shape = {"(ha[h]r[hhi])", "((hh))"}; /* Shared table/ADT graph. */
static const StyxRpcSchema empty_shape = {"()", "(h)"}; /* Result-only table graph. */


/* EN: Root an AVM1 function with the shared graph parameter order.
 * RU: Удержать функцию AVM1 с общим порядком параметров графа. */
static int define(FVM_Context *context, const char *name, const unsigned char *actions,
    size_t size, unsigned count)
{
    const char *parameters[] = {"t", "p", "b"};
    FVM_Object *function;
    FVM_Value value;
    if (fvm_function_create_avm1(context, actions, size, count, parameters, NULL, &function) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, name, &value) == FVM_OK;
}


/* EN/RU: See hybrid-test.h. / См. hybrid-test.h. */
int test_hybrid_serve(FVM_Context *context, int fd)
{
    static const unsigned char change[] = {
        TABLE_EDITS,
        STR(1, 'f'), NEW_TABLE, ACTION_SET_VARIABLE,
        STR(20, '-','9','2','2','3','3','7','2','0','3','6','8','5','4','7','7','5','8','0','8'),
        STR(8, 'r','e','t','u','r','n','e','d'), INT(2), GET('f'), SET_TEXT,
        GET('f'), GET('t'), INT(2), ACTION_INIT_ARRAY, ACTION_RETURN, ACTION_END
    };
    static const unsigned char reject[] = {TABLE_EDITS, STR(1, 'x'), ACTION_THROW, ACTION_END};
    static const unsigned char empty[] = {NEW_TABLE, ACTION_RETURN, ACTION_END};
    unsigned char buffer[STYXRPC_MANAGED_LIMIT];
    FVM_StyxRpcManagedExport bindings[] = {
        {context, "change", change_shape, buffer, sizeof(buffer)},
        {context, "reject", change_shape, buffer, sizeof(buffer)},
        {context, "empty", empty_shape, buffer, sizeof(buffer)}
    };
    StyxRpcGraphExport exports[3];
    StyxRpcGraphModule module;
    if (fvm_styxrpc_hybrid_bind(context, "newTable") != FVM_OK ||
        !define(context, "change", change, sizeof(change), 3) ||
        !define(context, "reject", reject, sizeof(reject), 3) ||
        !define(context, "empty", empty, sizeof(empty), 0) ||
        fvm_styxrpc_export_graph(bindings, "change", CHANGE_SIGNATURE, exports) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 1, "reject", CHANGE_SIGNATURE, exports + 1) != FVM_OK ||
        fvm_styxrpc_export_graph(bindings + 2, "empty", EMPTY_SIGNATURE, exports + 2) != FVM_OK ||
        styxrpc_graph_module_init(&module, "flash-hybrid.dis", exports, 3) != STYXRPC_OK)
    {
        fputs("FlashVM hybrid export setup failed\n", stderr);
        return 0;
    }
    RemotePhy *phy = phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
    StyxRpcServer *server = styxrpc_server_create(phy, &styxrpc_graph_module_tree, &module);
    if (!server)
    {
        return 0;
    }
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return 1;
}


/* EN: Set one fixture scalar through the public table method.
 * RU: Записать скаляр через публичный метод таблицы. */
static int store(FVM_Context *context, const FVM_Value *table, const char *method,
    const FVM_Value *key, int number)
{
    FVM_Value args[2], out;
    args[0] = *key;
    fvm_value_number(args + 1, number);
    return fvm_object_call(context, table->object, method, 2, args, &out) == FVM_OK &&
        out.type == FVM_VALUE_BOOLEAN && out.boolean;
}


/* EN: Seed both key namespaces and a signed-64 index using exact AS values.
 * RU: Заполнить оба пространства и 64-битный индекс точными значениями AS. */
static int seed(FVM_Context *context, const FVM_Value *table)
{
    static const char counted_key[] = {'a', 0, (char)0xd0, (char)0xb1};
    FVM_Value key;
    fvm_value_number(&key, 0);
    if (!store(context, table, "setIndex", &key, 0))
    {
        return 0;
    }
    fvm_value_number(&key, 1);
    return store(context, table, "setIndex", &key, 19) &&
        fvm_value_string(&key, "1") == FVM_OK && store(context, table, "setText", &key, 29) &&
        fvm_value_string(&key, "9223372036854775807") == FVM_OK && store(context, table, "setIndex", &key, 7) &&
        fvm_value_string_bytes(&key, counted_key, sizeof(counted_key)) == FVM_OK &&
        store(context, table, "setText", &key, 31) &&
        fvm_value_string(&key, "after") == FVM_OK && store(context, table, "setText", &key, 33);
}


/* EN: Check a numeric table method result or an unchanged bag field.
 * RU: Сверить числовой результат метода или сохранённое поле сумки. */
static int measure(FVM_Context *context, const FVM_Value *owner, const char *name,
    const FVM_Value *key, int number)
{
    FVM_Value out;
    return owner->type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, owner->object, name, key != NULL, key, &out) == FVM_OK &&
        out.type == FVM_VALUE_NUMBER && out.number == number;
}


/* EN: Check the complete pre-call graph after refusing a remote or local change.
 * RU: Сверить исходный граф после удалённого или локального отказа записи. */
static int unchanged(FVM_Context *context, const FVM_Value *args, const FVM_Value *inner)
{
    FVM_Value stamp, key, last;
    fvm_value_number(&key, 1);
    fvm_value_number(&last, -1);
    return fvm_object_get(context, args[2].object, "2", &stamp) == FVM_OK &&
        stamp.type == FVM_VALUE_NUMBER && stamp.number == 3 &&
        measure(context, args, "length", NULL, 2) &&
        measure(context, args, "count", NULL, 6) &&
        measure(context, args, "getIndex", &key, 19) &&
        measure(context, args, "getIndex", &last, 19) &&
        measure(context, inner, "count", NULL, 0);
}


/* EN: Check the failure phase before clearing the script exception.
 * RU: Сверить этап отказа перед очисткой исключения AS. */
static int failure(FVM_Context *context, const char *expected)
{
    FVM_Value error;
    return fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING &&
        !strcmp(error.string, expected) && fvm_exception_clear(context) == FVM_OK;
}


/* EN: Return the identity of one own slot in a graph or result.
 * RU: Сверить идентичность собственного слота графа или результата. */
static int alias(FVM_Context *context, const FVM_Value *owner, const char *slot, const FVM_Value *expected)
{
    FVM_Value field;
    return owner->type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, owner->object, slot, &field) == FVM_OK &&
        field.type == FVM_VALUE_OBJECT && field.object == expected->object;
}


/* EN: Verify sparse copy-back, preserved aliases and full signed-64 returned text.
 * RU: Сверить разреженную запись, алиасы и точный возвращённый знаковый big. */
static int check_result(FVM_Context *context, const FVM_Value *args, const FVM_Value *inner,
    const FVM_Value *result)
{
    FVM_Value fresh, key, value;
    fvm_value_number(&key, 2);
    return alias(context, result, "0", args) && alias(context, args + 2, "0", args) &&
        alias(context, args + 2, "1", inner) && alias(context, args + 1, "0", args) &&
        measure(context, args, "length", NULL, 1) && measure(context, args, "count", NULL, 7) &&
        measure(context, args, "getIndex", &key, 41) &&
        fvm_object_get(context, args[2].object, "2", &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 19 &&
        fvm_value_string(&key, "updated") == FVM_OK && measure(context, inner, "getText", &key, 91) &&
        fvm_object_get(context, result->object, "1", &fresh) == FVM_OK && fresh.type == FVM_VALUE_OBJECT &&
        fresh.object != args->object && fvm_value_string(&key, "returned") == FVM_OK &&
        fvm_object_call(context, fresh.object, "getText", 1, &key, &value) == FVM_OK &&
        value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 20 &&
        !memcmp(value.string, "-9223372036854775808", 20);
}


/* EN/RU: See hybrid-test.h. / См. hybrid-test.h. */
int test_hybrid_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    FVM_GCRootScope *roots = NULL;
    FVM_Value args[3], bag[3], peers[2], result, error;
    unsigned long reclaimed;
    target->module = "mounted/hybrid.dis";
    target->name = "empty";
    target->signature = EMPTY_SIGNATURE;
    target->schema = empty_shape;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = HYBRID_WAIT_MS;
    int ok = fvm_gc_roots_begin(context, &roots) == FVM_OK &&
        fvm_styxrpc_hybrid_bind(context, "newTable") == FVM_OK &&
        fvm_styxrpc_import_graph(context, "tableRPC", target) == FVM_OK &&
        fvm_call_bytes(context, "tableRPC", 8, 0, NULL, &result) == FVM_OK &&
        measure(context, &result, "count", NULL, 0) &&
        fvm_call_bytes(context, "newTable", 8, 0, NULL, args) == FVM_OK &&
        fvm_gc_roots_add(roots, args) == FVM_OK && seed(context, args) &&
        fvm_call_bytes(context, "newTable", 8, 0, NULL, bag + 1) == FVM_OK &&
        fvm_gc_roots_add(roots, bag + 1) == FVM_OK;
    if (ok)
    {
        bag[0] = peers[0] = args[0];
        fvm_value_number(bag + 2, 3);
        fvm_value_undefined(peers + 1);
        peers[1].type = FVM_VALUE_NULL;
        ok = fvm_exec_literal_array(context, 3, bag, args + 2) == FVM_OK &&
            fvm_gc_roots_add(roots, args + 2) == FVM_OK &&
            fvm_exec_literal_array(context, 2, peers, args + 1) == FVM_OK &&
            fvm_gc_roots_add(roots, args + 1) == FVM_OK &&
            fvm_gc_collect(context, &reclaimed) == FVM_OK;
    }
    if (ok)
    {
        target->name = "reject";
        target->signature = CHANGE_SIGNATURE;
        target->schema = change_shape;
        ok = fvm_call_bytes(context, "tableRPC", 8, 3, args, &result) == FVM_EXCEPTION &&
            failure(context, "Styx RPC remote function failed") && unchanged(context, args, bag + 1);
    }
    if (ok)
    {
        target->name = "change";
        ok = fvm_object_set_property_flags(context, args[2].object, "2", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK &&
            fvm_call_bytes(context, "tableRPC", 8, 3, args, &result) == FVM_EXCEPTION &&
            failure(context, "Styx RPC graph result could not be committed") && unchanged(context, args, bag + 1) &&
            fvm_object_set_property_flags(context, args[2].object, "2", 0, FVM_PROPERTY_READ_ONLY) == FVM_OK &&
            fvm_call_bytes(context, "tableRPC", 8, 3, args, &result) == FVM_OK &&
            check_result(context, args, bag + 1, &result);
    }
    if (!ok && fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "FlashVM hybrid import %s failed: %s\n", target->name, error.string);
    }
    if (roots)
    {
        fvm_gc_roots_end(roots);
    }
    target->session = NULL;
    return ok;
}
