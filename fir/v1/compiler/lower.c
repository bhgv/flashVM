/* C99 numeric source lowering; parser actions are declared in lower.h. */
#include "lower.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>

/* Borrowed output stream and copied source position; released after parsing. */
static FILE *destination;
static SourceMark location;
/* Complete consumed spelling; overlong input is rejected before copying. */
static char token[256];
/* Numeric mode, next SSA number, sticky first-error state and selected language. */
static int integer_mode, serial, failed, profile;
static char module_name[256];
/* Flat translation-unit bindings. source_config resets the live prefix. */
static char names[256][256];
static int bindings[256], immutable[256], name_count, source_line;
/* Retired lexical bindings are never reused during one bounded translation unit. */
static int binding_scope[256], active[256], scope_depth;
/* Debug owners are unique across sibling blocks, unlike lexical depth. */
static int debug_scopes[65], debug_scope_serial;


/* Configure one translation unit; each process owns one generated parser. */
void source_config(FILE *out, int language)
{
    location.line = 0;
    location.offset = -1;
    location.length = 0;
    source_line = 0;
    destination = out;
    profile = language;
    integer_mode = language == 4;
    serial = failed = name_count = scope_depth = debug_scope_serial = 0;
    debug_scopes[0] = 0;
}


/* Query semantic rejection separately from generated syntax diagnostics. */
int source_failed(void)
{
    return failed;
}


