/* Semantic bridge used by the generated Coco/R source parser. */
#ifndef FIR_SOURCE_LOWER_H
#define FIR_SOURCE_LOWER_H
#include <stdio.h>
void source_config(FILE *, int);
int source_failed(void);
void source_token(const char *, int, int);
int source_constant(int);
int source_binary(int, int, int);
int source_name(void);
int source_read(int);
void source_bind(int, int, int);
void source_annotation(void);
void source_trace(int);
void source_module(int);
void source_language(int);
#endif
