/* Optional synchronous HTTP(S) transport with explicit transfer/status/ownership checks. */
#include "http.h"
#include <curl/curl.h>
#include <string.h>

/* Stack-owned forwarding state; no callback pointer outlives curl_easy_perform. */
typedef struct FVM_HTTPWrite
{
    FVM_DesktopWrite write; /* Borrowed bounded consumer. */
    void *userdata;         /* Borrowed consumer state. */
    FVM_Result status;      /* First consumer error survives curl's generic write failure. */
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
    size = width * count;
    if (state->status != FVM_OK)
        return 0;
    state->status = state->write(bytes, size, state->userdata);
    return state->status == FVM_OK ? size : 0;
}

/* Independent handles and balanced global references preserve embedding-host curl ownership. */
FVM_Result fvm_desktop_http_fetch(const char *reference, const char *base, FVM_DesktopWrite write,
                                  void *write_userdata, char *resolved, size_t capacity,
                                  void *userdata)
{
    CURLU *url = 0;
    CURL *easy = 0;
    char *canonical = 0, *scheme = 0;
    long response = 0;
    FVM_Result result = FVM_ERROR;
    FVM_HTTPWrite state;
    if (!reference || !*reference || !write || !resolved || !capacity || userdata)
        return FVM_INVALID_ARGUMENT;
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
    state.write = write;
    state.userdata = write_userdata;
    state.status = FVM_OK;
    /* Check every setting: a rejected option must not silently weaken the request. */
    if (curl_easy_setopt(easy, CURLOPT_URL, canonical) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, http_write) != CURLE_OK ||
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, &state) != CURLE_OK ||
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
