/* Copyright (c) 2026 bhgv
 * EN: Bind exact Dis big values and typed keys to FlashVM owner-local methods.
 * RU: Привязка точных значений big и типов ключей к методам владельца FlashVM.
 */
#include "hybrid.h"
#include "flashvm/gc_roots.h"
#include "flashvm/userdata.h"

#define HYBRID_SAFE_BIG INT64_C(9007199254740991) /* Uniformly exact AS Number bound. */

typedef struct HybridMethod
{
    const char *name;                 /* Public AS member name. */
    StyxRpcHybridOperation operation; /* Checked SDK operation. */
    unsigned argc;                    /* Exact script argument count. */
    int text;                         /* Interpret the first argument as counted UTF-8. */
} HybridMethod;

static const HybridMethod methods[] = {
    {"getIndex", STYXRPC_HYBRID_GET, 1, 0},
    {"setIndex", STYXRPC_HYBRID_SET, 2, 0},
    {"deleteIndex", STYXRPC_HYBRID_DELETE, 1, 0},
    {"getText", STYXRPC_HYBRID_GET, 1, 1},
    {"setText", STYXRPC_HYBRID_SET, 2, 1},
    {"deleteText", STYXRPC_HYBRID_DELETE, 1, 1},
    {"length", STYXRPC_HYBRID_LENGTH, 0, 0},
    {"count", STYXRPC_HYBRID_COUNT, 0, 0}
}; /* Immutable owner callbacks and their exact script arity. */


/* EN: Accept only exact AS Numbers or counted signed decimal big strings.
 * RU: Принимать только точное число AS либо десятичную строку big с длиной. */
static int read_big(const FVM_Value *value, int64_t *out)
{
    if (value->type == FVM_VALUE_STRING)
    {
        return styxrpc_hybrid_big_parse(value->string, fvm_value_string_size(value), out);
    }
    if (value->type != FVM_VALUE_NUMBER ||
        !(value->number >= -(double)HYBRID_SAFE_BIG && value->number <= (double)HYBRID_SAFE_BIG))
    {
        return 0;
    }
    *out = (int64_t)value->number;
    return (double)*out == value->number;
}


/* [GC:] EN: Return an exact AS Number or a context-owned decimal string.
 * RU: Вернуть точное число AS или десятичную строку во владении контекста. */
static FVM_Result return_big(FVM_Context *context, int64_t number, FVM_Value *out)
{
    char bytes[STYXRPC_HYBRID_BIG_TEXT];
    unsigned short units[STYXRPC_HYBRID_BIG_TEXT];
    size_t size, i;
    if (number >= -HYBRID_SAFE_BIG && number <= HYBRID_SAFE_BIG)
    {
        fvm_value_number(out, (double)number);
        return FVM_OK;
    }
    size = styxrpc_hybrid_big_format(number, bytes, sizeof(bytes));
    for (i = 0; i < size; ++i)
    {
        units[i] = (unsigned char)bytes[i];
    }
    return size ? fvm_value_string_utf16(context, units, size, out) : FVM_ERROR;
}


/* EN: Decode the key in the namespace explicitly selected by the bound method.
 * RU: Прочитать ключ в пространстве, явно выбранном привязанным методом. */
static int read_key(const FVM_Value *value, int text, StyxRpcHybridKey *key)
{
    key->text = text;
    if (!text)
    {
        return read_big(value, &key->index);
    }
    if (value->type != FVM_VALUE_STRING)
    {
        return 0;
    }
    key->bytes = value->string;
    key->size = fvm_value_string_size(value);
    return 1;
}


/* [GC:] EN: Run one method on the verified receiver and publish its script result.
 * RU: Выполнить метод на проверенном получателе и выдать результат сценария. */
static FVM_Result apply(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const HybridMethod *method = state;
    StyxRpcHybrid *table = fvm_styxrpc_hybrid_lookup(context, self);
    StyxRpcHybridRequest request = {0};
    int ok;
    if (!table || argc != method->argc || (argc && !read_key(args, method->text, &request.key)) ||
        (argc == 2 && !read_big(args + 1, &request.value)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    request.operation = method->operation;
    ok = styxrpc_hybrid_apply(table, &request);
    if (request.operation == STYXRPC_HYBRID_GET)
    {
        fvm_value_undefined(out);
        return ok ? return_big(context, request.value, out) : FVM_OK;
    }
    if (!argc)
    {
        return ok && request.count <= INT64_MAX ?
            return_big(context, (int64_t)request.count, out) : FVM_ERROR;
    }
    fvm_value_boolean(out, ok);
    return FVM_OK;
}


/* [GC:] EN/RU: See hybrid.h. / См. hybrid.h. */
void fvm_styxrpc_hybrid_release(void *table)
{
    styxrpc_hybrid_close(table);
}


/* [GC:] EN/RU: See hybrid.h. / См. hybrid.h. */
FVM_Result fvm_styxrpc_hybrid_adopt(FVM_Context *context, StyxRpcHybrid *table, FVM_Value *out)
{
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Object *owner;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    unsigned i;
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_object_create(context, &owner);
    if (status == FVM_OK)
    {
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = owner;
        status = fvm_gc_roots_add(roots, &value);
    }
    for (i = 0; status == FVM_OK && i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        status = fvm_object_define_method_rooted(context, owner, methods[i].name,
            apply, (void *)(methods + i), NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_userdata_attach(context, owner, table, fvm_styxrpc_hybrid_release);
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN: Allocate one empty table and transfer it to AS only after publication.
 * RU: Выделить пустую таблицу и передать её AS после успешной публикации. */
static FVM_Result create(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    StyxRpcHybrid *table;
    FVM_Result status;
    (void)self;
    (void)args;
    (void)state;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    table = styxrpc_hybrid_open(NULL, 0);
    if (!table)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_styxrpc_hybrid_adopt(context, table, out);
    if (status != FVM_OK)
    {
        styxrpc_hybrid_close(table);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_hybrid.h. / См. flashvm/styxrpc_hybrid.h. */
FVM_Result fvm_styxrpc_hybrid_bind(FVM_Context *context, const char *global)
{
    return context && global && *global ? fvm_register_function(context, global, create, NULL) :
        FVM_INVALID_ARGUMENT;
}
