/* Additive C89 HTTP GET policy; old provider option structures and callbacks remain unchanged. */
#ifndef FVM_DESKTOP_HTTP_POLICY_H
#define FVM_DESKTOP_HTTP_POLICY_H
#include "source.h"
#include "../../ports/network/socket.h"
#define FVM_HTTP_REDIRECT_MAX 8U   /* Maximum caller-selected redirect transitions per resource. */
#define FVM_HTTP_MOVED 301         /* Permanent GET redirection. */
#define FVM_HTTP_FOUND 302         /* Temporary GET redirection. */
#define FVM_HTTP_SEE_OTHER 303     /* Retrieve the Location with GET. */
#define FVM_HTTP_TEMPORARY 307     /* Method-preserving temporary redirection (GET only here). */
#define FVM_HTTP_PERMANENT 308     /* Method-preserving permanent redirection (GET only here). */
#define FVM_HTTP_HEADER_LINE 8192U /* Maximum raw response line bytes. */
#define FVM_HTTP_HEADER_BYTES 32768U /* Metadata budget per native response. */
#define FVM_HTTP_HEADER_FIELDS 256U  /* Metadata field-work budget per response. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Initialize before use. All nested data remains immutable/borrowed through fetch/runner
     * return. */
    typedef struct FVM_DesktopHTTPPolicy
    {
        unsigned int max_redirects; /* 0 disables, 1..REDIRECT_MAX bounds transitions. */
        unsigned long
            connect_ms;         /* Per-hop connection cap, also bounded by remaining chain time. */
        unsigned long total_ms; /* One total monotonic budget across every hop and body. */
        const char *ca_file;    /* Optional borrowed PEM trust file; null uses provider defaults. */
    } FVM_DesktopHTTPPolicy;
    /* Initialize no redirects, default connect/total budgets and default trust; null returns zero.
     */
    int fvm_desktop_http_policy_init(FVM_DesktopHTTPPolicy *out);
#ifdef __cplusplus
}
#endif
#endif
