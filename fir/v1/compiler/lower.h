/* Semantic bridge used by the generated Coco/R source parser. */
#ifndef FIR_SOURCE_LOWER_H
#define FIR_SOURCE_LOWER_H
#include "fir.h"


/* Parser-local value: capture before RHS parsing and restore for target errors. */
typedef struct
{
    int line;      /* One-based line of the captured source token/expression. */
    long offset;   /* Zero-based UTF-8 byte offset in the admitted source snapshot. */
    size_t length; /* Source byte span; signed literals include the unary minus. */
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


/* Parser-owned short-circuit continuation; cells are frame-local scalar identities. */
typedef struct
{
    int cell;   /* Dominating bool.slot SSA identity; never a heap/native pointer. */
    int choice; /* Private logical CFG label number, independent of terminal decisions. */
} SourceLogic;


/* Begin a lazy bool operation after left has evaluated; disjunction selects ||.
 * Return a copied continuation. Sticky type/block errors prevent publication. */
SourceLogic source_logic_begin(int left, int disjunction);



/* Complete a lazy operation after parsing its guarded RHS; return a bool SSA value.
 * The caller restores the original operator span before calling this function. */
int source_logic_end(SourceLogic logic, int right);



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



/* Record a copied exact read/write origin; binding is a one-based translation-local identity. */
void source_debug_access(const char *, int, const char *, int, int, int, SourceMark);


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
 * kind selects int=0, real=1 or big/table=2; declaration selects interface versus definition.
 * No stream is acquired. Mismatch records the first semantic diagnostic. */
void source_result_type(int kind, int declaration);



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

/* Table-profile semantic entry points. SSA indices are local to one staged translation.
 * Inputs are borrowed; errors preserve the first exact source mark and prevent publication. */



/* Query whether the interface selected the explicit big/table result profile. */
int source_table_profile(void);



/* Emit a decimal big literal from the captured token; negative applies its unary sign. */
int source_table_constant(int negative);



/* Emit the captured token as a copied key; quoted distinguishes a literal from dotted name. */
int source_table_text(int quoted);



/* Emit a fresh big-valued table owner and return its SSA identity. */
int source_table_new(void);



/* Emit a fresh big positional constructor key; position is nonnegative. */
int source_table_position(int position);



/* Compare borrowed same-typed operands and return a normalized bool SSA value.
 * relation selects a source comparator; unsupported kinds make errors sticky. */
int source_table_relation(int left, int right, SourceRelation relation);



/* Negate a borrowed bool SSA value without truthiness or reevaluation. */
int source_table_not(int value);



/* Borrow a bool SSA value as a terminal condition; reject other static types. */
SourceCondition source_table_truth(int value);


/* Emit memory-observing table count, location membership and deleting mutation. */
int source_table_count(int table);
int source_table_has(int table, int key);
int source_table_delete(int slot);



/* Capture typed owner/key SSA inputs as a location; reject incompatible static types. */
int source_table_index(int table, int key);



/* Read a table, snapshot key or Unicode scalar from borrowed typed SSA inputs. */
int source_table_subscript(int receiver, int key);



/* Emit a big read of a captured location; absent keys remain observable runtime errors. */
int source_table_load(int location);



/* Emit int string length or big table/key-snapshot length from a borrowed SSA input. */
int source_table_len(int value);



/* Combine a captured old big value with an evaluated RHS using one compound op.
 * op is a copied arithmetic/bitwise symbol or '<'/'>' shift direction. Errors
 * remain sticky; this is not a lock, transaction or remote owner update. */
int source_table_update(int current, int right, int op);



/* Emit typed big arithmetic on borrowed translation-local SSA values; op selects +, -, *, /, %, &, | or ^. */
int source_table_binary(int left, int right, int op);



/* Emit a big unary SSA result; op is +, - or ~, and type/operator errors are sticky. */
int source_table_unary(int value, int op);



/* Convert borrowed typed SSA explicitly: target 0=big, 1=string, 2=int. Errors are sticky. */
int source_table_convert(int value, int target);



/* Emit checked big shift with an int count; direction selects left (<) or arithmetic right (>). */
int source_table_shift(int left, int right, int direction);



/* Reflect the type of a borrowed supported SSA value; keys inspect their actual snapshot kind. */
int source_table_typeof(int value);



/* Emit an independent all-key snapshot of the typed table SSA input. */
int source_table_keys(int table);



/* Emit a checked table/string slice; borrowed bounds are big for tables and int for strings. */
int source_table_slice(int table, int first, int end);



/* Capture relative slice bounds before RHS evaluation; all inputs are borrowed SSA values. */
int source_table_range(int table, int first, int end);



/* Replace a captured table/string range using a borrowed same-kind RHS.
 * Return -1 for table mutation or the fresh string SSA identity for local rebind. */
int source_table_splice(int slot, int value);



/* Reject a compound slice assignment before attempting a scalar load. */
void source_table_slice_compound(void);



/* Emit an effectful write; location is captured once and value must have big type. */
void source_table_store(int location, int value);



/* Bind a local name to its typed SSA value; -1 permits only a typed uninitialized declaration. */
void source_table_bind(int name, int value, int type, int declaration);



/* Require a big SSA result and emit the tutorial trace/return harness. */
void source_table_return(int value);


/* Query static string receiver type without evaluating its value again. */
int source_string_receiver(int receiver);



/* Capture one int rune index as a checked immutable string range. */
int source_string_char_range(int receiver, int index);



/* Convert a borrowed int SSA scalar to validated one-rune replacement text. */
int source_string_rune(int value);

#endif
