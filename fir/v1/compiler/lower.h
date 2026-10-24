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
int source_negate(int);
void source_declare(int, int, int);
void source_assign(int, int, int);
void source_hash_comment(void);
void source_scope_limit(void);
void source_enter_scope(void);
void source_leave_scope(void);
#endif
