/* A separately compiled C module: ordinary rooted methods and an AS-write accessor. */
#include "module.h"
#include "flashvm/gc.h"
#include <math.h>


/* Read one exact signed32 argument without narrowing or invoking AS coercion.
 * Values are borrowed only during the callback; return0 for NaN, infinity,
 * fractions and nonnumeric inputs. Range comparisons also reject infinities. */
static int integer_argument(const FVM_Value *value, double *number)
{
    if (value->type != FVM_VALUE_NUMBER || value->number != value->number ||
        value->number < -2147483648.0 || value->number > 2147483647.0 || floor(value->number) != value->number)
    {
        return 0;
    }
    *number = value->number;
    return 1;
}


/* NativeDemo.add(a,b): validate borrowed arguments before changing state/result.
 * Double arithmetic is exact in this bounded integer domain and avoids C signed
 * overflow. userdata is the host-owned instance; no frame/heap address is a wire value. */
static FVM_Result add(FVM_Context *context, FVM_Object *receiver, unsigned int count,
                      const FVM_Value *arguments, FVM_Value *result, void *userdata)
{
    DemoNativeState *state = (DemoNativeState *)userdata;
    double a;
    double b;
    double sum;
    (void)context;
    (void)receiver;
    if (!state || count != 2 || !arguments || state->calls >= DEMO_NATIVE_CALL_LIMIT ||
        !integer_argument(&arguments[0], &a) || !integer_argument(&arguments[1], &b))
    {
        return FVM_INVALID_ARGUMENT;
    }
    sum = a + b;
    if (sum < -2147483648.0 || sum > 2147483647.0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    state->calls++;
    fvm_value_number(result, sum);
    return FVM_OK;
}


/* NativeDemo.seed(): read managed data after host-side collection. The installed
 * rooted method retains seed_owner; the C userdata pointer alone is not a GC root.
 * No explicit collection or off-thread VM access occurs inside this callback. */
static FVM_Result seed(FVM_Context *context, FVM_Object *receiver, unsigned int count,
                       const FVM_Value *arguments, FVM_Value *result, void *userdata)
{
    DemoNativeState *state = (DemoNativeState *)userdata;
    (void)receiver;
    (void)arguments;
    if (!state || count != 0 || !state->seed_owner)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_object_get(context, state->seed_owner, "value", result);
}


/* NativeDemo.accumulate = integer: executable by the current AS display profile.
 * Validate first, then update only host-owned scalar state. No callback writes
 * another global or waits for a Flash frame. The setter result is deliberately void. */
static FVM_Result accumulate(FVM_Context *context, FVM_Object *receiver, unsigned int count,
                             const FVM_Value *arguments, FVM_Value *result, void *userdata)
{
    DemoNativeState *state = (DemoNativeState *)userdata;
    double input;
    double sum;
    (void)context;
    (void)receiver;
    if (!state || count != 1 || !arguments || state->calls >= DEMO_NATIVE_CALL_LIMIT ||
        !integer_argument(&arguments[0], &input))
    {
        return FVM_INVALID_ARGUMENT;
    }
    sum = state->total + input;
    if (sum < -2147483648.0 || sum > 2147483647.0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    state->total = sum;
    state->calls++;
    fvm_value_undefined(result);
    return FVM_OK;
}


/* Build privately and publish only through output after all registrations succeed.
 * [GC:] Pin the capture before allocating the namespace, then root the namespace
 * across method/accessor creation. Unwind both temporary pins on every error path. */
FVM_Result demo_native_module_create(FVM_Context *context, DemoNativeState *state,
                                     FVM_Object *seed_owner, FVM_Object **output)
{
    FVM_Object *module = NULL;
    FVM_Result status;
    int module_pinned = 0;
    if (!context || !state || !seed_owner || !output || state->seed_owner || state->calls || state->total != 0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_pin(context, seed_owner);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_object_create(context, &module);
    if (status == FVM_OK)
    {
        status = fvm_gc_pin(context, module);
        module_pinned = status == FVM_OK;
    }
    if (status == FVM_OK)
    {
        state->seed_owner = seed_owner;
        status = fvm_object_define_method_rooted(context, module, "add", add, state, seed_owner);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, module, "seed", seed, state, seed_owner);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_accessor_rooted(context, module, "accumulate", NULL, accumulate, state, seed_owner);
    }
    if (status == FVM_OK)
    {
        *output = module;
    }
    else
    {
        state->seed_owner = NULL;
    }
    if (module_pinned)
    {
        fvm_gc_unpin(context, module);
    }
    fvm_gc_unpin(context, seed_owner);
    return status;
}
