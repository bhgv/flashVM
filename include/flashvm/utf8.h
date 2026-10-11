#ifndef FLASHVM_UTF8_H
#define FLASHVM_UTF8_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* All lengths are byte counts. No NUL terminator is required for input. */
typedef enum FVM_UTF8_Status { FVM_UTF8_OK=0, FVM_UTF8_INVALID=-1, FVM_UTF8_NOSPACE=-2, FVM_UTF8_RANGE=-3 } FVM_UTF8_Status;
/* Validates the entire input, including embedded NUL. */
FVM_UTF8_Status fvm_utf8_validate(const char *text, size_t length, size_t *characters);
/* Read one Unicode scalar at *offset; advance only on success. */
FVM_UTF8_Status fvm_utf8_read(const char *text, size_t length, size_t *offset, unsigned long *codepoint);
/* Encode one scalar into up to four bytes; length is output byte count. */
FVM_UTF8_Status fvm_utf8_write(unsigned long codepoint, char *out, size_t capacity, size_t *length);
/* Returns byte offset of scalar index; index == scalar count is allowed. */
FVM_UTF8_Status fvm_utf8_offset(const char *text, size_t length, size_t index, size_t *offset);
#ifdef __cplusplus
}
#endif
#endif