/* Report the first semantic failure without producing a deliverable file. */
static void reject(const char *message)
{
    char text[512];
    snprintf(text, sizeof(text), "source line %d: %s", source_line, message);
    source_error(FIR_DIAG_SEMANTIC, 0, location.offset, location.length, text);
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
    {
        r = strchr(text, 'R');
    }
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
        {
            digit = ch - '0';
        }
        else if (ch >= 'a' && ch <= 'z')
        {
            digit = ch - 'a' + 10;
        }
        else if (ch >= 'A' && ch <= 'Z')
        {
            digit = ch - 'A' + 10;
        }
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
    if (!negative)
    {
        return (int32_t)value;
    }
    if (value == UINT32_C(2147483648))
    {
        return INT32_MIN;
    }
    return -(int32_t)value;
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
    {
        integer = integer_literal(text, negative);
    }
    else
    {
        errno = 0;
        value = strtod(text, &end);
        if (negative)
        {
            value = -value;
        }
        if (!text[0] || *end || errno || !isfinite(value))
        {
            reject("invalid finite AS numeric literal");
        }
    }
    if (serial > 1000)
    {
        reject("expression size limit exceeded");
    }
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: %s = const.%s ",
                result,
                integer_mode ? "i32" : "f64",
                integer_mode ? "i32" : "f64");
        if (integer_mode)
        {
            fprintf(destination, "%ld\n", (long)integer);
        }
        else
        {
            fprintf(destination, "%.17g\n", value);
        }
    }
    if (!failed)
    {
        source_debug_event("constant", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Emit a three-address instruction; source operators are left associative. */
int source_binary(int left, int right, int op)
{
    int result = serial++;
    if (integer_mode && op == '/')
    {
        reject("division is not available in the i32 source profile");
    }
    if (serial > 1000)
    {
        reject("expression size limit exceeded");
    }
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: %s = %s.%s%s %%v%d, %%v%d\n",
                result,
                integer_mode ? "i32" : "f64",
                integer_mode ? "i32" : "f64",
                op == '+'   ? "add"
                : op == '-' ? "sub"
                : op == '/' ? "div"
                            : "mul",
                integer_mode ? ".wrap" : "",
                left,
                right);
    }
    if (!failed)
    {
        source_debug_event("binary", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Scanner bridge rejects complete oversized tokens before any spelling copy. */
void source_token(const char *text, int line, long offset, size_t length, int oversized)
{
    source_line = line;
    location.line = line;
    location.offset = offset;
    location.length = length;
    if (oversized)
    {
        reject("token exceeds 255 bytes");
        token[0] = 0;
    }
    else
    {
        strcpy(token, text);
    }
}


/* A source binding names its current SSA value; assignment never redefines SSA. */
int source_name(void)
{
    int i;
    for (i = name_count - 1; i >= 0; --i)
    {
        if (active[i] && !strcmp(names[i], token))
        {
            return i;
        }
    }
    if (name_count == 256)
    {
        reject("variable limit exceeded");
        return 0;
    }
    strcpy(names[name_count], token);
    bindings[name_count] = -1;
    immutable[name_count] = 0;
    binding_scope[name_count] = scope_depth;
    active[name_count] = 1;
    return name_count++;
}


/* Resolve the nearest active binding to its current SSA definition. */
int source_read(int name)
{
    if (bindings[name] < 0)
    {
        reject("read before initialized declaration");
        return 0;
    }
    return bindings[name];
}


/* Bind declarations in their lexical owner; initialization reads the outer binding.
 * Nondeclaration writes keep targeting the nearest active outer binding. */
void source_bind(int name, int value, int declaration)
{
    if (declaration && integer_mode && binding_scope[name] < scope_depth)
    {
        if (name_count == 256)
        {
            reject("variable limit exceeded");
            return;
        }
        /* Table rows are disjoint, but memmove also makes the bounded copy
         * explicit to fortified compilers before name_count is advanced. */
        memmove(names[name_count], names[name], sizeof(names[name_count]));
        name = name_count++;
        bindings[name] = -1;
        immutable[name] = 0;
        binding_scope[name] = scope_depth;
        active[name] = 1;
    }
    if (declaration && bindings[name] >= 0)
    {
        reject("duplicate declaration");
    }
    if (!declaration && immutable[name])
    {
        reject("assignment to const binding");
    }
    if (!declaration && bindings[name] < 0)
    {
        reject("assignment before declaration");
    }
    bindings[name] = value;
    if (!failed)
    {
        source_debug_event(declaration ? "declare" : "assign",
                           value,
                           names[name],
                           debug_scopes[binding_scope[name]],
                           binding_scope[name],
                           location);
    }
}


/* Restrict numeric AS annotations without claiming a general type checker. */
void source_annotation(void)
{
    if (profile == 1)
    {
        reject("AS1 does not support type annotations");
    }
    if (strcmp(token, "Number"))
    {
        reject("only Number annotations are supported");
    }
}


/* Emit the tutorial observable only while the translation remains valid. */
void source_trace(int value)
{
    if (!failed)
    {
        fprintf(destination, "    trace %%v%d\n", value);
        source_debug_event("trace", value, "", debug_scopes[scope_depth], scope_depth, location);
    }
}


/* Require implement and module declarations to use the same spelling. */
void source_module(int first)
{
    if (first)
    {
        strcpy(module_name, token);
    }
    else if (strcmp(token, module_name))
    {
        reject("module name mismatch");
    }
}


/* Reject a parsed source family that disagrees with the requested profile. */
void source_language(int limbo)
{
    if (integer_mode != limbo)
    {
        reject("source does not match selected language");
    }
}


/* Unary minus is multiplication by -1: unlike 0-x this preserves AS -0. */
int source_negate(int value)
{
    int minus = serial++;
    if (serial > 1000)
    {
        reject("expression size limit exceeded");
    }
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: %s = const.%s -1\n",
                minus,
                integer_mode ? "i32" : "f64",
                integer_mode ? "i32" : "f64");
        source_debug_event("constant", minus, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return source_binary(value, minus, '*');
}


/* Enforce AS3-only constants and retain their immutability in flat AS scope. */
void source_declare(int name, int value, int constant)
{
    if (constant && profile != 3)
    {
        reject("const declaration requires AS3 profile");
    }
    source_bind(name, value, 1);
    immutable[name] = constant;
}


/* Lower compound assignment before updating the visible lexical binding. */
void source_assign(int name, int value, int op)
{
    if (op)
    {
        value = source_binary(source_read(name), value, op);
    }
    source_bind(name, value, 0);
}


/* Keep native Limbo comments from silently changing AS lexical coverage. */
void source_hash_comment(void)
{
    if (!integer_mode)
    {
        reject("# comments require Limbo profile");
    }
}


/* Enter a lexical Limbo owner; parser nesting independently bounds recursion. */
void source_enter_scope(void)
{
    ++scope_depth;
    debug_scopes[scope_depth] = ++debug_scope_serial;
    source_debug_event("scope-enter", -1, "", debug_scopes[scope_depth], scope_depth, location);
}


/* Retire only local names; values assigned to outer names remain current. */
void source_leave_scope(void)
{
    int i;
    for (i = 0; i < name_count; ++i)
    {
        if (active[i] && binding_scope[i] == scope_depth)
        {
            active[i] = 0;
        }
    }
    source_debug_event("scope-leave", -1, "", debug_scopes[scope_depth], scope_depth, location);
    --scope_depth;
}


/* Consume over-budget block openers without recursive descent or recovery loops. */
void source_scope_limit(void)
{
    reject("block/group/unary nesting limit exceeded");
}


/* Return a copied token span, independent of later scanner/token mutations. */
SourceMark source_mark(void)
{
    return location;
}


/* Attribute target-side errors to the identifier rather than the final RHS. */
void source_restore(SourceMark mark)
{
    location = mark;
    source_line = mark.line;
}


/* Retire caller-owned stream roots after every translation, including failure. */
void source_release(void)
{
    destination = NULL;
}
