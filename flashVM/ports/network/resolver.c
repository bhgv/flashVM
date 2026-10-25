/* Native resolver shared by FlashVM and hosted Dis Srv. DNS is synchronous. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L /* Expose standard POSIX resolver declarations. */
#endif
#include "resolver.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


/* Acquire one temporary Winsock reference on Windows; other platforms need none.
 * No parameters. Return nonzero on success; every successful call needs finish. */
static int resolver_start(void)
{
#ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return 1;
#endif
}


/* Release exactly the temporary Winsock reference acquired by resolver_start. */
static void resolver_finish(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}


/* Append a distinct bounded name to owned result storage; text is borrowed.
 * Return nothing; duplicate/full answers are ignored without changing earlier ones. */
static void resolver_add(FVM_ResolverNames *result, const char *text)
{
    size_t index;
    size_t length = strlen(text);
    if (length >= FVM_RESOLVER_TEXT || result->count >= FVM_RESOLVER_NAMES)
    {
        return;
    }
    for (index = 0; index < result->count; index++)
    {
        if (strcmp(result->names[index], text) == 0)
        {
            return;
        }
    }
    memcpy(result->names[result->count], text, length + 1);
    result->count++;
}


/* Resolve borrowed text with numeric output (reverse=0) or required host output.
 * out is caller-owned, published only after success; all addrinfo state is released. */
static FVM_Result resolver_names(const char *text, int reverse, FVM_ResolverNames *out)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *entry;
    FVM_ResolverNames result;
    char name[FVM_RESOLVER_TEXT];
    int status;
    if (!text || !*text || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!resolver_start())
    {
        return FVM_ERROR;
    }
    memset(&hints, 0, sizeof(hints));
    memset(&result, 0, sizeof(result));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = reverse ? AI_NUMERICHOST : 0;
    status = getaddrinfo(text, NULL, &hints, &addresses);
    if (status == 0)
    {
        for (entry = addresses; entry; entry = entry->ai_next)
        {
            if (entry->ai_family != AF_INET && entry->ai_family != AF_INET6)
            {
                continue;
            }
            if (getnameinfo(entry->ai_addr,
                            entry->ai_addrlen,
                            name,
                            sizeof(name),
                            NULL,
                            0,
                            reverse ? NI_NAMEREQD : NI_NUMERICHOST) == 0)
            {
                resolver_add(&result, name);
            }
        }
        freeaddrinfo(addresses);
    }
    resolver_finish();
    if (!result.count)
    {
        return FVM_ERROR;
    }
    memcpy(out, &result, sizeof(result));
    return FVM_OK;
}


/* Public forward lookup; borrowed host/out follow the header's blocking contract. */
FVM_Result fvm_port_resolve_host(const char *host, FVM_ResolverNames *out)
{
    return resolver_names(host, 0, out);
}


/* Public reverse lookup; borrowed address/out follow the header's blocking contract. */
FVM_Result fvm_port_resolve_address(const char *address, FVM_ResolverNames *out)
{
    return resolver_names(address, 1, out);
}


/* Resolve one service with checked decimal parsing before native service lookup.
 * network/service are borrowed; port is published only on success. No VM callbacks. */
FVM_Result fvm_port_resolve_service(const char *network, const char *service, unsigned int *port)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    unsigned int value = 0;
    size_t index;
    int numeric = 1;
    int status;
    if (!network || !service || !*service || !port)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (strcmp(network, "tcp") != 0 && strcmp(network, "udp") != 0)
    {
        return FVM_UNSUPPORTED;
    }
    if (service[0] == '+' || service[0] == '-')
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (index = 0; service[index]; index++)
    {
        if ((unsigned char)service[index] <= ' ' || (unsigned char)service[index] == 127)
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (service[index] < '0' || service[index] > '9')
        {
            numeric = 0;
        }
    }
    if (numeric)
    {
        for (index = 0; service[index]; index++)
        {
            if (value > (FVM_RESOLVER_PORT_MAX - (unsigned int)(service[index] - '0')) / 10)
            {
                return FVM_INVALID_ARGUMENT;
            }
            value = value * 10 + (unsigned int)(service[index] - '0');
        }
        *port = value;
        return FVM_OK;
    }
    if (!resolver_start())
    {
        return FVM_ERROR;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = strcmp(network, "tcp") == 0 ? SOCK_STREAM : SOCK_DGRAM;
    status = getaddrinfo("127.0.0.1", service, &hints, &addresses);
    if (status == 0)
    {
        value = ntohs(((struct sockaddr_in *)addresses->ai_addr)->sin_port);
        freeaddrinfo(addresses);
    }
    resolver_finish();
    if (status != 0)
    {
        return FVM_ERROR;
    }
    *port = value;
    return FVM_OK;
}
