/* FlashVM C89 tutorial: resolve an absent method to an original C Function. */
#include "flashvm/function.h" /* Construct native Functions and use object values. */
#include "flashvm/standard.h" /* Install the SWF<=8 standard environment. */
#include "flashvm/gc.h" /* Retain the receiver and explicit construction roots. */
#include "property_resolve_example.h" /* Keep tutorial callback counts named. */
#include <stdio.h> /* Print the resolved method result. */
#include <string.h> /* Compare the complete requested name. */
#define CHECK(call) do { if ((call) != FVM_OK) goto cleanup; } while (0) /* Keep error checks active in Release. */
static FVM_Result sum(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args, FVM_Value *out, void *userdata) /* Original C implementation. */
{ /* Begin the original callback. */
    (void)c; /* No additional VM service is needed for two Numbers. */
    (void)self; /* The sum does not inspect its method receiver. */
    (void)userdata; /* This sample has no host-owned arithmetic state. */
    if (n != FVM_RESOLVE_EXAMPLE_SUM_ARGUMENTS || args[0].type != FVM_VALUE_NUMBER || args[1].type != FVM_VALUE_NUMBER) return FVM_INVALID_ARGUMENT; /* Reject a different signature. */
    fvm_value_number(out, args[0].number + args[1].number); /* Produce the result from the actual arguments. */
    return FVM_OK; /* Complete the original function. */
} /* End the original callback. */
static FVM_Result resolve(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args, FVM_Value *out, void *userdata) /* Locate a Function without executing it yet. */
{ /* Begin the resolver callback. */
    (void)c; /* The retained Function already belongs to this context. */
    (void)self; /* The VM will bind the returned Function to the original receiver. */
    fvm_value_undefined(out); /* Unknown names remain unresolved. */
    if (n == FVM_RESOLVE_EXAMPLE_NAME_ARGUMENTS && args[0].type == FVM_VALUE_STRING && fvm_value_string_size(args) == FVM_RESOLVE_EXAMPLE_NAME_BYTES && !memcmp(args[0].string, FVM_RESOLVE_EXAMPLE_METHOD, FVM_RESOLVE_EXAMPLE_NAME_BYTES)) /* Match all counted bytes. */
    { /* Return the captured native Function for sum. */
        out->type = FVM_VALUE_OBJECT; /* Publish an object-valued method result. */
        out->object = (FVM_Object *)userdata; /* Use the original Function, retained by the resolver capture. */
    } /* Finish name selection. */
    return FVM_OK; /* Lookup itself succeeded even for an unknown name. */
} /* End the resolver callback. */
int main(void) /* Run the standalone C89 tutorial. */
{ /* Begin process-local setup. */
    static const unsigned char swf6[] = {'F','W','S',6,16,0,0,0,8,0,0,12,1,0,0,0}; /* Minimal SWF6 fixture enables legacy AS semantics. */
    FVM_Config config; /* Hold runtime configuration. */
    FVM_Runtime *runtime = 0; /* Own the VM runtime until cleanup. */
    FVM_Context *context = 0; /* Own the loaded VM heap. */
    FVM_Object *receiver = 0, *addition = 0, *resolver = 0; /* Track the objects built below. */
    FVM_Value value, args[FVM_RESOLVE_EXAMPLE_SUM_ARGUMENTS], result; /* Hold the resolver binding, inputs and output. */
    int stored = 1, exit_code = 1; /* Treat any incomplete setup or wrong result as failure. */
    CHECK(fvm_config_init(&config)); /* Initialize the public configuration ABI. */
    CHECK(fvm_runtime_create(&config, &runtime)); /* Create the runtime. */
    CHECK(fvm_context_create(runtime, &context)); /* Create an independent context. */
    CHECK(fvm_load_memory(context, swf6, sizeof(swf6))); /* Select supported SWF6 behavior. */
    CHECK(fvm_standard_install(context)); /* Install Object and Function. */
    CHECK(fvm_construct(context, "Object", 0, 0, &receiver)); /* Allocate the method receiver. */
    CHECK(fvm_gc_pin(context, receiver)); /* Keep the receiver across later collections. */
    CHECK(fvm_function_create_native(context, sum, 0, 0, &addition)); /* Wrap the original C callback. */
    CHECK(fvm_gc_pin(context, addition)); /* Protect the Function during resolver construction. */
    CHECK(fvm_function_create_native(context, resolve, addition, addition, &resolver)); /* Retain addition as a managed capture as well as userdata. */
    CHECK(fvm_gc_pin(context, resolver)); /* Protect the resolver until publication. */
    fvm_value_undefined(&value); /* Initialize every value field before publishing an object. */
    value.type = FVM_VALUE_OBJECT; /* Store a Function object as the hook. */
    value.object = resolver; /* Select the resolver Function. */
    CHECK(fvm_object_set(context, receiver, "__resolve", &value)); /* Install the raw resolver property. */
    CHECK(fvm_gc_unpin(context, resolver)); /* The receiver now retains its resolver. */
    CHECK(fvm_gc_unpin(context, addition)); /* The resolver capture now retains the original sum. */
    CHECK(fvm_gc_collect(context, 0)); /* Demonstrate that the published capture graph survives GC. */
    CHECK(fvm_object_has_own(context, receiver, FVM_RESOLVE_EXAMPLE_METHOD, &stored)); /* Inspect presence without running __resolve. */
    if (stored) goto cleanup; /* Require the method to be absent before lookup. */
    fvm_value_number(args, 19); /* Prepare the first Number. */
    fvm_value_number(args + 1, 23); /* Prepare the second Number. */
    CHECK(fvm_object_call(context, receiver, FVM_RESOLVE_EXAMPLE_METHOD, FVM_RESOLVE_EXAMPLE_SUM_ARGUMENTS, args, &result)); /* Resolve then execute the original sum with the prepared arguments. */
    if (result.type != FVM_VALUE_NUMBER || result.number != 42) goto cleanup; /* Check observable behavior. */
    CHECK(fvm_object_has_own(context, receiver, FVM_RESOLVE_EXAMPLE_METHOD, &stored)); /* Check that lookup did not cache a property. */
    if (stored) goto cleanup; /* The resolver returns a Function without storing it. */
    printf("sum = %.0f\n", result.number); /* Print sum = 42 on success. */
    exit_code = 0; /* Mark the completed tutorial as successful. */
cleanup: /* Release all partially or fully initialized state. */
    if (context) fvm_context_destroy(context); /* Reclaim the pinned receiver and its entire graph. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release runtime-owned services. */
    return exit_code; /* Report success or failure to CTest/the shell. */
} /* End the tutorial. */
