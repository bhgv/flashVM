/* Immutable outer-type alias binding over the authoritative Coco/R catalog. */
#ifndef LIMBO_RESOLVE_H
#define LIMBO_RESOLVE_H
#include "signature.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define LIMBO_ALIAS_MAX_DEPTH 256U /* Bound the number of distinct alias edges per query. */


/* Borrow a direct Type production for an explicit alias/function/value declaration.
 * owner/declaration are immutable. Return 1 and copy its node index, or 0 preserving
 * output. Does not resolve aliases, includes, fields or composite members. */
int limbo_interface_declared_type(const LimboInterface *owner, size_t declaration, size_t *out);


/* Resolve unqualified outer aliases through enclosing module scopes then file scope. Each
 * replacement uses its declaration's lexical scope, not the use site's scope.
 * Return 1 with the final Type node, or 0 preserving *out. Optional diagnostic is
 * cleared; -1 invalid query, -2 allocation, -10 missing/non-alias/qualified/generic
 * name, -11 cycle/depth limit. Coordinates count source UTF-8 bytes (LF lines).
 * Composite outer types are returned unchanged; their contained types are not
 * resolved. Does not load includes/imports or perform full Limbo type checking.
 * Input/output/diagnostic storage must be disjoint. Parser globals are untouched; synchronize owner destruction and output storage.
 * Temporary name storage is freed before return; no borrowed pointer is retained. */
int limbo_interface_resolve_type(
    const LimboInterface *owner, size_t scope, size_t type_node, size_t *out, LimboDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
