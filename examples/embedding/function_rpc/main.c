/* FlashVM C89 tutorial: a codec-free C function served to a separate process.
 * Run "serve" and pass its first output line to another process running "call URI".
 * Учебный C callback остаётся в серверном процессе и использует его userdata. */
/* Declare the shared authority and exclusive source-context handoff APIs. */
#include "flashvm/capability.h"
/* Declare native function construction and invocation. */
#include "flashvm/function.h"
/* Declare counted thrown-value access and clearing. */
#include "flashvm/exception.h"
/* Declare exported URI disposal. */
#include "flashvm/serialization.h"
/* Declare explicit managed roots and collection. */
#include "flashvm/gc.h"
/* Keep tutorial assertions active in Release as well as Debug. */
#include <assert.h>
/* Supply URI publication and stdin shutdown synchronization. */
#include <stdio.h>
/* Supply mode selection and counted exception comparison. */
#include <string.h>
/* Name tutorial payload/count limits separately from the wire protocol. */
#include "function_rpc_example.h"
/* Socket/thread APIs are optional; disabled builds still run the usage branch. */
#ifdef OS_PORTS
/* Host-owned callback state must survive until source quiescence. */
typedef struct Server
/* Start the userdata representation. */
{
    /* Original source context: callback execution must never migrate to the client heap. */
    FVM_Context *context;
    /* Count completed attempts on the authority thread; read only after reclaim. */
    unsigned int calls;
/* End the userdata representation. */
} Server;
/* Execute the original C callback on the authority; no serialization codec is registered. */
static FVM_Result original(FVM_Context *c, FVM_Object *receiver, unsigned int argc, const FVM_Value *argv, FVM_Value *out, void *userdata)
/* Begin original function execution. */
{
    /* Recover the server-owned userdata without sending this pointer over the network. */
    Server *state = (Server *)userdata;
    /* This tutorial does not inspect the default receiver. */
    (void)receiver;
    /* Validate original context identity and the single numeric parameter. */
    assert(c == state->context && argc == 1 && argv[0].type == FVM_VALUE_NUMBER);
    /* Record an attempt in original state before choosing a result. */
    ++state->calls;
    /* A negative sample requests a counted exception containing an embedded NUL. */
    if (argv[0].number == -1)
    /* Begin exception production. */
    {
        /* Preserve all three bytes rather than treating the value as a C string. */
        assert(fvm_value_string_bytes(out, "C\0E", FVM_FUNCTION_EXAMPLE_THROW_SIZE) == FVM_OK);
        /* Publish the thrown value through the ordinary VM exception API. */
        return fvm_exception_raise(c, out);
    /* End exception production. */
    }
    /* The next sample demonstrates a plain unsupported status. */
    if (argv[0].number == -2)
        /* Return the original status to the remote interface. */
        return FVM_UNSUPPORTED;
    /* The last sample demonstrates an ordinary error status. */
    if (argv[0].number == -3)
        /* Do not manufacture an exception when the original callback did not throw. */
        return FVM_ERROR;
    /* Calculate the ordinary result using server-side configuration. */
    fvm_value_number(out, FVM_FUNCTION_EXAMPLE_BIAS + argv[0].number);
    /* Tell the caller that the numeric result is available. */
    return FVM_OK;
/* End the original callback. */
}
/* Finish optional callback declarations. */
#endif
/* Choose the standalone server/client mode, or show nonblocking usage with no arguments. */
int main(int argc, char **argv)
/* Begin the C89 entry point. */
{
/* Compile executable transport only where the platform provides sockets and threads. */
#ifdef OS_PORTS
    /* Configure runtime defaults before creating any contexts. */
    FVM_Config config;
    /* Own a runtime until its single local context has been destroyed. */
    FVM_Runtime *runtime = 0;
    /* Each process owns a distinct VM heap. */
    FVM_Context *context = 0;
    /* The server owns a broker; the client imports its URI without starting one. */
    FVM_ResourceBroker *broker = 0;
    /* Store the local original function or imported callable facade. */
    FVM_Object *function = 0;
    /* Keep the server userdata on the stack until all authority calls finish. */
    Server state;
    /* Store the malloc-owned exported URI only in server mode. */
    char *uri = 0;
    /* Receive a shutdown line without accessing the handed-off VM heap. */
    char shutdown_line[FVM_FUNCTION_EXAMPLE_CONTROL_SIZE];
    /* Reuse a single numeric input and result in the client. */
    FVM_Value argument, result;
    /* Missing/invalid modes must return immediately during automatic example runs. */
    if (argc < 2 || (strcmp(argv[1], "serve") && (strcmp(argv[1], "call") || argc != 3)))
    /* Begin usage-only execution. */
    {
        /* Explain the two-process invocation convention. */
        puts("Use: function_rpc serve | function_rpc call URI (in separate processes)");
        /* No VM or broker was created, so there is nothing to release. */
        return 0;
    /* End usage-only execution. */
    }
    /* Initialize the ABI-compatible configuration. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Create this process's runtime. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    /* Create this process's exclusively owned context. */
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    /* Select the source process that owns and executes the C function. */
    if (!strcmp(argv[1], "serve"))
    /* Begin source initialization. */
    {
        /* Point userdata at the original heap; the proxy must never replace it. */
        state.context = context;
        /* Start the server-side count before publishing the function. */
        state.calls = 0;
        /* Allocate one loopback listener; port zero selects an available TCP port. */
        assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
        /* Retain the broker from the source context. */
        assert(fvm_context_set_broker(context, broker) == FVM_OK);
        /* Explicitly export live interfaces rather than cloned function bodies. */
        assert(fvm_context_function_transfer(context, 1) == FVM_OK);
        /* Create a C function retaining host-owned userdata; no managed capture is needed. */
        assert(fvm_function_create_native(context, original, &state, 0, &function) == FVM_OK);
        /* [GC:] Publishing the target pins the original function until source detachment. */
        assert(fvm_broker_export(context, function, &uri) == FVM_OK);
        /* Transfer the idle context exclusively to the authority thread. */
        assert(fvm_context_function_serve(context, 1) == FVM_OK);
        /* Publish only the URI, never a callback address or userdata pointer. */
        puts(uri);
        /* Make the URI visible to a supervising process before waiting for shutdown. */
        fflush(stdout);
        /* The host waits on stdin and does not inspect source state while serving. */
        assert(fgets(shutdown_line, sizeof(shutdown_line), stdin) != 0);
        /* Reclaim the heap after active invocation and result encoding complete. */
        assert(fvm_context_function_serve(context, 0) == FVM_OK);
        /* Original userdata confirms that all four remote attempts reached this process. */
        assert(state.calls == FVM_FUNCTION_EXAMPLE_REQUESTS);
        /* Return a compact completion marker to the supervisor. */
        puts("original calls=4");
        /* Release the host-owned URI copy; this does not revoke the pinned target. */
        fvm_serialized_free(uri);
    /* End source-mode work. */
    }
    /* Otherwise run the client against the provided bearer capability. */
    else
    /* Begin recipient-mode work. */
    {
        /* Allocate only a callable facade; the original C function remains in the other process. */
        assert(fvm_broker_import(context, argv[2], &function) == FVM_OK);
        /* [GC:] Retain the interface through explicit client-side collection. */
        assert(fvm_gc_pin(context, function) == FVM_OK);
        /* [GC:] Collection must preserve the proxy and its URI capture. */
        assert(fvm_gc_collect(context, 0) == FVM_OK);
        /* Ask the original function to add its server-side bias. */
        fvm_value_number(&argument, 2);
        /* Confirm the remote ordinary return value. */
        assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_OK);
        /* Read a numeric response copied from the original process. */
        assert(result.number == FVM_FUNCTION_EXAMPLE_BIAS + 2);
        /* Request the embedded-NUL exception. */
        fvm_value_number(&argument, -1);
        /* Verify exception status and the complete counted thrown value. */
        assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_EXCEPTION);
        /* Fetch the pending thrown value from the recipient context. */
        assert(fvm_exception_get(context, &result) == FVM_OK);
        /* Counted length includes the byte following the embedded NUL. */
        assert(fvm_value_string_size(&result) == FVM_FUNCTION_EXAMPLE_THROW_SIZE);
        /* Compare all payload bytes rather than stopping at the NUL. */
        assert(!memcmp(result.string, "C\0E", FVM_FUNCTION_EXAMPLE_THROW_SIZE));
        /* Clear the local pending exception before issuing the next call. */
        assert(fvm_exception_clear(context) == FVM_OK);
        /* Request a plain unsupported result. */
        fvm_value_number(&argument, -2);
        /* Preserve the exact original status. */
        assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_UNSUPPORTED);
        /* Request an ordinary error result. */
        fvm_value_number(&argument, -3);
        /* Distinguish an error return from a thrown exception. */
        assert(fvm_function_invoke(context, function, 0, 1, &argument, &result) == FVM_ERROR);
        /* Report completion without accessing any server-owned memory. */
        puts("remote result=42; counted exception; unsupported; error");
    /* End recipient-mode work. */
    }
    /* [GC:] Detach exported roots or release the facade before freeing the heap. */
    fvm_context_destroy(context);
    /* Stop and join the authority; null is harmless in client mode. */
    fvm_broker_stop(broker);
    /* Release the server's retained broker handle, if any. */
    fvm_broker_release(broker);
    /* Release the runtime after its context and server thread are gone. */
    fvm_runtime_destroy(runtime);
/* In disabled configurations no transport or runtime allocation takes place. */
#else
    /* Mark argc unused in the stub example. */
    (void)argc;
    /* Mark argv unused in the stub example. */
    (void)argv;
/* End configuration selection. */
#endif
    /* Indicate successful completion of either tutorial mode or usage. */
    return 0;
/* End the entry point. */
}
