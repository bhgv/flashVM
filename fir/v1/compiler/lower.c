/* C99 numeric source lowering; parser actions are declared in lower.h. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
static FILE *destination;
static char token[256];
static int integer_mode, serial, failed, profile;
static char module_name[256];
static char names[256][256];
static int bindings[256], name_count, source_line;
/* Configure one translation unit; each process owns one generated parser. */
void source_config(FILE *out, int language)
{
    destination = out; profile = language; integer_mode = language == 4;
    serial = failed = name_count = 0;
}
/* Query semantic rejection separately from generated syntax diagnostics. */
int source_failed(void) { return failed; }
/* Report the first semantic failure without producing a deliverable file. */
static void reject(const char *message)
{
    if (!failed) fprintf(stderr, "source line %d: %s\n", source_line, message);
    failed = 1;
}
/* Capture a complete spelling; never silently truncate a long token. */
static void spelling(char *out)
{
    strcpy(out, token);
}
/* Emit a finite literal with explicit language arithmetic semantics. */
int source_constant(int negative)
{
    char text[256], *end;
    double value;
    int result = serial++;
    spelling(text); errno = 0; value = strtod(text, &end);
    if (negative) value = -value;
    if (!text[0] || *end || errno || !isfinite(value)) reject("invalid literal");
    if (integer_mode && strpbrk(text, ".eE")) reject("Limbo int literal requires a decimal integer spelling");
    if (integer_mode && (value < INT32_MIN || value > INT32_MAX || value != (double)(int32_t)value))
        reject("Limbo literal outside i32 range");
    if (serial > 1000) reject("expression size limit exceeded");
    if (failed) return result;
    fprintf(destination, "    %%v%d: %s = const.%s ", result,
            integer_mode ? "i32" : "f64", integer_mode ? "i32" : "f64");
    if (integer_mode) fprintf(destination, "%ld\n", (long)(int32_t)value);
    else fprintf(destination, "%.17g\n", value);
    return result;
}
/* Emit a three-address instruction; source operators are left associative. */
int source_binary(int left, int right, int op)
{
    int result = serial++;
    if (serial > 1000) reject("expression size limit exceeded");
    if (!failed) fprintf(destination, "    %%v%d: %s = %s.%s%s %%v%d, %%v%d\n", result,
        integer_mode ? "i32" : "f64", integer_mode ? "i32" : "f64",
        op == '+' ? "add" : op == '-' ? "sub" : "mul", integer_mode ? ".wrap" : "", left, right);
    return result;
}
/* Scanner bridge rejects complete oversized tokens before any spelling copy. */
void source_token(const char *text, int line, int oversized)
{
    source_line = line;
    if (oversized) { reject("token exceeds 255 bytes"); token[0] = 0; }
    else strcpy(token, text);
}
/* A source binding names its current SSA value; assignment never redefines SSA. */
int source_name(void)
{
    int i;
    for (i = 0; i < name_count; ++i) if (!strcmp(names[i], token)) return i;
    if (name_count == 256) { reject("variable limit exceeded"); return 0; }
    strcpy(names[name_count], token); bindings[name_count] = -1;
    return name_count++;
}
int source_read(int name)
{
    if (bindings[name] < 0) { reject("read before initialized declaration"); return 0; }
    return bindings[name];
}
void source_bind(int name, int value, int declaration)
{
    if (declaration && bindings[name] >= 0) reject("duplicate declaration");
    if (!declaration && bindings[name] < 0) reject("assignment before declaration");
    bindings[name] = value;
}
void source_annotation(void)
{
    if (profile == 1) reject("AS1 does not support type annotations");
    if (strcmp(token, "Number")) reject("only Number annotations are supported");
}
void source_trace(int value)
{
    if (!failed) fprintf(destination, "    trace %%v%d\n", value);
}
void source_module(int first)
{
    if (first) strcpy(module_name, token);
    else if (strcmp(token, module_name)) reject("module name mismatch");
}
void source_language(int limbo)
{
    if (integer_mode != limbo) reject("source does not match selected language");
}
