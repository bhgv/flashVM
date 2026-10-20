/* Transport-neutral redirect policy; bounded URI resolution, clocks and cancellation only. */
#include "http_redirect.h"
#include "../../ports/network/clock.h"
#include <string.h>
/* Stack-owned bridge retains one deadline while individual transport handles come and go. */
typedef struct FVM_HTTPChain
{
    unsigned long start; /* Monotonic sample before the first transport hop. */
    unsigned long total; /* Whole-chain millisecond budget, never refreshed by redirects. */
    const FVM_NetControl *control; /* Optional borrowed outer cancellation callback. */
} FVM_HTTPChain;
/* No native resources are created while initializing the caller's policy. */
int fvm_desktop_http_policy_init(FVM_DesktopHTTPPolicy *out)
{
    if (!out)
        return 0;
    out->max_redirects = 0;
    out->connect_ms = FVM_NET_CONNECT_MS;
    out->total_ms = FVM_NET_TOTAL_MS;
    out->ca_file = 0;
    return 1;
}
/* Status family is deliberately narrower than arbitrary 3xx responses or browser navigation. */
int fvm_http_redirect_status(int status)
{
    return status == FVM_HTTP_MOVED || status == FVM_HTTP_FOUND || status == FVM_HTTP_SEE_OTHER ||
           status == FVM_HTTP_TEMPORARY || status == FVM_HTTP_PERMANENT;
}
/* URI resolver validates schemes/authority/escaping later, before any next-hop connection. */
int fvm_http_redirect_location(FVM_HTTPResponse *response, const char *value, size_t size)
{
    size_t i;
    if (!response || !value || !size || size > FVM_DESKTOP_MAX_LOCATION || response->location_seen)
        return 0;
    for (i = 0; i < size; ++i)
        if ((unsigned char)value[i] <= ' ' || (unsigned char)value[i] >= FVM_SOCKET_ASCII_DEL)
            return 0;
    memcpy(response->location, value, size);
    response->location[size] = 0;
    response->location_seen = 1;
    return 1;
}
/* Match the one header needed by the policy without locale-dependent case conversion. */
static int redirect_location_name(const char *bytes, size_t size)
{
    const char name[] = "location:";
    size_t i;
    if (size < sizeof(name) - 1)
        return 0;
    for (i = 0; i < sizeof(name) - 1; ++i)
    {
        char c = bytes[i];
        if (c >= 'A' && c <= 'Z')
            c = (char)(c + ('a' - 'A'));
        if (c != name[i])
            return 0;
    }
    return 1;
}
/* libcurl validates transport framing; this enforces independent bounded redirect metadata. */
int fvm_http_redirect_header(FVM_HTTPResponse *response, const char *bytes, size_t size)
{
    size_t begin, end, i;
    if (!response || !bytes || size < 2 || size > FVM_HTTP_HEADER_LINE ||
        size > FVM_HTTP_HEADER_BYTES - response->header_bytes || bytes[size - 2] != '\r' ||
        bytes[size - 1] != '\n')
        return 0;
    response->header_bytes += size;
    for (i = 0; i + 2 < size; ++i)
        if (!bytes[i] || ((unsigned char)bytes[i] < ' ' && bytes[i] != '\t') ||
            (unsigned char)bytes[i] == FVM_SOCKET_ASCII_DEL)
            return 0;
    if (size >= 12 && !memcmp(bytes, "HTTP/", 5))
    {
        const char *space = (const char *)memchr(bytes, ' ', size - 2);
        size_t remaining = space ? (size_t)(bytes + size - 2 - space) : 0;
        if (!space || remaining < 4 || space[1] < '1' || space[1] > '5' || space[2] < '0' ||
            space[2] > '9' || space[3] < '0' || space[3] > '9' ||
            (remaining > 4 && space[4] != ' '))
            return 0;
        response->status = (space[1] - '0') * 100 + (space[2] - '0') * 10 + space[3] - '0';
        response->location_seen = 0;
        response->location[0] = 0;
        return 1;
    }
    if (size == 2)
        return 1;
    if (bytes[0] == ' ' || bytes[0] == '\t')
        return 0; /* No obsolete folded fields, even when libcurl tolerates them. */
    if (++response->fields > FVM_HTTP_HEADER_FIELDS)
        return 0;
    if (!fvm_http_redirect_status(response->status) || !redirect_location_name(bytes, size))
        return 1;
    begin = sizeof("location:") - 1;
    end = size - 2;
    while (begin < end && (bytes[begin] == ' ' || bytes[begin] == '\t'))
        ++begin;
    while (end > begin && (bytes[end - 1] == ' ' || bytes[end - 1] == '\t'))
        --end;
    return fvm_http_redirect_location(response, bytes + begin, end - begin);
}
/* A clock failure is treated as an expired budget; do not silently extend network work. */
static unsigned long redirect_remaining(FVM_HTTPChain *chain)
{
    unsigned long now, elapsed;
    if (!fvm_net_clock(&now))
        return 0;
    elapsed = (now - chain->start) & FVM_NET_CLOCK_MASK;
    return elapsed >= chain->total ? 0 : chain->total - elapsed;
}
/* The transport can query this bridge while blocked between response chunks. */
static int redirect_cancelled(void *userdata)
{
    FVM_HTTPChain *chain = (FVM_HTTPChain *)userdata;
    return (chain->control && chain->control->cancelled &&
            chain->control->cancelled(chain->control->userdata)) ||
           !redirect_remaining(chain);
}
/* Old callbacks are never switched implicitly to this explicit redirect-capable profile. */
FVM_Result fvm_http_redirect_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                                   void *sink, char *resolved, size_t capacity,
                                   const FVM_DesktopHTTPPolicy *policy,
                                   const FVM_NetControl *control, FVM_HTTPHop hop)
{
    FVM_SocketURL current, next;
    FVM_DesktopHTTPPolicy limits;
    FVM_HTTPResponse response;
    FVM_HTTPChain chain;
    FVM_NetControl bridge;
    unsigned int redirects = 0;
    FVM_Result result;
    if (!policy || !hop || !write || !resolved || !capacity ||
        policy->max_redirects > FVM_HTTP_REDIRECT_MAX || !policy->connect_ms ||
        policy->connect_ms > policy->total_ms || policy->total_ms > FVM_NET_MAX_MS)
        return FVM_INVALID_ARGUMENT;
    result = fvm_socket_url_resolve(reference, base, &current);
    if (result != FVM_OK)
        return result;
    chain.total = policy->total_ms;
    chain.control = control;
    if (!fvm_net_clock(&chain.start))
        return FVM_ERROR;
    bridge.cancelled = redirect_cancelled;
    bridge.userdata = &chain;
    for (;;)
    {
        if (strlen(current.canonical) >= capacity)
            return FVM_INVALID_ARGUMENT;
        limits = *policy;
        limits.total_ms = redirect_remaining(&chain);
        if (!limits.total_ms || redirect_cancelled(&chain))
            return FVM_ERROR;
        if (limits.connect_ms > limits.total_ms)
            limits.connect_ms = limits.total_ms;
        memset(&response, 0, sizeof(response));
        result = hop(&current, write, sink, &limits, &bridge, &response);
        if (result != FVM_OK)
            return result;
        if (redirect_cancelled(&chain))
            return FVM_ERROR;
        if (!fvm_http_redirect_status(response.status))
        {
            memcpy(resolved, current.canonical, strlen(current.canonical) + 1);
            return FVM_OK;
        }
        if (!response.location_seen || redirects++ >= policy->max_redirects)
            return FVM_ERROR;
        result = fvm_socket_url_resolve(response.location, current.canonical, &next);
        if (result != FVM_OK)
            return result;
        if (current.tls && !next.tls)
            return FVM_UNSUPPORTED;
        if (!strcmp(current.canonical, next.canonical))
            return FVM_ERROR;
        current = next;
    }
}
