#include "flashvm/exception.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "network_port.h"
#include "thread_port.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef OS_PORTS
struct Server
{
    FVM_PortSocket *listener;
};
static int receive(FVM_PortSocket *socket, void *data, size_t length)
{
    size_t offset = 0;
    while (offset < length)
    {
        long n = fvm_port_socket_recv(socket, (char *)data + offset, length - offset);
        if (n <= 0)
            return 0;
        offset += (size_t)n;
    }
    return 1;
}
static void serve(void *arg)
{
    struct Server *server = (struct Server *)arg;
    FVM_PortSocket *socket;
    unsigned char request[112];
    unsigned char response[9] = {0, 0, 0, 0, 0, 0, 0, 1, 0};
    int i;
    for (i = 0; i < 2; ++i)
    {
        socket = fvm_port_tcp_accept(server->listener, 5000);
        assert(socket);
        assert(fvm_port_socket_timeout(socket, 2000));
        assert(receive(socket, request, sizeof(request)));
        if (!i)
            assert(fvm_port_socket_send(socket, response, sizeof(response)) == sizeof(response));
        /* The second request loses its response after a successful connect. */
        fvm_port_socket_close(socket);
    }
}
#endif
int main(void)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *proxy = 0;
    FVM_Value value;
    FVM_ExceptionInfo info;
    FVM_ResourceBroker *broker = 0;
    FVM_Mutex *mutex = 0;
    FVM_Object *lock = 0;
    char *exported = 0, *session;
    struct Server server;
    FVM_PortThread *thread;
    unsigned short port = 0;
    char uri[256];
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_extensions_install(context) == FVM_OK);
    server.listener = fvm_port_tcp_listen("127.0.0.1", 0, &port);
    assert(server.listener && port);
    sprintf(
        uri,
        "fvm://127.0.0.1:%u/00000000000000000000000000000000/11111111111111111111111111111111/8",
        (unsigned int)port);
    thread = fvm_port_thread_start(serve, &server);
    assert(thread);
    assert(fvm_broker_import(context, uri, &proxy) == FVM_OK);
    assert(fvm_object_call(context, proxy, "touch", 0, 0, &value) == FVM_EXCEPTION);
    assert(fvm_exception_info(context, &info) == FVM_OK);
    assert(info.code == FVM_RESOURCE_DISCONNECTED);
    assert(info.flags & FVM_EXCEPTION_NETWORK);
    assert(!strcmp(info.address, uri));
    assert(fvm_exception_clear(context) == FVM_OK);
    fvm_port_thread_join(thread);
    fvm_port_socket_close(server.listener);
    assert(fvm_object_call(context, proxy, "touch", 0, 0, &value) == FVM_EXCEPTION);
    assert(fvm_exception_info(context, &info) == FVM_OK && info.code == FVM_RESOURCE_UNREACHABLE);
    assert(fvm_exception_clear(context) == FVM_OK);
    /* A reachable authority distinguishes stale session from connection loss. */
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(context, broker) == FVM_OK);
    assert(fvm_mutex_create(context, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(context, mutex, &lock) == FVM_OK);
    assert(fvm_broker_export(context, lock, &exported) == FVM_OK);
    session = strchr(exported + 6, '/') + 1;
    *session = *session == 'a' ? 'b' : 'a';
    assert(fvm_broker_import(context, exported, &proxy) == FVM_ERROR);
    assert(fvm_exception_info(context, &info) == FVM_OK && info.code == FVM_RESOURCE_STALE);
    assert(fvm_exception_get(context, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
    assert(fvm_exception_clear(context) == FVM_OK);
    fvm_serialized_free(exported);
    fvm_mutex_release(mutex);
    fvm_context_destroy(context);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
#endif
    return 0;
}
