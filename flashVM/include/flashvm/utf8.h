/* FlashVM: Public embedding API declarations: utf8.
 * File: include/flashvm/utf8.h.
 */

#ifndef FLASHVM_UTF8_H
/* Define FLASHVM UTF8 H constant or compilation switch. */
#define FLASHVM_UTF8_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
/* All lengths are byte counts. No NUL terminator is required for input. */
/* Define FVM_UTF8_Status enum and its representation. */
typedef enum FVM_UTF8_Status
{
    FVM_UTF8_OK = 0 /* FVM UTF  OK: stored state of the enclosing type. */,
    FVM_UTF8_INVALID = -1 /* FVM UTF  INVALID: stored state of the enclosing type. */,
    FVM_UTF8_NOSPACE = -2 /* FVM UTF  NOSPACE: stored state of the enclosing type. */,
    FVM_UTF8_RANGE = -3 /* FVM UTF  RANGE: stored state of the enclosing type. */
} FVM_UTF8_Status;

/* Validates the entire input, including embedded NUL. */


/* Validate utf8. */
FVM_UTF8_Status fvm_utf8_validate(const char *text, size_t length, size_t *characters);
/* Read one Unicode scalar at *offset; advance only on success. */


/* Read utf8. */
FVM_UTF8_Status fvm_utf8_read(const char *text, size_t length, size_t *offset,
                              unsigned long *codepoint);
/* Encode one scalar into up to four bytes; length is output byte count. */


/* Write utf8. */
FVM_UTF8_Status fvm_utf8_write(unsigned long codepoint, char *out, size_t capacity,
                               size_t *length);
/* Returns byte offset of scalar index; index == scalar count is allowed. */


/* Implement utf8 offset for this subsystem. */
FVM_UTF8_Status fvm_utf8_offset(const char *text, size_t length, size_t index, size_t *offset);
#ifdef __cplusplus
}
#endif
#endif
