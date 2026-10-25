/* Semantic bridge used by the generated Coco/R source parser. */
#ifndef FIR_SOURCE_LOWER_H
#define FIR_SOURCE_LOWER_H
#include "fir.h"


/* Parser-local value: capture before RHS parsing and restore for target errors. */
typedef struct
{
    int line;
    long offset;
    size_t length;
} SourceMark;


/* Scalar relation selected by the authoritative Coco/R source grammar. */
typedef enum
{
    SOURCE_LESS,         /* lhs < rhs. */
    SOURCE_EQUAL,        /* lhs == rhs. */
    SOURCE_GREATER,      /* rhs < lhs. */
    SOURCE_NOT_EQUAL,    /* Invert equality when selecting source branches. */
    SOURCE_LESS_EQUAL,   /* Invert rhs < lhs. */
    SOURCE_GREATER_EQUAL /* Invert lhs < rhs. */
} SourceRelation;


/* Generated-parser-local condition; no heap state or reference escapes compilation. */
typedef struct
{
    int value;  /* Normalized FIR bool SSA index. */
    int invert; /* Reverse FIR edges to implement the source relation without bool casts. */
} SourceCondition;


/* Lower borrowed scalar operands/relation to a bool with the current source origin. */
SourceCondition source_compare(int left, int right, SourceRelation relation);


/* Start a terminal decision and return its private label identity; errors retire staged output. */
int source_branch(SourceCondition condition);


/* Start the other arm of one terminal decision; choice is the value returned by source_branch. */
void source_else(int choice);


/* Emit a terminal trace/return without changing the caller's source binding environment. */
void source_return(int value);


/* Close generated FIR, adding a return only for nonterminal AS trace programs. */
void source_finish(void);


SourceMark source_mark(void);


void source_restore(SourceMark);


/* Record only the first error; a negative offset explicitly means unlocated. */
void source_error(FirSourceDiagnosticKind, int, long, size_t, const char *);


void source_release(void);


/* Record copied source origins without exposing compiler-owned storage. */
void source_debug_event(const char *, int, const char *, int, int, SourceMark);


void source_config(FILE *, int);


int source_failed(void);


void source_token(const char *, int, long, size_t, int);


int source_constant(int);


int source_binary(int, int, int);


int source_name(void);


int source_read(int);


void source_bind(int, int, int);


void source_annotation(void);


void source_trace(int);


void source_module(int);


void source_language(int);


/* Select/check the homogeneous numeric return type at its source mark.
 * real selects binary64; declaration records the interface, zero checks the definition.
 * No stream is acquired. Mismatch records the first semantic diagnostic. */
void source_result_type(int real, int declaration);


/* Reject an explicitly typed local that differs from the selected result profile. */
void source_binding_type(int real);


/* Return the selected provenance profile after parsing, before borrowed output publication. */
FirSemantics source_semantics(void);


int source_negate(int);


void source_declare(int, int, int);


void source_assign(int, int, int);


void source_hash_comment(void);


void source_scope_limit(void);


void source_enter_scope(void);


void source_leave_scope(void);
#endif
