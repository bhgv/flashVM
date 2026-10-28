/* Native C export metadata from the existing Coco/R interface catalog. */
#ifndef LIMBO_NATIVE_H
#define LIMBO_NATIVE_H
#include "signature.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* Emit one module's checked scalar native ABI, or preflight when output is NULL.
 * catalog is borrowed; cell_bytes selects classic 4 or hosted NC64 8, not a guessed host ABI.
 * Supported: byte/int/big/real/string, their unqualified scalar aliases, void and constants.
 * Reject includes, ADTs, self/variadic/polymorphic/nested exports before writing.
 * Return 1/0, copying an optional diagnostic. Callers own streams/publication and catalog lifetime.
 * No parser globals or bootstrap process is used; output I/O failure may leave a prefix. */
int limbo_native_header(FILE *output, const LimboInterface *catalog, unsigned cell_bytes, char *error, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
