/* FlashVM C89 tutorial: original C functions across processes, VM threads and LAN.
 * Modes: serve [BIND_HOST ADVERTISED_HOST PORT], call URI, or threads.
 * Оригинальный callback сохраняет свою VM и userdata; получатель работает с интерфейсом. */
/* Declare the shared authority and exclusive source-context handoff APIs. */
#include "flashvm/capability.h"
/* Declare independent worker runtimes on native threads. */
#include "flashvm/worker.h"
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
/* Parse a configurable TCP port without narrowing or accepting trailing text. */
#include <stdlib.h>
/* Detect numeric overflow during port parsing. */
#include <errno.h>
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
/* Invoke the same interface checks from a process client or a worker-owned VM. */
static void call_original(FVM_Context *context, const char *uri)
/* Begin recipient work using only its own heap and a copied URI. */
{
    /* Retain a callable facade, never an original function pointer. */
    FVM_Object *function = 0;
    /* Keep call input and returned values local to this recipient. */
    FVM_Value argument, result;
    /* Allocate only a callable facade; the original C function remains in the other process. */
    assert(fvm_broker_import(context, uri, &function) == FVM_OK);
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
/* Finish recipient work; context destruction later releases the facade root. */
}
/* A public Worker creates its own independent runtime/context on another native thread. */
static FVM_Result client_thread(FVM_Context *context, const FVM_Value *input, FVM_Value *out, void *userdata)
/* Begin execution on the worker's native thread. */
{
    /* Query the current worker without inspecting the source VM heap. */
    FVM_Worker *current = 0;
    /* Confirm that the recipient is a worker-owned VM, not the source host context. */
    assert(fvm_worker_current(context, &current) == FVM_OK && current != 0);
    /* No VM object or callback userdata is shared with the source runtime. */
    (void)userdata;
    /* Worker transfer copies this primitive URI string into the recipient. */
    assert(input->type == FVM_VALUE_STRING);
    /* Use actual TCP calls even though both runtimes inhabit one process. */
    call_original(context, input->string);
    /* Return a primitive result that remains valid after the worker is joined. */
    fvm_value_number(out, FVM_FUNCTION_EXAMPLE_BIAS + 2);
    /* Report successful completion of all four original function calls. */
    return FVM_OK;
/* End worker execution. */
}
/* Finish optional callback declarations. */
#endif
/* Select two-process, two-runtime-thread or configurable LAN execution. */
int main(int argc, char **argv)
/* Begin the C89 entry point. */
{
/* Compile transport only where platform sockets and native workers are available. */
#ifdef OS_PORTS
    /* Configure each independently created runtime with the same feature defaults. */
    FVM_Config config;
    /* Own the source runtime, or this standalone client's runtime. */
    FVM_Runtime *runtime = 0;
    /* Keep this local context independent of the context owned by a Worker. */
    FVM_Context *context = 0;
    /* The source owns exactly one TCP listener for all its exported functions. */
    FVM_ResourceBroker *broker = 0;
    /* Store the original C function before transferring source ownership. */
    FVM_Object *function = 0;
    /* Preserve original userdata until serving has ended. */
    Server state;
    /* A Worker owns a second runtime and executes the client on a native thread. */
    FVM_Worker *worker = 0;
    /* Pass only a copied primitive URI to the Worker; return a number. */
    FVM_Value input, result;
    /* Dispose of this malloc-owned URI after clients finish. */
    char *uri = 0;
    /* Default to local-only access unless the host explicitly supplies LAN addresses. */
    const char *bind_host = "127.0.0.1", *advertised_host = "127.0.0.1";
    /* Port zero asks the OS to choose the listener's port. */
    unsigned long port = 0;
    /* Check that numeric parsing consumes the complete optional port argument. */
    char *port_end = 0;
    /* Wait for a supervisor or terminal line without touching the source heap. */
    char shutdown_line[FVM_FUNCTION_EXAMPLE_CONTROL_SIZE];
    /* Recognize each mode only with its documented number of arguments. */
    if (argc < 2 || !((!strcmp(argv[1], "serve") && (argc == 2 || argc == 5)) || (!strcmp(argv[1], "call") && argc == 3) || (!strcmp(argv[1], "threads") && argc == 2)))
    /* Begin usage and malformed-mode handling. */
    {
        /* Show the source and client process commands. */
        puts("Use: function_rpc serve [BIND_HOST ADVERTISED_HOST PORT]; function_rpc call URI");
        /* Show the in-process, independent-runtime option. */
        puts("Or: function_rpc threads");
        /* Automatic example execution uses no arguments; malformed commands report an error. */
        return argc == 1 ? 0 : FVM_FUNCTION_EXAMPLE_USAGE_ERROR;
    /* End usage-only execution. */
    }
    /* Parse the LAN listener configuration before creating a VM or broker. */
    if (argc == 5)
    /* Begin explicit-address handling. */
    {
        /* Bind all or one selected local IPv4 interface according to the caller's argument. */
        bind_host = argv[2];
        /* Advertise an IPv4 address or DNS name reachable from the other computer. */
        advertised_host = argv[3];
        /* Reset overflow state before parsing the unsigned decimal port. */
        errno = 0;
        /* Delay narrowing to unsigned short until the complete range has been checked. */
        port = strtoul(argv[4], &port_end, 10);
        /* Reject empty, negative, out-of-range and partially parsed values. */
        if (argv[4][0] < '0' || argv[4][0] > '9' || *port_end || errno == ERANGE || port > FVM_FUNCTION_EXAMPLE_PORT_MAX)
        /* Begin invalid-port reporting. */
        {
            /* Explain the accepted listener-port range. */
            fputs("PORT must be a decimal integer from 0 through 65535\n", stderr);
            /* No runtime, listener or function target has been published. */
            return FVM_FUNCTION_EXAMPLE_USAGE_ERROR;
        /* End invalid-port reporting. */
        }
    /* End explicit-address handling. */
    }
    /* Initialize the C89 public configuration. */
    assert(fvm_config_init(&config) == FVM_OK);
    /* Create this process's main runtime. */
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    /* Create a context belonging only to that runtime. */
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    /* A standalone client needs no listener and owns no source userdata. */
    if (!strcmp(argv[1], "call"))
        /* Invoke the original function on another process or computer via its URI. */
        call_original(context, argv[2]);
    /* Both source modes share identical original-function and broker setup. */
    else
    /* Begin original source setup. */
    {
        /* Require the callback to execute in this original context. */
        state.context = context;
        /* Read this count only before serving starts and after it has ended. */
        state.calls = 0;
        /* Create the single loopback or LAN listening port after validating its numeric range. */
        assert(fvm_broker_start(bind_host, advertised_host, (unsigned short)port, &broker) == FVM_OK);
        /* Retain the authority from the original source context. */
        assert(fvm_context_set_broker(context, broker) == FVM_OK);
        /* Select live function interfaces without requiring a native callback codec. */
        assert(fvm_context_function_transfer(context, 1) == FVM_OK);
        /* Original native userdata stays in this process. */
        assert(fvm_function_create_native(context, original, &state, 0, &function) == FVM_OK);
        /* [GC:] Pin the original function while publishing its random target URI. */
        assert(fvm_broker_export(context, function, &uri) == FVM_OK);
        /* Transfer the source heap exclusively to the broker's execution thread. */
        assert(fvm_context_function_serve(context, 1) == FVM_OK);
        /* Select another VM/runtime on a different thread in this same process. */
        if (!strcmp(argv[1], "threads"))
        /* Begin thread-separated VM communication. */
        {
            /* Primitive string transfer copies the URI; no VM object crosses the thread boundary. */
            fvm_value_string(&input, uri);
            /* Start an independent recipient runtime/context through the public Worker API. */
            assert(fvm_worker_run(&config, client_thread, &input, 0, &worker) == FVM_OK);
            /* Join only the recipient; the source heap remains owned by its broker thread. */
            assert(fvm_worker_join(worker, &result) == FVM_OK);
            /* Verify a primitive result from the recipient worker. */
            assert(result.type == FVM_VALUE_NUMBER && result.number == FVM_FUNCTION_EXAMPLE_BIAS + 2);
            /* Release the joined worker and its independent runtime. */
            fvm_worker_destroy(worker);
            /* State explicitly that both active VM heaps communicated over TCP. */
            puts("two independent runtime threads via TCP passed");
        /* End thread-separated communication. */
        }
        /* External clients obtain the URI through stdout or a terminal. */
        else
        /* Begin process/LAN coordination. */
        {
            /* Publish an address reachable from the selected recipient computer. */
            puts(uri);
            /* Flush before waiting so an external supervisor can start the client. */
            fflush(stdout);
            /* Wait on host stdin without accessing the handed-off original VM heap. */
            assert(fgets(shutdown_line, sizeof(shutdown_line), stdin) != 0);
        /* End external-client coordination. */
        }
        /* Reclaim the source context after active invocation and result encoding have completed. */
        assert(fvm_context_function_serve(context, 0) == FVM_OK);
        /* Prove the original userdata, rather than a client copy, observed all attempts. */
        assert(state.calls == FVM_FUNCTION_EXAMPLE_REQUESTS);
        /* Publish a stable completion marker for automated integration checks. */
        puts("original calls=4");
        /* Free only the host-owned URI buffer; context detachment revokes the target later. */
        fvm_serialized_free(uri);
    /* End source-mode work. */
    }
    /* [GC:] Release the local heap and detach its function roots after serving has stopped. */
    fvm_context_destroy(context);
    /* Stop/join the authority; null is harmless for the standalone client. */
    fvm_broker_stop(broker);
    /* Release the retained broker handle after its service thread has ended. */
    fvm_broker_release(broker);
    /* Release the main runtime after both VM execution threads have finished. */
    fvm_runtime_destroy(runtime);
/* Keep a no-op usage example in configurations without platform transport. */
#else
    /* Mark the command count unused in a disabled build. */
    (void)argc;
    /* Mark command arguments unused in a disabled build. */
    (void)argv;
/* End transport configuration selection. */
#endif
    /* All requested communication and cleanup completed successfully. */
    return 0;
/* End the C89 entry point. */
}
