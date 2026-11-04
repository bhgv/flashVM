/* Local FP conversion of saved bytes; no shared payload ownership. */
#ifndef DVM_REAL_CONVERSION_VALUE_H
#define DVM_REAL_CONVERSION_VALUE_H
#include <stddef.h>


/* opcode selects CVTRF/FR/WF/LF/FW/FL. Write source/result widths through
 * nonnull owner-local pointers; return1 supported,0 unknown unchanged. No FP,
 * allocation, callback or shared access; native WORD width is retained. */
int dvm_runtime_real_conversion_sizes(int opcode, size_t *source, size_t *result);


/* source/result borrow owner-local saved bytes of validated widths. Evaluate
 * native conversion outside ALL guards. Return NULL on success, static exInval
 * for nonfinite/out-of-range rounded integer input without writing result.
 * Unknown internal opcode aborts. Native FP status/rounding/traps are unchanged;
 * no shared access, allocation, owner transfer, callback or VM unwind here. */
const char *dvm_runtime_real_conversion_value(int opcode, const void *source, void *result);


/* Copy width bytes between independently mapped source/destination spans.
 * Called under source-only or destination-only guards, never for FP evaluation.
 * Return void; no callbacks, allocation, ownership or overlap restrictions beyond
 * memmove. The caller supplies extent, lifetime and complete exclusion. */
void dvm_runtime_real_conversion_copy(void *destination, const void *source, size_t width);
#endif
