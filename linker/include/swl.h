/* Independent C99 SWO1/SWA1 linker API. No compiler or VM headers required. */
#ifndef SWL_H
#define SWL_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SwlContext SwlContext;
/* Return nonzero only after consuming all bytes. Called once per emission. */
typedef int (*SwlWriter)(void *user, const unsigned char *bytes, size_t size);
typedef struct {
    unsigned width, height, frames_per_second;
    int allow_flashvm_extensions;
} SwlOptions;
typedef struct { const unsigned char *data; size_t size; } SwlObject;
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
int swl_add_rect(SwlContext *, const char *name, unsigned width, unsigned height,
                 uint32_t rgba, char *, size_t);
int swl_place(SwlContext *, const char *name, unsigned x, unsigned y, unsigned depth, char *, size_t);
int swl_play(SwlContext *, const char *name, char *, size_t);
int swl_emit(SwlContext *, SwlWriter, void *, char *, size_t);
/* Whole-member archive, no implicit search path or lazy symbol resolution. */
int swl_archive(const SwlObject *, size_t, SwlWriter, void *, char *, size_t);
#endif
