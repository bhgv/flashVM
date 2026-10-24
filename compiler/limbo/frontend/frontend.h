/* Standalone C99 syntax API; no argv, output streams, exit or subprocesses. */
#ifndef limbo_FRONTEND_H
#define limbo_FRONTEND_H
/* UTF-8 scanner units and the 2 MiB input bound require octets and >=32-bit int. */
#include <limits.h>
#if CHAR_BIT != 8 || INT_MAX < 2147483647
#error "This syntax frontend requires 8-bit bytes and at least 32-bit int"
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        int code, line, column;
        long offset;
        char message[256];
    } LimboDiagnostic;
    /* Return 1 accepted / 0 rejected. Optional caller-owned diagnostic.
     * Input bytes/stream remain caller-owned. Sequential calls reset parser state;
     * serialize calls to this parser externally (not reentrant). */
    int limbo_check(const void *bytes, size_t size, LimboDiagnostic *diagnostic);
    int limbo_read(FILE *input, LimboDiagnostic *diagnostic);
    /* Immutable owned concrete syntax tree. No name binding, types or code generation. */
    typedef struct LimboTree LimboTree;
    /* Preorder node indices; SIZE_MAX means absent. Spans are half-open UTF-8 bytes.
     * Empty productions anchor at lookahead; ancestor ends include descendant anchors.
     * kind is a static grammar-production name. Tree queries copy this small view. */
    typedef struct LimboNode
    {
        const char *kind; /* Static production name; terminals are represented by source bytes. */
        size_t begin; /* Inclusive byte offset; no conversion to editor columns. */
        size_t end; /* Exclusive byte offset, including any descendant empty-rule anchors. */
        size_t parent; /* Preorder owner index, or SIZE_MAX for the root. */
        size_t first_child; /* First direct production child, or SIZE_MAX. */
        size_t next_sibling; /* Next direct sibling, or SIZE_MAX. */
    } LimboNode;
    /* Parse with at most 262144 nodes and the syntax API's existing input/work limits.
     * Preserve *out on any failure. Success transfers a new owner; caller releases old owners.
     * Input/output/diagnostic storage must be disjoint and accessible. Serialize parser calls.
     * Diagnostic -5 is tree node/storage limit, -2 allocation failure; syntax codes unchanged. */
    int limbo_tree_parse(const void *bytes, size_t size, LimboTree **out,
                         LimboDiagnostic *diagnostic);
    /* Release all nodes and the copied source; null is safe. Synchronize against readers. */
    void limbo_tree_destroy(LimboTree *tree);
    /* Return immutable cardinality; null returns zero. Successful trees start at root index 0. */
    size_t limbo_tree_count(const LimboTree *tree);
    /* Borrow NUL-terminated original UTF-8 bytes until destruction; null returns null.
     * Source size excludes the added terminator. No caller path or file is retained. */
    const char *limbo_tree_source(const LimboTree *tree);
    size_t limbo_tree_size(const LimboTree *tree);
    /* Copy one view; invalid arguments/index return zero and preserve output. */
    int limbo_tree_node(const LimboTree *tree, size_t index, LimboNode *out);
#ifdef __cplusplus
}
#endif
#endif
