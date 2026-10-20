/* Optional synchronous HTTP(S) transport with explicit transfer/status/ownership checks. */
#include "http.h"
#include <curl/curl.h>
#include <string.h>

/* Stack-owned forwarding state; no callback pointer outlives curl_easy_perform. */
typedef struct FVM_HTTPWrite
{
    FVM_DesktopWrite write;        /* Borrowed bounded consumer. */
    void *userdata;                /* Borrowed consumer state. */
    const FVM_NetControl *control; /* Borrowed, thread-safe cancellation callback. */
    FVM_Result status;             /* First consumer error survives curl's generic write failure. */
} FVM_HTTPWrite;

/* Translate libcurl's item counts without overflow or truncation. */
static size_t http_write(char *bytes, size_t width, size_t count, void *userdata)
{
    FVM_HTTPWrite *state = (FVM_HTTPWrite *)userdata;
    size_t size;
    if (width && count > (size_t)-1 / width)
    {
        state->status = FVM_INVALID_ARGUMENT;
        return 0;
    }
    if (state->control && state->control->cancelled &&
        state->control->cancelled(state->control->userdata))
    {
        state->status = FVM_ERROR;
        return 0;
    }
    size = width * count;
    if (state->status != FVM_OK)
        return 0;
    state->status = state->write(bytes, size, state->userdata);
    return state->status == FVM_OK ? size : 0;
}

/* libcurl invokes progress periodically even while waiting for response bytes. */
static int http_progress(void *userdata, curl_off_t download_total, curl_off_t downloaded,
                         curl_off_t upload_total, curl_off_t uploaded)
{
    FVM_HTTPWrite *state = (FVM_HTTPWrite *)userdata;
    (void)download_total;
    (void)downloaded;
    (void)upload_total;
    (void)uploaded;
    return state->control && state->control->cancelled &&
           state->control->cancelled(state->control->userdata);
}
/* Independent handles and balanced global references preserve embedding-host curl ownership. */
static FVM_Result http_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                             void *write_userdata, char *resolved, size_t capacity,
                             const FVM_NetControl *control)
{
    CURLU *url = 0;
    CURL *easy = 0;
    char *canonical = 0, *scheme = 0;
    long response = 0;
    FVM_Result result = FVM_ERROR;
    FVM_HTTPWrite state;
    if (!reference || !*reference || !write || !resolved || !capacity)
        return FVM_INVALID_ARGUMENT;
    if (control && control->cancelled && control->cancelled(control->userdata))
        return FVM_ERROR;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
        return FVM_ERROR;
    url = curl_url();
    if (!url)
    {
        result = FVM_OUT_OF_MEMORY;
        goto cleanup;
    }
    /* An absolute reference overrides a non-HTTP embedded/local provenance label. */
    if (!strstr(reference, "://") && base && curl_url_set(url, CURLUPART_URL, base, 0) != CURLUE_OK)
        goto cleanup;
    if (curl_url_set(url, CURLUPART_URL, reference, 0) != CURLUE_OK ||
        curl_url_get(url, CURLUPART_SCHEME, &scheme, 0) != CURLUE_OK ||
        (strcmp(scheme, "http") && strcmp(scheme, "https")) ||
        curl_url_get(url, CURLUPART_URL, &canonical, 0) != CURLUE_OK ||
        strlen(canonical) >= capacity)
        goto cleanup;
    easy = curl_easy_init();
    if (!easy)
    {
        result = FVM_OUT_OF_MEMORY;
        goto cleanup;
    }
    state.control = control;
    state.write = write;
    state.userdata = write_userdata;
    state.status = FVM_OK;
    /* Check every setting: a rejected option must not silently weaken the request. */
    if (curl_easy_setopt(easy, CURLOPT_URL, canonical) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, http_write) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, &state) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_XFERINFOFUNCTION, http_progress) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_XFERINFODATA, &state) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_NOPROGRESS, control && control->cancelled ? 0L : 1L) !=
            CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, FVM_DESKTOP_HTTP_CONNECT_MS) !=
            CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, FVM_DESKTOP_HTTP_TOTAL_MS) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 0L) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L) != CURLE_OK)
        goto cleanup;
    if (curl_easy_perform(easy) != CURLE_OK || state.status != FVM_OK)
    {
        result = state.status != FVM_OK ? state.status : FVM_ERROR;
        goto cleanup;
    }
    if (curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response) != CURLE_OK ||
        response < FVM_DESKTOP_HTTP_SUCCESS_MIN || response >= FVM_DESKTOP_HTTP_SUCCESS_END)
        goto cleanup;
    if (control && control->cancelled && control->cancelled(control->userdata))
        goto cleanup;
    memcpy(resolved, canonical, strlen(canonical) + 1);
    result = FVM_OK;
cleanup:
    curl_easy_cleanup(easy);
    curl_free(canonical);
    curl_free(scheme);
    curl_url_cleanup(url);
    curl_global_cleanup();
    return result;
}

/* Query the linked SDK rather than claiming HTTPS for a libcurl built without TLS/HTTP. */
unsigned int fvm_desktop_http_capabilities(void)
{
    const curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    const char *const *protocol;
    unsigned int flags = FVM_DESKTOP_NETWORK_CURL;
    if (!info || !info->protocols)
        return flags;
    for (protocol = info->protocols; *protocol; ++protocol)
    {
        if (!strcmp(*protocol, "http"))
            flags |= FVM_DESKTOP_NETWORK_HTTP;
        if (!strcmp(*protocol, "https") && (info->features & CURL_VERSION_SSL))
            flags |= FVM_DESKTOP_NETWORK_HTTPS;
    }
    return flags;
}

/* Preserve the original null-userdata contract and synchronous provider ABI. */
FVM_Result fvm_desktop_http_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                                  void *write_userdata, char *resolved, size_t capacity,
                                  void *userdata)
{
    if (userdata)
        return FVM_INVALID_ARGUMENT;
    return http_fetch(reference, base, write, write_userdata, resolved, capacity, 0);
}
/* Additive controlled entrypoint; the callback state remains borrowed until return. */
FVM_Result fvm_desktop_http_fetch_controlled(const char *reference, const char *base,
                                             FVM_DesktopWrite write, void *write_userdata,
                                             char *resolved, size_t capacity, void *userdata)
{
    return http_fetch(reference, base, write, write_userdata, resolved, capacity,
                      (const FVM_NetControl *)userdata);
}

/* Older libcurl global-init/cleanup APIs cannot safely execute on an arbitrary worker. */
FVM_Result fvm_desktop_http_transfer_fetch(const char *reference, const char *base,
                                           FVM_DesktopWrite write, void *write_userdata,
                                           char *resolved, size_t capacity,
                                           const FVM_NetControl *control, void *userdata)
{
    if (!fvm_desktop_http_background_supported())
        return FVM_UNSUPPORTED;
    if (userdata)
        return FVM_INVALID_ARGUMENT;
    return http_fetch(reference, base, write, write_userdata, resolved, capacity, control);
}

/* Owner-thread metadata check also lets an IDE avoid unsupported older libcurl worker APIs. */
int fvm_desktop_http_background_supported(void)
{
#ifdef CURL_VERSION_THREADSAFE
    const curl_version_info_data *info = curl_version_info(CURLVERSION_NOW);
    return info && (info->features & CURL_VERSION_THREADSAFE) &&
           (fvm_desktop_http_capabilities() & FVM_DESKTOP_NETWORK_HTTP);
#else
    return 0;
#endif
}
