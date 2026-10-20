/* C99 numeric source lowering; parser actions are declared in lower.h. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
/* Borrowed output and diagnostic storage; never closed/freed by lowering. */
static FILE *destination;
static char *diagnostic;
static size_t diagnostic_capacity;
/* Complete consumed spelling; overlong input is rejected before copying. */
static char token[256];
/* Numeric mode, next SSA number, sticky first-error state and selected language. */
static int integer_mode, serial, failed, profile;
static char module_name[256];
/* Flat translation-unit bindings. source_config resets the live prefix. */
static char names[256][256];
static int bindings[256], immutable[256], name_count, source_line;
/* Configure one translation unit; each process owns one generated parser. */
void source_config(FILE *out, int language, char *error, size_t capacity)
{
    diagnostic = error;
    diagnostic_capacity = capacity;
    source_line = 0;
    destination = out;
    profile = language;
    integer_mode = language == 4;
    serial = failed = name_count = 0;
}
/* Query semantic rejection separately from generated syntax diagnostics. */
int source_failed(void)
{
    return failed;
}
/* Report the first semantic failure without producing a deliverable file. */
static void reject(const char *message)
{
    if (!failed && diagnostic && diagnostic_capacity)
        snprintf(diagnostic, diagnostic_capacity, "source line %d: %s", source_line, message);
    failed = 1;
}
/* Capture a complete spelling; never silently truncate a long token. */
static void spelling(char *out)
{
    strcpy(out, token);
}
/* Checked radix conversion for the i32 tutorial profile, including INT32_MIN. */
static int32_t integer_literal(const char *text, int negative)
{
    const char *digits = text, *r = strchr(text, 'r');
    uint32_t value = 0, limit = negative ? UINT32_C(2147483648) : UINT32_C(2147483647);
    unsigned base = 10, digit;
    if (!r)
        r = strchr(text, 'R');
    if (r)
    {
        base = 0;
        while (digits < r)
        {
            if (*digits < '0' || *digits > '9' || base > 36)
            {
                reject("invalid Limbo radix");
                return 0;
            }
            base = base * 10 + (unsigned)(*digits++ - '0');
        }
        if (base < 2 || base > 36)
        {
            reject("Limbo radix must be 2..36");
            return 0;
        }
        digits = r + 1;
    }
    if (!*digits)
    {
        reject("missing integer digits");
        return 0;
    }
    for (; *digits; ++digits)
    {
        unsigned ch = (unsigned char)*digits;
        if (ch >= '0' && ch <= '9')
            digit = ch - '0';
        else if (ch >= 'a' && ch <= 'z')
            digit = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'Z')
            digit = ch - 'A' + 10;
        else
        {
            reject("Limbo requires an integer literal");
            return 0;
        }
        if (digit >= base || value > (limit - digit) / base)
        {
            reject("invalid digit or literal outside i32 profile");
            return 0;
        }
        value = value * base + digit;
    }
    return negative ? value == UINT32_C(2147483648) ? INT32_MIN : -(int32_t)value : (int32_t)value;
}
/* AS numeric literals are binary64; Limbo retains checked i32 spellings. */
int source_constant(int negative)
{
    char text[256], *end;
    double value = 0;
    int32_t integer = 0;
    int result = serial++;
    spelling(text);
    if (integer_mode)
        integer = integer_literal(text, negative);
    else
    {
        errno = 0;
        value = strtod(text, &end);
        if (negative)
            value = -value;
        if (!text[0] || *end || errno || !isfinite(value))
            reject("invalid finite AS numeric literal");
    }
    if (serial > 1000)
        reject("expression size limit exceeded");
    if (!failed)
    {
        fprintf(destination, "    %%v%d: %s = const.%s ", result, integer_mode ? "i32" : "f64",
                integer_mode ? "i32" : "f64");
        if (integer_mode)
            fprintf(destination, "%ld\n", (long)integer);
        else
            fprintf(destination, "%.17g\n", value);
    }
    return result;
}
/* Emit a three-address instruction; source operators are left associative. */
int source_binary(int left, int right, int op)
{
    int result = serial++;
    if (integer_mode && op == '/')
        reject("division is not available in the i32 source profile");
    if (serial > 1000)
        reject("expression size limit exceeded");
    if (!failed)
        fprintf(destination, "    %%v%d: %s = %s.%s%s %%v%d, %%v%d\n", result,
                integer_mode ? "i32" : "f64", integer_mode ? "i32" : "f64",
                op == '+'   ? "add"
                : op == '-' ? "sub"
                : op == '/' ? "div"
                            : "mul",
                integer_mode ? ".wrap" : "", left, right);
    return result;
}
/* Scanner bridge rejects complete oversized tokens before any spelling copy. */
void source_token(const char *text, int line, int oversized)
{
    source_line = line;
    if (oversized)
    {
        reject("token exceeds 255 bytes");
        token[0] = 0;
    }
    else
        strcpy(token, text);
}
/* A source binding names its current SSA value; assignment never redefines SSA. */
int source_name(void)
{
    int i;
    for (i = 0; i < name_count; ++i)
        if (!strcmp(names[i], token))
            return i;
    if (name_count == 256)
    {
        reject("variable limit exceeded");
        return 0;
    }
    strcpy(names[name_count], token);
    bindings[name_count] = -1;
    immutable[name_count] = 0;
    return name_count++;
}
int source_read(int name)
{
    if (bindings[name] < 0)
    {
        reject("read before initialized declaration");
        return 0;
    }
    return bindings[name];
}
void source_bind(int name, int value, int declaration)
{
    if (declaration && bindings[name] >= 0)
        reject("duplicate declaration");
    if (!declaration && immutable[name])
        reject("assignment to const binding");
    if (!declaration && bindings[name] < 0)
        reject("assignment before declaration");
    bindings[name] = value;
}
void source_annotation(void)
{
    if (profile == 1)
        reject("AS1 does not support type annotations");
    if (strcmp(token, "Number"))
        reject("only Number annotations are supported");
}
void source_trace(int value)
{
    if (!failed)
        fprintf(destination, "    trace %%v%d\n", value);
}
void source_module(int first)
{
    if (first)
        strcpy(module_name, token);
    else if (strcmp(token, module_name))
        reject("module name mismatch");
}
void source_language(int limbo)
{
    if (integer_mode != limbo)
        reject("source does not match selected language");
}
/* Unary minus is multiplication by -1: unlike 0-x this preserves AS -0. */
int source_negate(int value)
{
    int minus = serial++;
    if (serial > 1000)
        reject("expression size limit exceeded");
    if (!failed)
        fprintf(destination, "    %%v%d: %s = const.%s -1\n", minus, integer_mode ? "i32" : "f64",
                integer_mode ? "i32" : "f64");
    return source_binary(value, minus, '*');
}
void source_declare(int name, int value, int constant)
{
    if (constant && profile != 3)
        reject("const declaration requires AS3 profile");
    source_bind(name, value, 1);
    immutable[name] = constant;
}
void source_assign(int name, int value, int op)
{
    if (op)
        value = source_binary(source_read(name), value, op);
    source_bind(name, value, 0);
}
void source_hash_comment(void)
{
    if (!integer_mode)
        reject("# comments require Limbo profile");
}
