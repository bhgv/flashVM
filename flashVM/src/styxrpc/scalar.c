/* Copyright (c) 2026 bhgv
 * EN: Translate exact signed Int32 values between a FlashVM owner and libstyxrpc.
 * RU: Передача точных знаковых Int32 между владельцем FlashVM и libstyxrpc.
 */
#include "flashvm/styxrpc.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "styxrpc/scalar_client.h"


/* EN: Extract a finite exact Int32 from the borrowed ActionScript primitive.
 * RU: Извлечь конечный точный Int32 из заимствованного примитива ActionScript. */
static int scalar_int(const FVM_Value *value, int64_t *number)
{
    if (value->type != FVM_VALUE_NUMBER ||
        !(value->number >= INT32_MIN && value->number <= INT32_MAX))
    {
        return 0;
    }
    *number = (int64_t)value->number;
    return (double)*number == value->number;
}


/* EN: Report a bridge outcome as a script exception in the active context.
 * RU: Сообщить результат ошибки моста как исключение в активном контексте. */
static FVM_Result raise_status(FVM_Context *context, const char *message)
{
    FVM_Value error;
    FVM_Result status = fvm_value_string(&error, message);
    return status == FVM_OK ? fvm_exception_raise(context, &error) : status;
}


/* EN: Borrow binding from userdata and call once on the context owner thread.
 * RU: Заимствовать binding из userdata и вызвать один раз в потоке владельца. */
static FVM_Result invoke_dis(FVM_Context *context, FVM_Object *self, unsigned int argc,
    const FVM_Value *argv, FVM_Value *result, void *userdata)
{
    FVM_StyxRpcImport *binding = userdata;
    int64_t argument;
    int64_t answer;
    StyxRpcOutcome outcome;
    (void)self;
    if (argc != 1 || !scalar_int(argv, &argument))
    {
        return raise_status(context, "Styx RPC requires one exact int32 argument");
    }
    outcome = styxrpc_scalar_invoke(binding->session, binding->module, binding->name,
        STYXRPC_INT_SIGNATURE, argument, binding->wait_ms, &answer);
    switch (outcome)
    {
    case STYXRPC_CALL_OK:
        fvm_value_number(result, (double)answer);
        return FVM_OK;
    case STYXRPC_CALL_NOT_SENT:
        return raise_status(context, "Styx RPC call was not sent");
    case STYXRPC_CALL_REMOTE_ERROR:
        return raise_status(context, "Styx RPC remote function failed");
    default:
        return raise_status(context, "Styx RPC outcome is uncertain");
    }
}


/* EN/RU: See flashvm/styxrpc.h. / См. flashvm/styxrpc.h. */
FVM_Result fvm_styxrpc_import_int(FVM_Context *context, const char *global, FVM_StyxRpcImport *binding)
{
    if (!context || !global || !*global || !binding || !binding->session || !binding->module ||
        !*binding->module || !binding->name || !*binding->name || !binding->wait_ms ||
        binding->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_register_function(context, global, invoke_dis, binding);
}


/* [ASINC] EN: Execute the named FlashVM function and encode only an exact Int32 result.
 * RU: Выполнить именованную функцию FlashVM и вернуть только точный Int32. */
static int invoke_flash(void *userdata, int64_t argument, int64_t *result)
{
    const FVM_StyxRpcExport *binding = userdata;
    FVM_Value arg;
    FVM_Value value;
    FVM_Result status;
    if (argument < INT32_MIN || argument > INT32_MAX ||
        fvm_exception_get(binding->context, &value) != FVM_NOT_FOUND)
    {
        return 0;
    }
    fvm_value_number(&arg, (double)argument);
    status = fvm_avm1_call(binding->context, binding->function, NULL, 1, &arg, &value);
    if (status == FVM_EXCEPTION)
    {
        fvm_exception_clear(binding->context);
    }
    return status == FVM_OK && scalar_int(&value, result);
}


/* EN/RU: See flashvm/styxrpc.h. / См. flashvm/styxrpc.h. */
FVM_Result fvm_styxrpc_export_int(FVM_StyxRpcExport *binding, const char *name, struct StyxRpcExport *exported)
{
    StyxRpcExport descriptor;
    StyxRpcCall checked;
    if (!binding || !binding->context || !binding->function || !*binding->function ||
        !name || !*name || !exported)
    {
        return FVM_INVALID_ARGUMENT;
    }
    descriptor.context = binding;
    descriptor.name = name;
    descriptor.signature = STYXRPC_INT_SIGNATURE;
    descriptor.invoke = invoke_flash;
    if (styxrpc_open(&checked, &descriptor) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *exported = descriptor;
    return FVM_OK;
}
