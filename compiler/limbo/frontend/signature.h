/* Structured function views over the owned Coco/R tree, without ABI guesses. */
#ifndef LIMBO_SIGNATURE_H
#define LIMBO_SIGNATURE_H
#include "interface.h"
#ifdef __cplusplus
extern "C" {
#endif
/* All node indices refer to limbo_interface_tree(owner). SIZE_MAX means absent.
 * Counts expand grouped names. A variadic star is not a fixed argument. */
typedef struct LimboSignature
{
    size_t type_node;
    size_t parameters_node;
    size_t result_node;
    size_t parameter_count;
    int variadic;
    int polymorphic;
} LimboSignature;
/* Parameter names and types borrow source byte ranges, never native frames.
 * self-qualified parameters have a NamedType node and self_parameter=1. */
typedef struct LimboParameter
{
    size_t name_node; /* Cursor for limbo_interface_parameter_next. */
    size_t name_begin;
    size_t name_end;
    size_t type_node;
    int self_parameter;
} LimboParameter;
/* Immutable allocation-free queries. Invalid owner/declaration/index/output
 * returns zero and preserves *out. Owners must outlive all borrowed views.
 * Read-only calls use no parser globals; synchronize owner destruction. */
int limbo_interface_signature(const LimboInterface *, size_t declaration, LimboSignature *out);
int limbo_interface_parameter(const LimboInterface *, size_t declaration, size_t parameter,
                              LimboParameter *out);
/* Start with previous_name=SIZE_MAX; thereafter pass the returned name_node.
 * Advances across grouped formals in source order, with no quadratic rescanning.
 * A foreign/non-parameter cursor is rejected and preserves output. */
int limbo_interface_parameter_next(const LimboInterface *, size_t declaration,
                                   size_t previous_name, LimboParameter *out);
/* First semantic binding check: reject repeated fixed parameter identifiers (-9)
 * while allowing repeated nil placeholders. No alias/type/layout resolution.
 * Allocates temporary per-function sorted keys, freed before return; -2 allocation
 * failure, -1 null owner. Optional diagnostic is cleared each call. Owner unchanged.
 * Diagnostic offsets/columns count original UTF-8 bytes, LF-delimited one-based lines. */
int limbo_interface_validate_functions(const LimboInterface *, LimboDiagnostic *);
/* Classify an existing Type production by its grammar-selected outer shape.
 * Static strings: byte/int/big/real/string/array/list/chan/ref/fn/tuple/fixed/named.
 * Named aliases stay named. Invalid or non-Type nodes return NULL. No layouts,
 * include expansion, recursive alias resolution or structural signatures. */
const char *limbo_interface_type_kind(const LimboInterface *, size_t type_node);
#ifdef __cplusplus
}
#endif
#endif
