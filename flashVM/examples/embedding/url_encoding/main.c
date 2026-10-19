/* FlashVM C89 tutorial: percent-encode counted UTF-16, including NUL, and verify its roundtrip. */
#include "flashvm/flashvm.h" /* Runtime/context acquisition and status codes. */
#include "flashvm/standard.h" /* Install the escape/unescape global functions. */
#include "flashvm/value.h" /* Counted UTF-16 input/output and synchronous global calls. */
#include <stdio.h> /* Print ASCII encoding and a verified unit count. */
#include <string.h> /* Compare complete UTF-16 buffers, including embedded zero. */
/* Configure an owning-thread VM and keep all borrowed input valid until its synchronous call ends. */
int main(void) /* Build with a strict C89 compiler; no network, SWF or display is required. */
{ /* Initialize cleanup-safe ownership before allocating resources. */
    FVM_Config config; /* Caller-owned configuration copied by runtime creation. */
    FVM_Runtime *runtime = 0; /* Outlive the context and every returned managed value. */
    FVM_Context *context = 0; /* Own all result strings until context destruction. */
    FVM_Value input, encoded, decoded; /* Local values are used synchronously without intervening collection. */
    const unsigned short units[] = {0x4e2d, 0x6587, ' ', '+', ' ', '@', 0, 'Z'}; /* Eight UTF-16 units, including NUL. */
    unsigned short copy[sizeof(units) / sizeof(units[0])]; /* Exact local capacity for the decoded spelling. */
    size_t count = 0; /* Receive the counted length independently of the embedded zero. */
    FVM_Result status = fvm_config_init(&config); /* Initialize ABI defaults before acquiring the runtime. */
    if (status == FVM_OK) /* Acquire the outer owner only after valid configuration. */
        status = fvm_runtime_create(&config, &runtime); /* No worker or device is created by this example. */
    if (status == FVM_OK) /* Acquire an independent managed heap on this thread. */
        status = fvm_context_create(runtime, &context); /* The context owns the standard bindings. */
    if (status == FVM_OK) /* Standard installation is transactional on conflicts/allocation failure. */
        status = fvm_standard_install(context); /* Publish both URL global functions and their dependencies. */
    if (status == FVM_OK) /* Preserve all code units instead of constructing a terminated C string. */
        status = fvm_value_string_utf16(context, units, sizeof(units) / sizeof(units[0]), &input); /* Context owns converted storage. */
    if (status == FVM_OK) /* Convert bytes using the AS1/AS2 alphanumeric-only escape contract. */
        status = fvm_call(context, "escape", 1, &input, &encoded); /* Additional arguments are unnecessary; no URL is opened. */
    if (status == FVM_OK) /* The encoded spelling consists entirely of non-NUL ASCII. */
        printf("encoded: %s\n", encoded.string); /* Expected suffix: %20%2B%20%40%00Z. */
    if (status == FVM_OK) /* Decode the complete counted result once, not recursively. */
        status = fvm_call(context, "unescape", 1, &encoded, &decoded); /* Percent-encoded plus remains plus; literal plus would become space. */
    if (status == FVM_OK) /* Copy back to caller-owned storage before destroying the VM. */
        status = fvm_value_get_utf16(&decoded, copy, sizeof(copy) / sizeof(copy[0]), &count); /* NUL counts as an ordinary unit. */
    if (status == FVM_OK && (count != sizeof(units) / sizeof(units[0]) || memcmp(units, copy, sizeof(units)) != 0)) /* Check the entire original spelling. */
        status = FVM_ERROR; /* Report an unexpected roundtrip through normal cleanup. */
    if (status == FVM_OK) /* Print only after complete content and length verification. */
        printf("roundtrip units: %lu (including NUL)\n", (unsigned long)count); /* Expected: eight units. */
    else /* Preserve installation, coercion, unsupported-profile or memory failures. */
        fprintf(stderr, "URL example failed: %d\n", (int)status); /* Do not silently use an incomplete result. */
    if (context) /* End access to every managed string before its owning heap is released. */
        fvm_context_destroy(context); /* [GC:] Release bindings, strings and partial preparation on all paths. */
    if (runtime) /* The runtime must remain alive through context cleanup. */
        fvm_runtime_destroy(runtime); /* Release the outer owner last. */
    return status == FVM_OK ? 0 : 1; /* Expose verified success or an unexpected error to the caller. */
} /* No managed pointer escapes this example. */
