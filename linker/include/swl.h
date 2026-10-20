/* Independent C99 SWO1/SWA1 linker API. No compiler or VM headers required. */
#ifndef SWL_H
#define SWL_H
/* SWO/SWA/SWF lengths and bit writers describe octets, never host-wide chars. */
#include <limits.h>
#if CHAR_BIT != 8 || INT_MAX < 2147483647
#error "SWL requires 8-bit bytes and at least 32-bit int"
#endif
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct SwlContext SwlContext;
    /* Return nonzero only after consuming all bytes. Called once per emission. */
    typedef int (*SwlWriter)(void *user, const unsigned char *bytes, size_t size);
    typedef struct
    {
        unsigned width, height;       /* Stage dimensions in pixels; accepted range is 1..3276. */
        unsigned frames_per_second;   /* Integer SWF rate, 1..255 frames/second. */
        int allow_flashvm_extensions; /* Explicit opt-in for nonstandard object capabilities. */
    } SwlOptions;
    typedef struct
    {
        const unsigned char
            *data;   /* Borrowed bytes; add/archive operations copy what they retain. */
        size_t size; /* Exact byte count, never inferred from a string terminator. */
    } SwlObject;
    /* Context owns copies of accepted inputs; caller retains every input buffer.
     * Distinct contexts are independent. Do not share a context concurrently. */
    SwlContext *swl_create(const SwlOptions *options);
    void swl_destroy(SwlContext *context);
    /* Failed adds leave the context unchanged. Every public operation returns
     * 1 success/0 failure, with a caller-owned diagnostic buffer (may be NULL/0). */
    int swl_add_object(SwlContext *, const void *, size_t, char *, size_t);
    int swl_add_archive(SwlContext *, const void *, size_t, char *, size_t);
    /* Named resources are exported; JPEG is wrapped in a bitmap-filled shape.
     * SVF1 initially supports a single filled rectangle, not arbitrary SVG. */
    int swl_add_jpeg(SwlContext *, const char *name, const void *, size_t, char *, size_t);
    int swl_add_wav(SwlContext *, const char *name, const void *, size_t, char *, size_t);
    int swl_add_svf(SwlContext *, const char *name, const void *, size_t, char *, size_t);
    int swl_add_rect(SwlContext *, const char *name, unsigned width, unsigned height, uint32_t rgba,
                     char *, size_t);
    int swl_place(SwlContext *, const char *name, unsigned x, unsigned y, unsigned depth, char *,
                  size_t);
    int swl_play(SwlContext *, const char *name, char *, size_t);
    int swl_emit(SwlContext *, SwlWriter, void *, char *, size_t);
    /* Validated module metadata. No bytecode or context ownership is transferred. */
    typedef struct
    {
        const char *name;      /* Borrowed NUL-terminated name, valid during callback only. */
        size_t code_size;      /* AVM1 bytes including the final ActionEnd. */
        unsigned requirements; /* SWO1 capability mask: bit 0 requires FlashVM channels. */
    } SwlModuleInfo;
    /* Return zero to stop visiting and report failure. Copy metadata you retain. */
    typedef int (*SwlModuleVisitor)(void *user, const SwlModuleInfo *module);
    /* Inspect one SWO1 or SWA1 buffer, selected by magic. Validate the ENTIRE input
     * before the first callback. Visits are in archive order; extension requirements
     * are reported without authorizing execution. No filesystem, output or shared
     * context state. Callback cancellation cannot undo earlier caller side effects.
     * Return 1 on complete visitation, 0 on invalid input/allocation/cancellation.
     * NULL visitor is invalid; NULL/zero diagnostic storage is permitted. */
    int swl_inspect(const void *, size_t, SwlModuleVisitor, void *, char *, size_t);
    /* Whole-member archive, no implicit search path or lazy symbol resolution. */
    int swl_archive(const SwlObject *, size_t, SwlWriter, void *, char *, size_t);
#ifdef __cplusplus
}
#endif
#endif
