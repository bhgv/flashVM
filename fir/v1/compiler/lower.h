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

/* Alias bridges execute only inside the serialized source parser. They borrow its
 * current token/mark and may emit FIR to borrowed staging streams. They perform no
 * runtime operations or ownership transfer.
 * Rejections set the existing sticky diagnostic; callers must not publish failed output. */
#define SOURCE_ALIAS_MAX 128U /* Bounded source-profile aliases per translation unit. */


/* Capture the consumed named RHS and return its unresolved T_VOID marker. */
FirType source_alias_target(void);


/* Resolve all declarations after the prefix; missing/cyclic unused aliases also fail. */
void source_alias_validate(void);


/* Return whether borrowed scanner spelling name denotes a visible source alias.
 * This read-only predicate neither consumes tokens nor changes diagnostics. */
int source_alias_named(const char *name);


/* Cast compiler SSA value to the previously resolved target; see lower.c for
 * profile restrictions. Returns SSA ID; sticky errors prevent publication. */
int source_alias_cast(int value, FirType target);


/* Capture the current identifier/mark as a pending module or alias header. */
void source_alias_header(void);


/* found is the generated prefix flag; reject zero, return no value. */
void source_alias_module(int found);


/* Select file (0) or implementing-module (1) alias scope; owner/parser-only. */
void source_alias_scope(int module);


/* Resolve a declared alias at the current token; return canonical type.
 * Unknown/cyclic references are sticky semantic errors. No source reparsing or I/O. */
FirType source_alias_type(void);


/* Bind pending alias to canonical type (a resolved FirType), without emitting IR.
 * Duplicate/limit failures point to the original header; no ownership escapes. */
void source_alias_declare(FirType type);


/* Resolve a current alias and admit it as interface/definition result (declaration=1/0). */
void source_result_alias(int declaration);


/* Require the current alias to match the homogeneous int/real local profile. */
void source_binding_alias(void);


/* Return canonical annotation type in the big/table profile, or T_VOID on rejection. */
FirType source_table_alias(void);


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


/* Emit lock/unlock for a typed table SSA receiver, selected by acquire.
 * Borrowed strings emit nothing; generated entry may suspend or throw. */
void source_table_lease(int receiver, int acquire);


/* Start a bounded while CFG, promoting initialized typed locals; return its ID. */
int source_loop_begin(void);


/* Emit the checked bool test for a borrowed parser loop ID and condition. */
void source_loop_condition(int loop, SourceCondition condition);


/* Emit a loop backedge/continuation after lexical body cleanup. */
void source_loop_end(int loop);


/* Parser-owned captured sequence traversal. All fields are compiler SSA IDs, not pointers. */
typedef struct
{
    int loop;  /* Enclosing while-style CFG identity. */
    int cell;  /* Private mutable traversal position. */
    int key;   /* Current table key or string/snapshot position. */
    int value; /* Current big payload, rune int or snapshot key. */
} SourceIteration;


/* Capture a table/string/key sequence, build its CFG and produce typed per-pass values. */
SourceIteration source_iteration_begin(int table);


/* Advance the private cursor and emit its backedge after retiring body locals. */
void source_iteration_end(SourceIteration iteration);


/* Declare iteration value and optional key (-1 omits it) at authored body-scope marks. */
void source_iteration_bind(SourceIteration iteration, int key, int value, SourceMark keymark, SourceMark valuemark);


/* Parser-owned function/loop conditional, separate from the numeric terminal profile. */
typedef struct
{
    int choice;     /* Unique yes/no/join label identity. */
    int yes_closed; /* True arm ended in return or loop transfer on every path. */
} SourceChoice;


/* Reject a statement after a path-closing transfer; no runtime work. */
void source_statement(void);


/* Emit an innermost-loop transfer; is_continue selects cursor/test versus exit. */
void source_loop_transfer(int is_continue);


/* Promote join-carried typed locals before parsing a structured condition. */
SourceChoice source_choice_begin(void);


/* Emit checked bool condition edges for a borrowed structured choice. */
void source_choice_test(SourceChoice choice, SourceCondition condition);


/* Finish the true arm, record its closure, and begin the false arm. */
void source_choice_else(SourceChoice *choice);


/* Merge only normal paths; omit a join if both arms transfer out. */
void source_choice_end(SourceChoice choice);


/* Type-check a big function result and close the current path with return. */
void source_flow_return(int value);


/* Parser-owned counted-for continuation; no native/runtime pointer escapes. */
typedef struct
{
    int loop;    /* Shared innermost-loop label identity. */
    int context; /* Private staged-step stream slot, -1 after a resource failure. */
} SourceFor;


/* Begin a classic for after its initializer; own one staged step until end/release. */
SourceFor source_for_begin(void);


/* Produce the checked constant-true condition of an omitted middle clause. */
SourceCondition source_for_true(void);


/* Redirect only FIR emission into the owned header-step stream. */
void source_for_step_begin(SourceFor control);


/* Restore the borrowed destination after parsing the header step. */
void source_for_step_end(SourceFor control);


/* Emit a reachable body/continue step and backedge, or discard an unused step. */
void source_for_end(SourceFor control);


/* Return one modular same-type big/int update; direction selects +/- one.
 * Borrow the SSA input, reject other types and preserve statement-only semantics. */
int source_table_increment(int value, int direction);


/* Defer step origins by private context ID, then resume ordinary debug emission. */
void source_debug_defer_begin(int context);
void source_debug_defer_end(void);


/* Publish a reachable step's origins or discard them; always free captured nodes. */
void source_debug_defer_flush(int context, int emit);


/* Release all pending origin captures after successful or failed parsing. */
void source_debug_defer_release(void);


/* Require a closed big-result path at the end of the structured function body. */
void source_flow_finish(void);

#endif
