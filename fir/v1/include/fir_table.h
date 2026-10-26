/* Optional hosted reference execution for the typed straight-line table FIR subset. */
#ifndef FIR_TABLE_H
#define FIR_TABLE_H
#include "fir.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Verify and execute a caller-owned single-block table module against the C99 core.
 * Output is staged: runtime/coverage errors publish no traces. All temporary table/key owners
 * are retired before return, including aliases and failure paths. No VM/JIT or network runs here.
 * Returns one on success; zero with bounded error. Caller retains module/output ownership. */
int fir_table_run(FirModule *module, FILE *output, char *error, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
