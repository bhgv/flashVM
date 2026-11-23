/* Copyright (c) 2026 bhgv
 * EN: Serve real AVM1 initializers for the scoped channel integration client.
 * RU: Обслуживание настоящих инициализаторов AVM1 для клиента каналов.
 */
#include "channel-test.h"
#include "flashvm/function.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <errno.h>
#include <stdio.h>
#include <sys/random.h>

#define PORT_SIGNATURE UINT32_C(0x57a0e232)       /* fn(string): chan of string. */
#define RENDEZVOUS_SIGNATURE UINT32_C(0x421b2140) /* fn(): chan of int. */
#define BAD_SEED_SIGNATURE UINT32_C(0x1ecacb4d)   /* fn(string): chan of int. */
#define TEST_CHANNEL_LEASES 3U /* Two FIFOs, plus one optional rendezvous. */
#define TEST_MUX_LIMIT 65536U /* Shared physical carrier frame limit. */
#define TEST_STYX_MSIZE 4096U /* Negotiated message size. */
#define TEST_STYX_WAIT 3000U /* Native mount timeout in milliseconds. */
#define TEST_STYX_TAGS 8U /* Concurrent reverse request tags. */

#define ACTION_END 0x00          /* End the AVM1 initializer. */
#define ACTION_GET_VARIABLE 0x1c /* Resolve its seed formal. */
#define ACTION_RETURN 0x3e       /* Return the seed to the exporter. */
#define ACTION_PUSH 0x96         /* Push a typed Flash literal. */
#define PUSH_STRING 0x00         /* Terminated name operand. */
#define PUSH_UNDEFINED 0x03      /* Empty initializer result. */


/* EN: Fill the requested token bytes from the test host CSPRNG.
 * RU: Заполнить токен случайными байтами из CSPRNG тестового хоста. */
int test_channel_entropy(void *unused, void *bytes, size_t length)
{
    unsigned char *out = bytes;
    (void)unused;
    while (length)
    {
        ssize_t got = getrandom(out, length, 0);
        if (got < 0 && errno == EINTR)
        {
            continue;
        }
        if (got <= 0)
        {
            return 0;
        }
        out += got;
        length -= (size_t)got;
    }
    return 1;
}


/* [GC:] EN: Bind an actual AVM1 initializer in the dispatcher-owned context.
 * RU: Привязать настоящий инициализатор AVM1 в контексте диспетчера. */
static int define(FVM_Context *context, const char *name, const unsigned char *actions, size_t size, unsigned count)
{
    const char *parameters[] = {"v"};
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


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_serve(FVM_Context *context, int fd)
{
    static const unsigned char echo[] = {ACTION_PUSH, 3, 0, PUSH_STRING, 'v', 0,
        ACTION_GET_VARIABLE, ACTION_RETURN, ACTION_END};
    static const unsigned char empty[] = {ACTION_PUSH, 1, 0, PUSH_UNDEFINED, ACTION_RETURN, ACTION_END};
    FVM_StyxRpcChannelExport bindings[] = {
        {context, "seed", {"(s)", "(c[s])"}, 1},
        {context, "empty", {"()", "(c[i])"}, 0},
        {context, "seed", {"(s)", "(c[i])"}, 1}
    };
    StyxRpcChannelExport exports[sizeof(bindings) / sizeof(*bindings)];
    StyxRpcModuleChannel leases[TEST_CHANNEL_LEASES];
    StyxRpcModuleChannels registry;
    StyxRpcChannelModule module;
    if (!define(context, "seed", echo, sizeof(echo), 1) ||
        !define(context, "empty", empty, sizeof(empty), 0) ||
        fvm_styxrpc_channel_export(bindings, "port", PORT_SIGNATURE, exports) != FVM_OK ||
        fvm_styxrpc_channel_export(bindings + 1, "rendezvous", RENDEZVOUS_SIGNATURE, exports + 1) != FVM_OK ||
        fvm_styxrpc_channel_export(bindings + 2, "badseed", BAD_SEED_SIGNATURE, exports + 2) != FVM_OK ||
        !styxrpc_module_channels_init(&registry, leases, TEST_CHANNEL_LEASES, test_channel_entropy, NULL) ||
        styxrpc_channel_module_init(&module, "flash-channels.dis", exports,
            sizeof(exports) / sizeof(*exports), &registry) != STYXRPC_OK)
    {
        fputs("FlashVM channel export setup failed\n", stderr);
        return 0;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), TEST_MUX_LIMIT);
    if (!mux)
    {
        return 0;
    }
    StyxClient *reverse = styxclientmountphy(phybidirchannel(mux, 1), TEST_STYX_MSIZE,
        "flash-channels", "", TEST_STYX_WAIT, TEST_STYX_TAGS);
    StyxRpcServer *server = reverse ? styxrpc_server_create(phybidirchannel(mux, 0),
        &styxrpc_channel_module_tree, &module) : NULL;
    while (styxrpc_server_step(server))
    {
    }
    int ok = server != NULL;
    styxrpc_server_destroy(server);
    styxclientshutdown(reverse);
    styxclientfree(reverse);
    phybidirfree(mux);
    return ok;
}
