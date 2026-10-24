/* Declaration catalog for explicit Limbo interfaces; no target ABI/layout is inferred. */
#ifndef LIMBO_INTERFACE_H
#define LIMBO_INTERFACE_H
#include "frontend.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Syntactic declaration categories; named aliases are not resolved to underlying types. */
    typedef enum LimboDeclKind
    {
        LIMBO_MODULE,
        LIMBO_ADT,
        LIMBO_FUNCTION,
        LIMBO_CONSTANT,
        LIMBO_ALIAS,
        LIMBO_EXCEPTION,
        LIMBO_IMPORT,
        LIMBO_VALUE,
        LIMBO_INCLUDE
    } LimboDeclKind;
    /* Owns a production tree, declaration names and indexed immutable views. */
    typedef struct LimboInterface LimboInterface;
    /* All byte ranges refer to the retained tree's original UTF-8 snapshot. */
    typedef struct LimboDeclaration
    {
        LimboDeclKind kind; /* Declared category, before alias/import/type resolution. */
        const char *name;   /* Borrowed NUL-terminated name; empty for an include directive. */
        size_t scope;       /* Enclosing module declaration index; SIZE_MAX is file scope. */
        size_t node;        /* Original Top production index in the retained tree. */
        size_t name_begin;  /* Inclusive identifier byte offset. */
        size_t name_end;    /* Exclusive identifier byte offset. */
        size_t type_begin;  /* DeclType start, or Top start for include. */
        size_t type_end;    /* DeclType end, or Top end for include. */
    } LimboDeclaration;
    /* Parse explicit declarations, preserve *out on error, transfer a new owner on success.
     * Includes are cataloged but never opened/expanded. ADT fields remain in the tree.
     * Function bodies, inferred declarations, implement and grouped module definitions
     * are rejected with -6. Duplicate scope/name is -7, >16384 declarations is -8.
     * -2 means allocation failure; syntax/tree diagnostics retain their existing codes.
     * Storage arguments must be disjoint. Serialize parser calls as for limbo_tree_parse. */
    int limbo_interface_parse(const void *data, size_t size, LimboInterface **out,
                              LimboDiagnostic *diagnostic);
    /* Null-safe release; synchronize against all borrowed views and readers. */
    void limbo_interface_destroy(LimboInterface *interface);
    /* Borrow the complete parse product; never destroy the borrowed tree separately. */
    const LimboTree *limbo_interface_tree(const LimboInterface *interface);
    /* Count catalog entries in source order, with grouped names expanded left to right. */
    size_t limbo_interface_count(const LimboInterface *interface);
    /* Copy one declaration; invalid arguments/index preserve output and return zero. */
    int limbo_interface_declaration(const LimboInterface *interface, size_t index,
                                    LimboDeclaration *out);
    /* Find an exact UTF-8 name within a module declaration or SIZE_MAX file scope.
     * Scope must name a module from this catalog. No normalization/overloading/import lookup.
     * name must be accessible and NUL-terminated. Return zero and preserve *index on any
     * miss/error. Immutable binary search uses no parser globals or allocation. */
    int limbo_interface_find(const LimboInterface *interface, size_t scope, const char *name,
                             size_t *index);
    /* Return a stable ASCII category name, or NULL for an invalid enum value. */
    const char *limbo_declaration_kind(LimboDeclKind kind);
#ifdef __cplusplus
}
#endif
#endif
