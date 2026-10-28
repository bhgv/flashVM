/* C99 numeric source lowering; parser actions are declared in lower.h. */
#include "lower.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <inttypes.h>


/* Borrowed output stream and copied source position; released after parsing. */
static FILE *destination;
static SourceMark location;


/* Complete consumed spelling; overlong input is rejected before copying. */
static char token[256];


/* Numeric mode, next SSA number, sticky first-error state and selected language. */
static int integer_mode, serial, failed, profile;
static char module_name[256];


/* One bounded source-profile declaration, with a canonical type or named edge. */
typedef struct
{
    char name[256];    /* Copied bounded scanner token, never an input pointer. */
    FirType type;      /* Canonical type, or T_VOID until its named edge resolves. */
    char target[256];  /* Copied RHS identifier; empty for a primitive target. */
    SourceMark origin; /* Exact RHS token for missing-name/cycle diagnostics. */
    int scope;         /* File=0, module=1, or a lexical body scope>=2. */
} SourceAlias;

static SourceAlias aliases[SOURCE_ALIAS_MAX]; /* Parser-owned bounded declaration prefix. */
static unsigned int alias_total;              /* All declarations this request, including retired blocks. */
static unsigned int alias_floor[65];          /* Active alias prefix saved at each lexical value scope. */
static unsigned int alias_count;              /* Active prefix; retired block entries are removed. */
static int alias_scope;                       /* File=0, module=1, lexical body scopes>=2. */
static char alias_header[256];                /* Pending declaration name, independent of RHS capture. */
static char alias_target[256];                /* Pending RHS token; cleared at every declaration header. */
static SourceMark alias_target_origin;        /* Source position of the pending named edge. */
static SourceMark alias_origin;               /* Diagnostic location of the pending header. */


/* Last-write SSA hints and initialization facts; cell-backed hints are never
 * emitted directly. source_config resets the live translation-unit prefix. */
static char names[256][256];
static int bindings[256], immutable[256], name_count, source_line;


/* Retired lexical bindings are never reused during one bounded translation unit. */
static int binding_scope[256], active[256], scope_depth;


/* Debug owners are unique across sibling blocks, unlike lexical depth. */
static int debug_scopes[65], debug_scope_serial;
static int logic_serial;  /* Logical expression label identities, reset per translation. */
static int blocks_used;   /* Entry plus all terminal/logical CFG blocks, bounded by FIR_BLOCKS. */
static int branch_serial; /* Total terminal decisions; two new blocks per decision. */
static int branch_bindings[FIR_BLOCKS / 2][256]; /* Incoming SSA bindings, isolated per decision. */
static int branch_cells[FIR_BLOCKS / 2][256];    /* Incoming scalar-cell identities isolated per terminal arm. */
static int branch_names[FIR_BLOCKS / 2];         /* Saved binding prefix; sibling locals are never reused. */
static int result_real;             /* Interface return type, checked against the definition before expressions. */
static FirType table_values[1001];  /* Per-SSA static types in the bounded table profile. */
static FirType table_bindings[256]; /* Declared/inferred local types, reset for each translation. */
static int table_cells[256];        /* Per-binding typed cell identities, -1 until loop promotion. */
static int loop_serial;             /* Unique loop labels in the translation. */
static int loop_depth;              /* Active loop count, bounded with parser nesting. */
static int loop_id[65];             /* Active innermost transfer target IDs. */
static int loop_iteration[65];      /* Whether continue must pass through a cursor step. */
static int loop_continues[65];      /* At least one reachable continue edge to the cursor step. */
static int path_closed;             /* Current loop statement path already ends in a transfer. */
static int conditional_serial;      /* Unique structured loop-if labels. */
static int conditional_depth;       /* Active loop-if lexical join contexts. */
static int conditional_scope[65];   /* Outer lexical scope of each structured join. */
static unsigned char loop_initialized[65][256]; /* Entry initialization facts for outer bindings. */
static int loop_entry_cells[65][256];           /* Entry cell IDs; body-only cells cannot escape. */
static int loop_for_context[65];                /* Counted-for step context, -1 for while/snapshot loops. */
static int loop_names[65];                      /* Binding prefix present before each active loop body. */
static int terminal; /* The last emitted arm already terminates and needs no footer return. */


/* Saved incoming/true-arm environments for a structured choice. No runtime
 * owners are held here: IDs name private SSA payloads/cells in staged FIR. */
typedef struct
{
    int names;               /* Binding prefix visible before either arm. */
    int incoming[256];       /* Incoming initialized-value IDs or -1. */
    int incoming_cells[256]; /* Dominating typed cells or -1. */
    int yes[256];            /* True-arm exit value IDs. */
    int yes_cells[256];      /* True-arm exit cell identities. */
} SourceChoiceState;

static SourceChoiceState choices[FIR_BLOCKS / 2]; /* Bounded request-local snapshots, slot zero unused. */


/* One staged classic-for header step. Streams are owned until end/release. */
typedef struct
{
    FILE *step;                  /* Owned temporary FIR step body, NULL when the slot is free. */
    FILE *parent;                /* Borrowed original FIR destination, never closed here. */
    int names;                   /* Header binding prefix; declarations are forbidden in the step. */
    int incoming[256];           /* Source facts before staging; restored before the first body. */
    unsigned char required[256]; /* Uninitialized header reads requiring proof at every step edge. */
    unsigned char ready[256];    /* Intersection of initialization facts at incoming step edges. */
    SourceMark origin[256];      /* First deferred read span for a failing proof, never a runtime pointer. */
} SourceForContext;

static SourceForContext for_contexts[64]; /* Active nested header/body contexts. */
static int for_step_context = -1;         /* Current captured step, -1 in ordinary source code. */


/* Emit a typed operation after sticky source errors have been checked. */
static int table_operation(FirType type, const char *spelling, int a, int b);


/* Configure one translation unit; each process owns one generated parser. */
void source_config(FILE *out, int language)
{
    source_release();
    alias_count = 0;
    alias_total = 0;
    alias_scope = 0;
    alias_header[0] = 0;
    alias_target[0] = 0;
    location.line = 0;
    location.offset = -1;
    location.length = 0;
    source_line = 0;
    destination = out;
    profile = language;
    integer_mode = language == 4;
    serial = failed = name_count = scope_depth = debug_scope_serial = 0;
    debug_scopes[0] = 0;
    branch_serial = terminal = logic_serial = 0;
    blocks_used = 1;
    result_real = 0;
    loop_serial = loop_depth = 0;
    path_closed = conditional_serial = conditional_depth = 0;
    memset(table_cells, -1, sizeof(table_cells));
    memset(for_contexts, 0, sizeof(for_contexts));
    for_step_context = -1;
    memset(table_values, 0, sizeof(table_values));
    memset(table_bindings, 0, sizeof(table_bindings));
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


/* Lower numeric spelling in the selected homogeneous profile; Limbo real requires
 * decimal fraction/exponent literals so inferred integer arithmetic cannot be silently changed. */
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
        if (profile == FIR_LIMBO && (!strpbrk(text, ".eE") || (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))))
        {
            reject("Limbo real profile requires decimal real literals; integer/radix conversions are unsupported");
        }
        errno = 0;
        value = strtod(text, &end);
        if (negative)
        {
            value = -value;
        }
        /* Admit representable nonzero subnormals even when strtod sets ERANGE.
         * Silent underflow to zero and overflow remain source errors. */
        if (!text[0] || *end ||
            (errno && !(errno == ERANGE && value != 0.0 && value >= -DBL_MIN && value <= DBL_MIN)) || !isfinite(value))
        {
            reject("invalid finite binary64 numeric literal");
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
    int result;
    if (op == '&' || op == '|' || op == '^' || op == '<' || op == '>')
    {
        return source_integer_bits(left, right, op);
    }
    result = serial++;
    if (!integer_mode && op == '%')
    {
        reject("remainder requires the Limbo int source profile");
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
                : op == '%' ? "mod"
                            : "mul",
                integer_mode ? ((op == '/' || op == '%') ? ".signed" : ".wrap") : "",
                left,
                right);
    }
    if (!failed)
    {
        source_debug_event("binary", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Emit pure signed32 bits or an eager checked shift. Borrow SSA IDs left/right;
 * op selects &, |, ^, < or >. Return the new ID; sticky errors prevent output.
 * This is compile-time lowering only, with no runtime owner or implicit coercion. */
int source_integer_bits(int left, int right, int op)
{
    const char *spelling = op == '&'   ? "i32.and"
                           : op == '|' ? "i32.or"
                           : op == '^' ? "i32.xor"
                           : op == '<' ? "i32.shl.checked"
                                       : "i32.shr.checked";
    int result = serial++;
    if (op != '&' && op != '|' && op != '^' && op != '<' && op != '>')
    {
        reject("unknown homogeneous int bit operator");
    }
    if (profile != FIR_LIMBO || !integer_mode)
    {
        reject("bit operations require the homogeneous Limbo int profile");
    }
    if (serial > 1000)
    {
        reject("expression size limit exceeded");
    }
    if (!failed)
    {
        fprintf(destination, "    %%v%d: i32 = %s %%v%d, %%v%d\n", result, spelling, left, right);
        source_debug_event("binary", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Complement a borrowed int SSA value without signed C arithmetic or reevaluation.
 * Return xor(value,-1); compiler staging owns the generated constant and debug site. */
int source_integer_not(int value)
{
    int minus = serial++;
    if (profile != FIR_LIMBO || !integer_mode || serial > 1000)
    {
        reject("int complement requires the Limbo int profile within the expression limit");
    }
    if (!failed)
    {
        fprintf(destination, "    %%v%d: i32 = const.i32 -1\n", minus);
        source_debug_event("constant", minus, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return source_integer_bits(value, minus, '^');
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
    if (profile == FIR_LIMBO && (!strcmp(token, "real") || !strcmp(token, "type")))
    {
        reject("Limbo type keyword is not a binding name");
        return 0;
    }
    if (profile == FIR_LIMBO)
    {
        unsigned int alias;
        for (alias = 0; alias < alias_count; alias++)
        {
            if (!strcmp(token, aliases[alias].name))
            {
                reject("source-profile local cannot shadow a type alias");
                return 0;
            }
        }
    }
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


/* [GC:] Select a private retained cell for one copied source payload type.
 * Return T_VOID for scalar/unsupported inputs. No runtime object is read or
 * retained here; generated Dis MOVP instructions establish actual ownership. */
static FirType source_reference_cell(FirType type)
{
    switch (type)
    {
    case T_STRING:
        return T_STRING_CELL;
    case T_TABLE_BIG:
        return T_TABLE_CELL;
    case T_KEYS:
        return T_KEYS_CELL;
    case T_TABLE_KEY:
        return T_KEY_CELL;
    default:
        return T_VOID;
    }
}


/* Resolve borrowed binding name to its current SSA value or promoted typed-cell
 * load. Return zero on a sticky initialization/resource error. This emits staged
 * FIR/debug records only; it never reads runtime variables or acquires owners.
 * An unreadable counted-for step operand records a deferred all-edge obligation;
 * the staged load is published only when every incoming body/continue path proves it. */
int source_read(int name)
{
    int value;
    if (bindings[name] < 0)
    {
        if (for_step_context >= 0 && name < for_contexts[for_step_context].names && table_cells[name] >= 0)
        {
            SourceForContext *context = &for_contexts[for_step_context];
            if (!context->required[name])
            {
                context->origin[name] = location;
            }
            context->required[name] = 1;
        }
        else
        {
            reject("read before initialized declaration");
            return 0;
        }
    }
    value = bindings[name];
    if (!failed && table_cells[name] >= 0)
    {
        if (source_reference_cell(table_bindings[name]) != T_VOID)
        {
            value = table_operation(table_bindings[name], "ref.load", table_cells[name], -1);
        }
        else if (table_bindings[name] == T_BOOL)
        {
            value = table_operation(T_BOOL, "bool.load", table_cells[name], -1);
        }
        else
        {
            value = table_operation(T_I64, "big.load", table_cells[name], -1);
            if (table_bindings[name] == T_I32)
            {
                value = source_table_convert(value, 2);
            }
        }
    }
    if (!failed)
    {
        source_debug_access(
            "read", value, names[name], debug_scopes[binding_scope[name]], binding_scope[name], name + 1, location);
    }
    return value;
}


/* Allocate a local shadow after its initializer has resolved the outer binding.
 * name is a translation-local identity. Return the local identity, or the original
 * on a sticky resource error. Names and static types remain compiler-owned. */
static int local_declaration(int name)
{
    if (profile == FIR_LIMBO && binding_scope[name] < scope_depth)
    {
        if (name_count == 256)
        {
            reject("variable limit exceeded");
            return name;
        }
        memmove(names[name_count], names[name], sizeof(names[name_count]));
        name = name_count++;
        bindings[name] = -1;
        immutable[name] = 0;
        table_bindings[name] = T_VOID;
        table_cells[name] = -1;
        binding_scope[name] = scope_depth;
        active[name] = 1;
    }
    return name;
}


/* Bind declarations in their lexical owner; initialization reads the outer binding.
 * Nondeclaration writes keep targeting the nearest active outer binding. */
void source_bind(int name, int value, int declaration)
{
    if (declaration)
    {
        name = local_declaration(name);
    }
    if (declaration && bindings[name] >= 0)
    {
        reject("duplicate declaration");
    }
    if (!declaration && immutable[name])
    {
        reject("assignment to const binding");
    }
    if (!declaration && bindings[name] < 0 && table_bindings[name] == T_VOID)
    {
        reject("assignment before declaration");
    }
    bindings[name] = value;
    if (!failed)
    {
        source_debug_access(declaration ? "declare" : "assign",
                            value,
                            names[name],
                            debug_scopes[binding_scope[name]],
                            binding_scope[name],
                            name + 1,
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


/* Save a header before its RHS captures another scanner token; owner-only. */
void source_alias_header(void)
{
    strcpy(alias_header, token);
    alias_target[0] = 0;
    alias_origin = location;
}


/* Require a real module header even when the alias-prefix repetition is empty. */
void source_alias_module(int found)
{
    if (!found)
    {
        reject("source module declaration required");
    }
}


/* Select the file/module prefix; source_alias_body later establishes lexical scopes. */
void source_alias_scope(int module)
{
    alias_header[0] = 0;
    alias_scope = module != 0;
}


/* Find a borrowed name in the active lexical prefix; return -1 on a miss.
 * Search declaration identities only. No allocation, source scanning or mutation. */
static int source_alias_find(const char *name, int scope)
{
    unsigned int index = alias_count;
    while (index)
    {
        index--;
        if (aliases[index].scope <= scope && !strcmp(aliases[index].name, name))
        {
            return (int)index;
        }
    }
    return -1;
}


/* Resolve index through declaration-scoped edges without recursive C frames.
 * Return canonical type or T_VOID with a sticky diagnostic at the failing RHS.
 * Cache only successful paths; fixed work holds at most the bounded catalog size. */
static FirType source_alias_resolve(int index)
{
    unsigned int path[SOURCE_ALIAS_MAX]; /* Borrowed catalog identities on this path. */
    unsigned int count = 0;
    unsigned int prior;
    FirType type;
    while (aliases[index].type == T_VOID)
    {
        source_restore(aliases[index].origin);
        for (prior = 0; prior < count; prior++)
        {
            if (path[prior] == (unsigned int)index)
            {
                reject("cyclic source type alias");
                return T_VOID;
            }
        }
        if (count == SOURCE_ALIAS_MAX)
        {
            reject("source type alias resolution limit");
            return T_VOID;
        }
        path[count++] = (unsigned int)index;
        index = source_alias_find(aliases[index].target, aliases[index].scope);
        if (index < 0)
        {
            reject("unknown source type alias target");
            return T_VOID;
        }
    }
    type = aliases[index].type;
    while (count)
    {
        aliases[path[--count]].type = type;
    }
    return type;
}


/* Capture a named RHS edge before later scanner tokens replace it. Return T_VOID
 * as an unresolved marker, not an error. The complete prefix is validated later. */
FirType source_alias_target(void)
{
    strcpy(alias_target, token);
    alias_target_origin = location;
    return T_VOID;
}


/* Validate every declaration before body emission, including unused aliases.
 * Restore the caller's mark on success; failing RHS locations remain authoritative. */
void source_alias_validate(void)
{
    unsigned int index;
    SourceMark mark = location;
    for (index = 0; index < alias_count && !failed; index++)
    {
        source_alias_resolve((int)index);
    }
    if (!failed)
    {
        source_restore(mark);
    }
}


/* Query a copied scanner identifier for cast disambiguation. name is borrowed;
 * return boolean without consuming input, emitting IR or reporting a diagnostic. */
int source_alias_named(const char *name)
{
    return profile == FIR_LIMBO && source_alias_find(name, alias_scope) >= 0;
}


/* Resolve the current consumed type token; report a miss at that use site.
 * Successful queries preserve that authored token mark for lowering/debug records. */
FirType source_alias_type(void)
{
    SourceMark mark = location;
    FirType type;
    int index = source_alias_find(token, alias_scope);
    if (profile != FIR_LIMBO || index < 0)
    {
        reject("unknown source type alias");
        return T_VOID;
    }
    type = source_alias_resolve(index);
    if (!failed)
    {
        source_restore(mark);
    }
    return type;
}


/* Publish a canonical alias only after RHS success; duplicate/limit errors keep old bindings. */
void source_alias_declare(FirType type)
{
    unsigned int index;
    source_restore(alias_origin);
    if (failed)
    {
        alias_header[0] = 0;
        return;
    }
    if (!alias_scope && !strcmp(alias_header, module_name))
    {
        reject("file type alias conflicts with implementing module");
        alias_header[0] = 0;
        return;
    }
    for (index = 0; index < alias_count; index++)
    {
        if (aliases[index].scope == alias_scope && !strcmp(aliases[index].name, alias_header))
        {
            reject("duplicate source type alias");
            alias_header[0] = 0;
            return;
        }
    }
    if (alias_total == SOURCE_ALIAS_MAX)
    {
        reject("source type alias limit");
        alias_header[0] = 0;
        return;
    }
    strcpy(aliases[alias_count].name, alias_header);
    aliases[alias_count].type = type;
    strcpy(aliases[alias_count].target, alias_target);
    aliases[alias_count].origin = alias_target[0] ? alias_target_origin : alias_origin;
    aliases[alias_count++].scope = alias_scope;
    alias_total++;
    alias_header[0] = 0;
}


/* Enter the function's type environment after matching both result annotations.
 * No parameters or result. Keep prefix aliases alive; body declarations are sequential. */
void source_alias_body(void)
{
    alias_scope = 2;
    alias_floor[0] = alias_count;
}


/* Intern a pending statement head only after grammar distinguishes value from type.
 * Return the existing value-name ID; preserve the parser's current token and mark.
 * All names remain compiler-owned and sticky errors prevent staged publication. */
int source_alias_value_name(void)
{
    char current[256];
    SourceMark mark = location;
    int name;
    strcpy(current, token);
    strcpy(token, alias_header);
    source_restore(alias_origin);
    name = source_name();
    alias_header[0] = 0;
    strcpy(token, current);
    source_restore(mark);
    return name;
}


/* Declare a body-local alias at the current header. type is the parsed target;
 * allowed excludes for-header clauses. Resolve immediately so no local forward edge
 * escapes its lexical lifetime. Reject reserved spellings and same-scope values.
 * Return nothing; failure retains the exact header/RHS diagnostic and staged output. */
void source_alias_local(FirType type, int allowed)
{
    unsigned int before = alias_count;
    int name;
    source_restore(alias_origin);
    if (!allowed || alias_scope < 2)
    {
        reject("local type alias requires a body statement, not a for clause");
        return;
    }
    if (!strcmp(alias_header, "real") || !strcmp(alias_header, "type") || !strcmp(alias_header, "value"))
    {
        reject("source-profile type alias requires an ordinary identifier");
        return;
    }
    for (name = 0; name < name_count; name++)
    {
        if (active[name] && binding_scope[name] == scope_depth && !strcmp(names[name], alias_header))
        {
            reject("local type alias conflicts with a value in this scope");
            return;
        }
    }
    source_alias_declare(type);
    if (!failed && alias_count > before)
    {
        source_alias_resolve((int)before);
        if (!failed)
        {
            source_restore(alias_origin);
        }
    }
}


/* Resolve and map only executable tutorial result types; no new backend semantics. */
void source_result_alias(int declaration)
{
    FirType type = source_alias_type();
    if (type == T_I32 || type == T_F64 || type == T_I64)
    {
        source_result_type(type == T_I32 ? 0 : type == T_F64 ? 1 : 2, declaration);
    }
    else if (!failed)
    {
        reject("source result alias requires int, real or big");
    }
}


/* Preserve homogeneous local typing after alias canonicalization. */
void source_binding_alias(void)
{
    FirType type = source_alias_type();
    if (type == T_I32 || type == T_F64)
    {
        source_binding_type(type == T_F64);
    }
    else if (!failed)
    {
        reject("numeric local alias requires int or real");
    }
}


/* Admit only already implemented table-profile annotation kinds. */
FirType source_table_alias(void)
{
    FirType type = source_alias_type();
    if (type != T_I32 && type != T_I64 && type != T_STRING && type != T_TABLE_BIG && !failed)
    {
        reject("unsupported table-profile type alias");
        return T_VOID;
    }
    return type;
}


/* Lower a cast whose target was resolved before its operand. value is an SSA ID,
 * target is canonical and copied by the parser. Return an SSA ID or sticky error.
 * Numeric profiles admit identity casts only; table profiles reuse checked existing
 * int/big/string conversions. No object cloning, runtime RPC or new FIR operation. */
int source_alias_cast(int value, FirType target)
{
    if (source_table_profile())
    {
        if (target == T_I32 || target == T_I64 || target == T_STRING)
        {
            return source_table_convert(value, target == T_I32 ? 2 : target == T_STRING ? 1 : 0);
        }
        reject("source alias cast requires int, big or string in table profile");
    }
    else if (target != (integer_mode ? T_I32 : T_F64))
    {
        reject("numeric alias cast must match homogeneous result profile");
    }
    return value;
}


/* Reject a parsed source family that disagrees with the requested profile. */
void source_language(int limbo)
{
    if ((profile == FIR_LIMBO) != limbo)
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


/* Lower a captured local update. name/value are compiler IDs, op is zero for a
 * plain assignment, operation is the authored operator span. Keep read/write sites
 * at the saved target and arithmetic at its operator. No runtime RPC is issued. */
void source_assign_at(int name, int value, int op, SourceMark operation)
{
    SourceMark target = location;
    if (op)
    {
        int left = source_read(name);
        source_restore(operation);
        value = source_binary(left, value, op);
    }
    source_restore(target);
    source_bind(name, value, 0);
}


/* Preserve the existing unmarked AS caller contract; target location is borrowed. */
void source_assign(int name, int value, int op)
{
    source_assign_at(name, value, op, location);
}


/* Keep native Limbo comments from silently changing AS lexical coverage. */
void source_hash_comment(void)
{
    if (profile != FIR_LIMBO)
    {
        reject("# comments require Limbo profile");
    }
}


/* Enter a lexical Limbo owner; parser nesting independently bounds recursion. */
void source_enter_scope(void)
{
    ++scope_depth;
    alias_floor[scope_depth] = alias_count;
    alias_scope = 2 + scope_depth;
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
    alias_count = alias_floor[scope_depth];
    --scope_depth;
    alias_scope = 2 + scope_depth;
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


/* Release owned step streams/deferred origins after every translation, including
 * failure; retire the borrowed root output without closing the caller stream. */
void source_release(void)
{
    int context;
    for (context = 0; context < 64; context++)
    {
        if (for_contexts[context].step)
        {
            fclose(for_contexts[context].step);
            for_contexts[context].step = NULL;
        }
    }
    for_step_context = -1;
    source_debug_defer_release();
    destination = NULL;
}


/* Lower one admitted Limbo relation to exact int/big or ordered real comparison.
 * Operands/relation are parser values. Return owns no storage; source errors remain sticky.
 * The current restored source mark identifies the comparison operator, not a later operand. */
SourceCondition source_compare(int left, int right, SourceRelation relation)
{
    SourceCondition condition;
    int swap = relation == SOURCE_GREATER || relation == SOURCE_LESS_EQUAL;
    int equality = relation == SOURCE_EQUAL || relation == SOURCE_NOT_EQUAL;
    condition.value = serial++;
    condition.invert =
        relation == SOURCE_NOT_EQUAL || relation == SOURCE_LESS_EQUAL || relation == SOURCE_GREATER_EQUAL;
    if (profile != FIR_LIMBO || serial > 1000)
    {
        reject("terminal comparisons require a bounded Limbo numeric profile");
    }
    if (result_real == 2 && !failed)
    {
        if (table_values[left] != T_I64 || table_values[right] != T_I64)
        {
            reject("terminal big comparison requires two big operands");
        }
        else
        {
            table_values[condition.value] = T_BOOL;
        }
    }
    if (result_real == 1)
    {
        /* Inverting < for >= would accept unordered values. Keep <= explicit,
         * swap only already evaluated operands, and invert equality only for !=. */
        swap = relation == SOURCE_GREATER || relation == SOURCE_GREATER_EQUAL;
        condition.invert = relation == SOURCE_NOT_EQUAL;
    }
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: bool = %s.%s %%v%d, %%v%d\n",
                condition.value,
                result_real == 2   ? "big"
                : result_real == 1 ? "f64"
                                   : "i32",
                equality           ? "eq"
                : result_real == 1 ? (relation == SOURCE_LESS_EQUAL || relation == SOURCE_GREATER_EQUAL ? "le" : "lt")
                                   : "lt.signed",
                swap ? right : left,
                swap ? left : right);
        source_debug_event("compare", condition.value, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return condition;
}


/* Publish the two edges and first label of a terminal decision; return its scalar identity.
 * condition is a copied bool/inversion pair. Bounds leave room for the entry plus two blocks
 * per decision. No assignment environment is merged: each accepted arm must return. */
int source_branch(SourceCondition condition)
{
    int choice = ++branch_serial;
    if (branch_serial > (FIR_BLOCKS - 1) / 2 || blocks_used + 2 > FIR_BLOCKS)
    {
        reject("terminal decision block limit exceeded");
    }
    if (!failed)
    {
        blocks_used += 2;
        branch_names[choice] = name_count;
        memcpy(branch_bindings[choice], bindings, (size_t)name_count * sizeof(bindings[0]));
        memcpy(branch_cells[choice], table_cells, (size_t)name_count * sizeof(table_cells[0]));
        fprintf(destination,
                "    if %%v%d, ^choose%d_%s, ^choose%d_%s\n^choose%d_yes:\n",
                condition.value,
                choice,
                condition.invert ? "no" : "yes",
                choice,
                condition.invert ? "yes" : "no",
                choice);
        source_debug_event("branch", condition.value, "", debug_scopes[scope_depth], scope_depth, location);
    }
    terminal = 0;
    return choice;
}


/* Begin the source-false arm with the decision's original SSA environment.
 * choice is parser-local. The true arm has terminated and retired its lexical
 * locals. Restore outer scalar/owner bindings, not mutable table contents: effects
 * execute only in the selected runtime arm. No phi or continuation is introduced. */
void source_else(int choice)
{
    if (!failed)
    {
        memcpy(bindings, branch_bindings[choice], (size_t)branch_names[choice] * sizeof(bindings[0]));
        memcpy(table_cells, branch_cells[choice], (size_t)branch_names[choice] * sizeof(table_cells[0]));
        fprintf(destination, "^choose%d_no:\n", choice);
    }
    terminal = 0;
}


/* Return the tutorial value through its existing trace harness, then terminate this FIR arm.
 * value is a parser-owned SSA index. No caller stream or source binding is acquired. */
void source_return(int value)
{
    source_trace(value);
    if (!failed)
    {
        fputs("    return\n", destination);
    }
    terminal = 1;
}


/* Finish one staged module. AS statements need a footer return; every Limbo result already has one.
 * No parameters; destination remains caller-owned until source_release retires the borrow. */
void source_finish(void)
{
    if (!terminal)
    {
        fputs("    return\n", destination);
    }
    fputs("}\n", destination);
}


/* Select a homogeneous Limbo result profile and require interface/definition agreement.
 * kind selects int=0, real=1 or big/table=2; declaration selects the interface versus definition.
 * No implicit conversion, output publication or allocation is performed. */
void source_result_type(int kind, int declaration)
{
    if (declaration)
    {
        result_real = kind;
    }
    else if (result_real != kind)
    {
        reject("Limbo interface and definition result types differ");
    }
    integer_mode = !kind;
}


/* Validate a typed local against the already selected homogeneous Limbo profile.
 * real is a parser boolean. Mismatch is sticky and prevents all output publication. */
void source_binding_type(int real)
{
    if (real == integer_mode)
    {
        reject("mixed int/real bindings require explicit conversion support");
    }
}


/* Return provenance after the generated parser has selected its result type.
 * No arguments or ownership transfer; the host serializes source compiler calls. */
FirSemantics source_semantics(void)
{
    return profile == FIR_LIMBO ? (result_real == 2 ? FIR_LIMBO_TABLES
                                   : integer_mode   ? FIR_LIMBO_I32
                                                    : FIR_LIMBO_REAL)
                                : FIR_AVM1_NUMERIC;
}


/* Query the explicit big-result profile selected by the grammar; no ownership or side effects. */
int source_table_profile(void)
{
    return result_real == 2;
}


/* Allocate a bounded typed SSA identity. Failure is sticky; callers never publish failed output. */
static int table_value(FirType type)
{
    int value = serial++;
    if (serial > 1000)
    {
        reject("table expression size limit exceeded");
        return 0;
    }
    table_values[value] = type;
    return value;
}


/* Emit one typed unary/binary table operation with copied source provenance. */
static int table_operation(FirType type, const char *spelling, int a, int b)
{
    static const char *const types[] = {
        "void",        "i32",          "f64",        "bool",        "string",     "any",       "channel",
        "i64",         "table.big",    "table.slot", "big.range",   "keys",       "table.key", "bool.slot",
        "table.range", "string.range", "i64.slot",   "string.cell", "table.cell", "keys.cell", "key.cell",
    };
    int result = table_value(type);
    if (!failed)
    {
        fprintf(destination, "    %%v%d: %s = %s", result, types[type], spelling);
        if (a >= 0)
        {
            fprintf(destination, " %%v%d", a);
        }
        if (b >= 0)
        {
            fprintf(destination, ", %%v%d", b);
        }
        fputc('\n', destination);
        source_debug_event("table-operation", result, spelling, debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Lower an exact decimal big constant; fractions, overflow and unsupported radices fail explicitly. */
int source_table_constant(int negative)
{
    char text[258];
    char *end;
    intmax_t number;
    int result = table_value(T_I64);
    snprintf(text, sizeof(text), "%s%s", negative ? "-" : "", token);
    errno = 0;
    number = strtoimax(text, &end, 10);
    if (errno || *end || number < INT64_MIN || number > INT64_MAX)
    {
        reject("table profile requires an exact decimal big literal");
    }
    if (!failed)
    {
        fprintf(destination, "    %%v%d: i64 = const.i64 %" PRId64 "\n", result, (int64_t)number);
        source_debug_event("constant", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Lower a quoted string or literal dotted member name into the same FIR string-key type. */
int source_table_text(int quoted)
{
    int result = table_value(T_STRING);
    if (!quoted && strlen(token) > 252)
    {
        reject("dotted table key exceeds FIR text limit");
    }
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: string = const.string %s%s%s\n",
                result,
                quoted ? "" : "\"",
                token,
                quoted ? "" : "\"");
        source_debug_event("constant", result, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Emit a checked int/big shift from borrowed SSA values. right must be int;
 * direction selects '<' or '>'. FIR retains the source width and eager count error. */
int source_table_shift(int left, int right, int direction)
{
    FirType kind = table_values[left];
    if ((kind != T_I64 && kind != T_I32) || table_values[right] != T_I32)
    {
        reject("shift requires int/big value and explicit int count");
    }
    return table_operation(kind,
                           kind == T_I32 ? (direction == '<' ? "i32.shl.checked" : "i32.shr.checked")
                                         : (direction == '<' ? "big.shl" : "big.shr"),
                           left,
                           right);
}


/* Create a fresh mutable table owner; no source binding is acquired here. */
int source_table_new(void)
{
    return table_operation(T_TABLE_BIG, "table.new", -1, -1);
}


/* Emit a constructor's nonnegative positional key without borrowing token text.
 * position is a parser-local sequence index. Return a fresh big SSA identity;
 * sticky resource errors prevent publication. No table owner is allocated;
 * output is written to the translation-owned staged FIR/debug streams. */
int source_table_position(int position)
{
    int result = table_value(T_I64);
    if (!failed)
    {
        fprintf(destination, "    %%v%d: i64 = const.i64 %d\n", result, position);
        source_debug_event("constant", result, "constructor-index", debug_scopes[scope_depth], scope_depth, location);
    }
    return result;
}


/* Count all entries of a borrowed typed table; sequence length remains separate. */
int source_table_count(int table)
{
    if (table_values[table] != T_TABLE_BIG)
    {
        reject("countof requires table of big");
    }
    return table_operation(T_I64, "table.count", table, -1);
}


/* Preserve the typed owner/key capture and return a normalized membership bool.
 * Numeric bounds and snapshot capabilities remain eager runtime checks.
 */
int source_table_has(int table, int key)
{
    int slot = source_table_index(table, key);
    return table_operation(T_BOOL, "table.has", slot, -1);
}


/* Remove the captured entry without evaluating a missing-key load or shifting indices.
 * The bool result distinguishes a removed entry from an already absent entry.
 */
int source_table_delete(int slot)
{
    if (table_values[slot] != T_TABLE_SLOT)
    {
        reject("delete requires a captured table location");
    }
    return table_operation(T_BOOL, "table.delete", slot, -1);
}


/* Begin a lazy logical expression using a private scalar merge cell.
 * left is an evaluated bool; disjunction selects || rather than &&. No owner,
 * heap allocation or source binding escapes. The skipped arm keeps left, while
 * only the RHS arm can overwrite it. All labels share the global block budget. */
SourceLogic source_logic_begin(int left, int disjunction)
{
    SourceLogic logic;
    logic.cell = 0;
    logic.choice = ++logic_serial;
    if (failed)
    {
        return logic;
    }
    if (table_values[left] != T_BOOL)
    {
        reject("logical operator requires bool operands");
    }
    if (blocks_used + 3 > FIR_BLOCKS)
    {
        reject("logical expression block limit exceeded");
    }
    if (!failed)
    {
        blocks_used += 3;
        logic.cell = table_operation(T_BOOL_SLOT, "bool.local", -1, -1);
        if (failed)
        {
            return logic;
        }
        fprintf(destination, "    bool.store %%v%d, %%v%d\n", logic.cell, left);
        fprintf(destination,
                "    if %%v%d, ^logic%d_%s, ^logic%d_%s\n"
                "^logic%d_skip:\n    jump ^logic%d_join\n^logic%d_rhs:\n",
                left,
                logic.choice,
                disjunction ? "skip" : "rhs",
                logic.choice,
                disjunction ? "rhs" : "skip",
                logic.choice,
                logic.choice,
                logic.choice);
        source_debug_event("logical-branch", left, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return logic;
}


/* Merge the executed RHS or skipped left into a fresh immutable bool SSA result.
 * logic is the parser's copied continuation; right has evaluated only in its
 * guarded block. Synthetic stores/loads keep the authored operator provenance.
 * Errors preserve staged outputs and never index an invalid cell identity. */
int source_logic_end(SourceLogic logic, int right)
{
    if (failed)
    {
        return 0;
    }
    if (table_values[right] != T_BOOL)
    {
        reject("logical operator requires bool operands");
        return 0;
    }
    fprintf(destination,
            "    bool.store %%v%d, %%v%d\n    jump ^logic%d_join\n^logic%d_join:\n",
            logic.cell,
            right,
            logic.choice,
            logic.choice);
    source_debug_event("logical-store", right, "", debug_scopes[scope_depth], scope_depth, location);
    return table_operation(T_BOOL, "bool.load", logic.cell, -1);
}


/* Negate a borrowed normalized bool without widening or reevaluating its producer.
 * value is an SSA identity; return a fresh bool identity. Invalid source kinds
 * make staged publication fail. No owner or native resource is acquired here. */
int source_table_not(int value)
{
    if (failed)
    {
        return 0;
    }
    if (table_values[value] != T_BOOL)
    {
        reject("logical negation requires bool; use an explicit comparison");
        return 0;
    }
    return table_operation(T_BOOL, "bool.not", value, -1);
}


/* Materialize a source comparison with Inferno precedence and exact operand types.
 * left/right are evaluated SSA values, relation is a copied parser enum. Same-type
 * int operands compare directly; strings admit scalar ordering, bool equality only.
 * Inverted relations use normalized scalar negation, never extra owner reads
 * or duplicate expressions. */
int source_table_relation(int left, int right, SourceRelation relation)
{
    FirType kind;
    SourceCondition condition;
    int equality = relation == SOURCE_EQUAL || relation == SOURCE_NOT_EQUAL;
    if (failed)
    {
        return 0;
    }
    kind = table_values[left];
    if (kind == T_STRING && table_values[right] == T_STRING)
    {
        int swap = relation == SOURCE_GREATER || relation == SOURCE_LESS_EQUAL;
        int invert = relation == SOURCE_NOT_EQUAL || relation == SOURCE_LESS_EQUAL || relation == SOURCE_GREATER_EQUAL;
        int compared =
            table_operation(T_BOOL, equality ? "string.eq" : "string.lt", swap ? right : left, swap ? left : right);
        return invert ? source_table_not(compared) : compared;
    }
    if (kind != table_values[right] || (kind != T_I64 && kind != T_I32 && !(kind == T_BOOL && equality)))
    {
        reject("comparison requires same-type big/int operands or bool equality");
    }
    if (!failed && (kind == T_I32 || kind == T_BOOL))
    {
        int swap = relation == SOURCE_GREATER || relation == SOURCE_LESS_EQUAL;
        int invert = relation == SOURCE_NOT_EQUAL || relation == SOURCE_LESS_EQUAL || relation == SOURCE_GREATER_EQUAL;
        int compared = table_operation(T_BOOL,
                                       kind == T_BOOL ? "bool.eq"
                                       : equality     ? "i32.eq"
                                                      : "i32.lt.signed",
                                       swap ? right : left,
                                       swap ? left : right);
        return invert ? source_table_not(compared) : compared;
    }
    condition = source_compare(left, right, relation);
    if (failed)
    {
        return 0;
    }
    return condition.invert ? source_table_not(condition.value) : condition.value;
}


/* Use a borrowed normalized bool as a terminal condition, without reevaluation. */
SourceCondition source_table_truth(int value)
{
    SourceCondition condition;
    condition.value = value;
    condition.invert = 0;
    if (table_values[value] != T_BOOL)
    {
        reject("table condition requires a comparison or membership bool");
    }
    return condition;
}


/* Capture a typed owner/key location. Both numeric and text syntax use this single operation. */
int source_table_index(int table, int key)
{
    if (table_values[table] != T_TABLE_BIG ||
        (table_values[key] != T_I64 && table_values[key] != T_STRING && table_values[key] != T_TABLE_KEY))
    {
        reject("table indexing requires table of big and a numeric/string/snapshot key");
    }
    return table_operation(T_TABLE_SLOT, "table.index", table, key);
}


/* Capture a snapshot key, table value or Unicode scalar from typed SSA operands.
 * String indexes require int; table/snapshot numeric keys remain big. Runtime
 * bounds remain eager effects, and all results retain the restored access span. */
int source_table_subscript(int receiver, int key)
{
    if (table_values[receiver] == T_STRING)
    {
        if (table_values[key] != T_I32)
        {
            reject("string index requires explicit int");
        }
        return table_operation(T_I32, "string.at", receiver, key);
    }
    if (table_values[receiver] == T_KEYS)
    {
        if (table_values[key] != T_I64)
        {
            reject("key snapshot indexing requires big");
        }
        return table_operation(T_TABLE_KEY, "keys.at", receiver, key);
    }
    return source_table_load(source_table_index(receiver, key));
}


/* Read a captured table location as big; runtime handles an absent key explicitly. */
int source_table_load(int slot)
{
    return table_operation(T_I64, "table.load", slot, -1);
}


/* Return native string int length or big table/snapshot length from the static type.
 * Strings count Unicode scalars, not bytes; no owner content is mutated. */
int source_table_len(int value)
{
    if (table_values[value] == T_STRING)
    {
        return table_operation(T_I32, "string.len", value, -1);
    }
    if (table_values[value] != T_TABLE_BIG && table_values[value] != T_KEYS)
    {
        reject("len requires string, table or key snapshot in this profile");
    }
    return table_operation(T_I64, table_values[value] == T_KEYS ? "keys.len" : "table.len", value, -1);
}


/* Lower one compound update after capturing its old value and evaluating RHS.
 * current/right are borrowed SSA identities; op selects an existing typed operation
 * or checked shift. Return a same-type int/big/string identity. No owner is
 * reacquired, expression reevaluated or store published here; the parser commits
 * only after success. */
int source_table_update(int current, int right, int op)
{
    if (failed)
    {
        return 0;
    }
    if (op == '<' || op == '>')
    {
        return source_table_shift(current, right, op);
    }
    return source_table_binary(current, right, op);
}


/* Emit arithmetic after evaluating both operands once in source order. Inputs
 * are borrowed SSA identities; incompatible types make output publication fail. */
int source_table_binary(int left, int right, int op)
{
    const char *spelling = "big.add";
    if (failed)
    {
        return 0;
    }
    if (table_values[left] == T_STRING && table_values[right] == T_STRING && op == '+')
    {
        return table_operation(T_STRING, "string.concat", left, right);
    }
    if (table_values[left] == T_I32 && table_values[right] == T_I32)
    {
        /* Typed int operands already carry exact signed32 values. Emit one pure
         * operation instead of two widenings, a big operation and narrowing. */
        if (op == '&' || op == '|' || op == '^')
        {
            return table_operation(T_I32, op == '&' ? "i32.and" : op == '|' ? "i32.or" : "i32.xor", left, right);
        }
        switch (op)
        {
        case '+':
            spelling = "i32.add.wrap";
            break;
        case '-':
            spelling = "i32.sub.wrap";
            break;
        case '*':
            spelling = "i32.mul.wrap";
            break;
        case '/':
            spelling = "i32.div.signed";
            break;
        case '%':
            spelling = "i32.mod.signed";
            break;
        default:
            reject("int arithmetic supports +, -, *, /, %, &, | and ^ in this profile");
            return 0;
        }
        return table_operation(T_I32, spelling, left, right);
    }
    if (table_values[left] != T_I64 || table_values[right] != T_I64)
    {
        reject("big arithmetic requires two big operands");
    }
    switch (op)
    {
    case '+':
        spelling = "big.add";
        break;
    case '-':
        spelling = "big.sub";
        break;
    case '*':
        spelling = "big.mul";
        break;
    case '/':
        spelling = "big.div";
        break;
    case '%':
        spelling = "big.mod";
        break;
    case '&':
        spelling = "big.and";
        break;
    case '|':
        spelling = "big.or";
        break;
    case '^':
        spelling = "big.xor";
        break;
    default:
        reject("unsupported big arithmetic operator");
        break;
    }
    return table_operation(T_I64, spelling, left, right);
}


/* Emit typed unary +/-/~ on a borrowed int/big SSA input. The parser has restored
 * the exact token span; no implicit string conversion or owner call occurs. */
int source_table_unary(int value, int op)
{
    if (failed)
    {
        return 0;
    }
    if (table_values[value] == T_I32)
    {
        int zero;
        if (op == '+')
        {
            return table_operation(T_I32, "copy", value, -1);
        }
        if (op == '~')
        {
            int mask = table_value(T_I32);
            if (!failed)
            {
                fprintf(destination, "    %%v%d: i32 = const.i32 -1\n", mask);
                source_debug_event("constant", mask, "", debug_scopes[scope_depth], scope_depth, location);
            }
            return table_operation(T_I32, "i32.xor", value, mask);
        }
        if (op != '-')
        {
            reject("int unary arithmetic supports +, - and ~");
            return 0;
        }
        zero = table_value(T_I32);
        if (!failed)
        {
            fprintf(destination, "    %%v%d: i32 = const.i32 0\n", zero);
            source_debug_event("constant", zero, "", debug_scopes[scope_depth], scope_depth, location);
        }
        return table_operation(T_I32, "i32.sub.wrap", zero, value);
    }
    if (table_values[value] != T_I64)
    {
        reject("big unary arithmetic requires big");
    }
    if (op != '+' && op != '-' && op != '~')
    {
        reject("unsupported big unary operator");
    }
    return table_operation(T_I64, op == '~' ? "big.not" : op == '-' ? "big.neg" : "copy", value, -1);
}


/* Explicitly convert borrowed int/big/text/key SSA values. target selects big=0, text=1, int=2.
 * Int narrowing is total modulo 2^32; decimal/key checks remain runtime errors. No implicit owner RPC. */
int source_table_convert(int value, int target)
{
    FirType type = table_values[value];
    if (target == 2)
    {
        if (type == T_STRING || type == T_TABLE_KEY)
        {
            /* Parse the complete signed64 decimal/key value once, before total
             * signed32 narrowing. Existing conversion errors remain observable. */
            value = source_table_convert(value, 0);
            return table_operation(T_I32, "value.int", value, -1);
        }
        if (type != T_BOOL && type != T_I32 && type != T_I64)
        {
            reject("int conversion requires int, big, string or snapshot key");
        }
        return table_operation(T_I32, "value.int", value, -1);
    }
    if (type != T_BOOL && type != T_I32 && type != T_I64 && type != T_STRING && type != T_TABLE_KEY)
    {
        reject("conversion requires int, big, string or snapshot key");
    }
    return table_operation(target ? T_STRING : T_I64, target ? "value.text" : "value.big", value, -1);
}


/* Reflect supported typed values without converting data or invoking owner methods.
 * A snapshot key remains dynamic: the VM must inspect its retained immutable owner. */
int source_table_typeof(int value)
{
    FirType type = table_values[value];
    if (type != T_BOOL && type != T_I32 && type != T_I64 && type != T_STRING && type != T_TABLE_BIG && type != T_KEYS &&
        type != T_TABLE_KEY)
    {
        reject("typeof requires a supported table-profile value");
    }
    return table_operation(T_STRING, "value.type", value, -1);
}


/* Copy all keys without exporting the mutable table representation. */
int source_table_keys(int table)
{
    if (table_values[table] != T_TABLE_BIG)
    {
        reject("keysof requires table of big");
    }
    return table_operation(T_KEYS, "table.keys", table, -1);
}


/* Build checked half-open bounds for a table copy or immutable Unicode string slice.
 * String bounds are int and widen exactly into the existing range representation. */
int source_table_slice(int table, int first, int end)
{
    int range;
    if (table_values[table] == T_STRING)
    {
        if (table_values[first] != T_I32 || table_values[end] != T_I32)
        {
            reject("string slice requires explicit int bounds");
            return 0;
        }
        first = source_table_convert(first, 0);
        end = source_table_convert(end, 0);
        range = table_operation(T_BIG_RANGE, "range.big", first, end);
        return table_operation(T_STRING, "string.slice", table, range);
    }
    if (table_values[table] != T_TABLE_BIG || table_values[first] != T_I64 || table_values[end] != T_I64)
    {
        reject("table slice requires numeric bounds");
    }
    range = table_operation(T_BIG_RANGE, "range.big", first, end);
    return table_operation(T_TABLE_BIG, "table.slice", table, range);
}


/* Capture a borrowed table/string and signed bounds before evaluating assignment RHS.
 * The runtime resolves relative positions eagerly; no raw address escapes. */
int source_table_range(int table, int first, int end)
{
    int range;
    if (table_values[table] == T_STRING)
    {
        if (table_values[first] != T_I32 || table_values[end] != T_I32)
        {
            reject("string slice assignment requires int rune bounds");
            return 0;
        }
        first = source_table_convert(first, 0);
        end = source_table_convert(end, 0);
        range = table_operation(T_BIG_RANGE, "range.big", first, end);
        return table_operation(T_STRING_RANGE, "string.range", table, range);
    }
    if (table_values[table] != T_TABLE_BIG || table_values[first] != T_I64 || table_values[end] != T_I64)
    {
        reject("slice assignment requires table of big and big bounds");
        return 0;
    }
    range = table_operation(T_BIG_RANGE, "range.big", first, end);
    return table_operation(T_TABLE_RANGE, "table.range", table, range);
}


/* Replace a captured table/string range with a borrowed same-kind RHS.
 * Return -1 after table mutation or a fresh string SSA identity for local rebind.
 * Native storage handles self aliasing; source type errors publish no operation. */
int source_table_splice(int slot, int value)
{
    if (table_values[slot] == T_STRING_RANGE)
    {
        if (table_values[value] != T_STRING)
        {
            reject("string slice replacement requires string");
            return -1;
        }
        return table_operation(T_STRING, "string.splice", slot, value);
    }
    if (table_values[slot] != T_TABLE_RANGE || table_values[value] != T_TABLE_BIG)
    {
        reject("slice replacement requires table of big");
    }
    if (!failed)
    {
        fprintf(destination, "    table.splice %%v%d, %%v%d\n", slot, value);
        source_debug_event("table-splice", slot, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return -1;
}


/* Type-check an explicit write; do not erase or reorder its memory effect. */
void source_table_store(int slot, int value)
{
    if (table_values[slot] != T_TABLE_SLOT || table_values[value] != T_I64)
    {
        reject("table of big rejects a non-big value");
    }
    if (!failed)
    {
        fprintf(destination, "    table.store %%v%d, %%v%d\n", slot, value);
        source_debug_event("table-store", slot, "", debug_scopes[scope_depth], scope_depth, location);
    }
}


/* Declare/infer or assign a typed local, preserving uninitialized-declaration rejection on reads.
 * type is zero for inferred declarations/assignments; value -1 means a typed declaration only. */
void source_table_bind(int name, int value, int type, int declaration)
{
    FirType actual = value >= 0 ? table_values[value] : (FirType)type;
    if (declaration && for_step_context >= 0)
    {
        reject("for step cannot declare bindings");
        return;
    }
    if (declaration)
    {
        name = local_declaration(name);
    }
    if (declaration && table_bindings[name])
    {
        reject("duplicate table-profile declaration");
    }
    if (!declaration && !table_bindings[name])
    {
        reject("table-profile assignment before declaration");
    }
    if ((type && actual != (FirType)type) || (!declaration && table_bindings[name] != actual))
    {
        reject("table-profile binding type mismatch");
    }
    if (!failed)
    {
        table_bindings[name] = actual;
        if (declaration && value < 0)
        {
            source_debug_access(
                "declare", -1, names[name], debug_scopes[binding_scope[name]], binding_scope[name], name + 1, location);
        }
        if (value >= 0)
        {
            source_bind(name, value, declaration);
            if (table_cells[name] >= 0)
            {
                const char *operation = "big.store";
                if (source_reference_cell(actual) != T_VOID)
                {
                    operation = "ref.store";
                }
                else if (actual == T_BOOL)
                {
                    operation = "bool.store";
                }
                else if (actual == T_I32)
                {
                    value = source_table_convert(value, 0);
                }
                fprintf(destination, "    %s %%v%d, %%v%d\n", operation, table_cells[name], value);
            }
        }
    }
}


/* Require a big result before emitting the existing trace/return entry harness. */
void source_table_return(int value)
{
    if (table_values[value] != T_I64)
    {
        reject("table profile value() must return big");
    }
    source_return(value);
}


/* Diagnose unsupported compound range writes; never generate a scalar read. */
void source_table_slice_compound(void)
{
    reject("compound assignment to slice is unsupported");
}


/* Test a borrowed receiver's static type; no runtime effect or ownership change. */
int source_string_receiver(int receiver)
{
    return table_values[receiver] == T_STRING;
}


/* Capture a string scalar write destination before evaluating its int RHS.
 * Negative indexes resolve against the current immutable string snapshot. */
int source_string_char_range(int receiver, int index)
{
    if (table_values[receiver] != T_STRING || table_values[index] != T_I32)
    {
        reject("string scalar assignment requires int rune index");
        return 0;
    }
    return table_operation(T_STRING_RANGE, "string.char.range", receiver, index);
}


/* Validate an int Unicode scalar and allocate replacement text; errors are eager. */
int source_string_rune(int value)
{
    if (table_values[value] != T_I32)
    {
        reject("string scalar replacement requires int Unicode scalar");
        return 0;
    }
    return table_operation(T_STRING, "rune.text", value, -1);
}


/* Emit one retained table ownership boundary for a borrowed typed SSA receiver.
 * receiver is evaluated once; acquire selects entry rather than leave. Strings
 * retain value semantics and emit nothing. No runtime wait occurs in this C
 * compiler; generated acquisition may suspend or throw in the owning Dis VM. */
void source_table_lease(int receiver, int acquire)
{
    if (!failed && table_values[receiver] == T_TABLE_BIG)
    {
        fprintf(destination, "    table.%s %%v%d\n", acquire ? "lock" : "unlock", receiver);
        source_debug_event(
            acquire ? "table-lock" : "table-unlock", receiver, "", debug_scopes[scope_depth], scope_depth, location);
    }
}


/* [GC:] Promote active locals into dominating typed frame cells.
 * include_uninitialized reserves zero/nil storage for declared locals at joins,
 * without marking them readable. Scalars keep bool/big storage; int widens on
 * store and narrows on load. Initialized reference cells hold an
 * independent rooted owner. Cells never escape this serial compiler request's
 * function; unsupported transient locations/ranges remain SSA-only. */
static void source_promote_locals(int include_uninitialized)
{
    int name;
    for (name = 0; name < name_count && !failed; name++)
    {
        FirType type = table_bindings[name];
        FirType reference = source_reference_cell(type);
        if (active[name] && (type == T_I64 || type == T_I32 || type == T_BOOL || reference != T_VOID) &&
            (bindings[name] >= 0 || include_uninitialized) && table_cells[name] < 0)
        {
            if (reference != T_VOID)
            {
                table_cells[name] =
                    table_operation(reference, bindings[name] < 0 ? "ref.nil" : "ref.local", bindings[name], -1);
            }
            else
            {
                int cell = table_operation(
                    type == T_BOOL ? T_BOOL_SLOT : T_BIG_SLOT, type == T_BOOL ? "bool.local" : "big.local", -1, -1);
                int value = bindings[name];
                table_cells[name] = cell;
                if (type == T_I32 && value >= 0)
                {
                    value = source_table_convert(value, 0);
                }
                if (!failed && value >= 0)
                {
                    fprintf(
                        destination, "    %s %%v%d, %%v%d\n", type == T_BOOL ? "bool.store" : "big.store", cell, value);
                }
            }
        }
    }
}


/* Begin a while continuation before parsing its condition. Reserve typed cells
 * for all active declarations, including unreadable zero/nil storage. Body writes
 * then dominate later reads within that iteration without exporting exit facts.
 * Return the unique loop ID. Sticky limits prevent publication;
 * no runtime storage is allocated by this compiler function. */
int source_loop_begin(void)
{
    int loop = ++loop_serial;
    if (loop_depth >= 64 || blocks_used + 3 > FIR_BLOCKS)
    {
        reject("loop nesting/block limit exceeded");
        return loop;
    }
    source_promote_locals(1);
    loop_id[loop_depth] = loop;
    loop_for_context[loop_depth] = -1;
    loop_iteration[loop_depth] = loop_continues[loop_depth] = 0;
    {
        int name;
        for (name = 0; name < name_count; name++)
        {
            loop_initialized[loop_depth][name] = bindings[name] >= 0;
            loop_entry_cells[loop_depth][name] = table_cells[name];
        }
    }
    loop_names[loop_depth++] = name_count;
    path_closed = 0;
    blocks_used += 3;
    if (!failed)
    {
        fprintf(destination, "    jump ^loop%d_test\n^loop%d_test:\n", loop, loop);
        source_debug_event("while-test", -1, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return loop;
}


/* Branch from a while test to its statement body or continuation. loop is the
 * parser-owned ID; condition is a checked bool/inversion pair. No bindings are
 * merged: loop-carried values reside in their dominating scalar cells. */
void source_loop_condition(int loop, SourceCondition condition)
{
    if (!failed)
    {
        fprintf(destination,
                "    if %%v%d, ^loop%d_%s, ^loop%d_%s\n^loop%d_body:\n",
                condition.value,
                loop,
                condition.invert ? "end" : "body",
                loop,
                condition.invert ? "body" : "end",
                loop);
        source_debug_event("while-branch", condition.value, "", debug_scopes[scope_depth], scope_depth, location);
    }
}


/* Retire one loop's compiler context without promoting body-only initialization.
 * No arguments or runtime work. Restore uninitialized entry bindings and their
 * dominating cell IDs; a potentially empty body cannot export a new cell/value. */
static void source_loop_leave(void)
{
    if (loop_depth)
    {
        int name;
        int depth = --loop_depth;
        for (name = 0; name < loop_names[depth]; name++)
        {
            if (!loop_initialized[depth][name])
            {
                bindings[name] = -1;
                table_cells[name] = loop_entry_cells[depth][name];
            }
        }
    }
}


/* Close a while body after its lexical locals have retired. Emit the backedge
 * and continuation for loop, then pop one parser loop context. No native mutex
 * spans the backedge unless explicit FIR ownership has been balanced there. */
void source_loop_end(int loop)
{
    if (!failed)
    {
        if (!path_closed)
        {
            fprintf(destination, "    jump ^loop%d_test\n", loop);
        }
        fprintf(destination, "^loop%d_end:\n", loop);
        source_debug_event("while-end", -1, "", debug_scopes[scope_depth], scope_depth, location);
    }
    source_loop_leave();
    path_closed = 0;
}


/* Capture one borrowed table/string/key sequence and traverse its stable extent.
 * Tables snapshot keys and read live big payloads; immutable strings yield int
 * rune indexes/values; key sequences yield big indexes and retained key values.
 * Return compiler-owned loop/cursor IDs. Type, allocation and nesting errors are
 * sticky. VM roots retain captured owners without exposing their addresses. */
SourceIteration source_iteration_begin(int table)
{
    SourceIteration iteration;
    SourceCondition condition;
    int keys;
    int length;
    int index;
    int zero;
    FirType type = table_values[table];
    if (type != T_TABLE_BIG && type != T_STRING && type != T_KEYS)
    {
        reject("for-in requires table of big, string or key snapshot");
    }
    keys = type == T_TABLE_BIG ? table_operation(T_KEYS, "table.keys", table, -1) : table;
    /* Captured strings/key sequences are immutable: compute their extent once. */
    length = source_table_len(keys);
    if (type == T_STRING)
    {
        length = source_table_convert(length, 0);
    }
    zero = source_table_position(0);
    iteration.cell = table_operation(T_BIG_SLOT, "big.local", -1, -1);
    if (!failed)
    {
        fprintf(destination, "    big.store %%v%d, %%v%d\n", iteration.cell, zero);
    }
    iteration.loop = source_loop_begin();
    if (!failed)
    {
        if (blocks_used + 1 > FIR_BLOCKS)
        {
            reject("for-in cursor block limit exceeded");
        }
        else
        {
            blocks_used++;
            loop_iteration[loop_depth - 1] = 1;
        }
    }
    index = table_operation(T_I64, "big.load", iteration.cell, -1);
    condition.value = table_operation(T_BOOL, "big.lt.signed", index, length);
    condition.invert = 0;
    source_loop_condition(iteration.loop, condition);
    if (type == T_TABLE_BIG)
    {
        iteration.key = table_operation(T_TABLE_KEY, "keys.at", keys, index);
        iteration.value = source_table_load(source_table_index(table, iteration.key));
    }
    else
    {
        iteration.key = type == T_STRING ? source_table_convert(index, 2) : index;
        iteration.value = source_table_subscript(keys, iteration.key);
    }
    return iteration;
}


/* Finish one snapshot iteration after lexical bindings have retired. iteration
 * is a copied compiler continuation. Cursor arithmetic uses exact modulo-64 big
 * operations; the finite snapshot length bounds all reachable cursor values. */
void source_iteration_end(SourceIteration iteration)
{
    if (!failed && (!path_closed || loop_continues[loop_depth - 1]))
    {
        int index;
        int one;
        int next;
        if (!path_closed)
        {
            fprintf(destination, "    jump ^loop%d_step\n", iteration.loop);
        }
        fprintf(destination, "^loop%d_step:\n", iteration.loop);
        path_closed = 0;
        index = table_operation(T_I64, "big.load", iteration.cell, -1);
        one = source_table_position(1);
        next = table_operation(T_I64, "big.add", index, one);
        if (!failed)
        {
            fprintf(destination, "    big.store %%v%d, %%v%d\n", iteration.cell, next);
        }
    }
    source_loop_end(iteration.loop);
}


/* Bind iteration values after scope entry; key=-1 omits the optional key name.
 * iteration/key/value are compiler IDs; marks identify authored names. A single
 * value keeps its existing big/rune/key type, without a synthetic source binding.
 * Reject duplicate pair names before declarations. No runtime work occurs. */
void source_iteration_bind(SourceIteration iteration, int key, int value, SourceMark keymark, SourceMark valuemark)
{
    if (key == value)
    {
        reject("for-in requires distinct key and value names");
    }
    if (key >= 0)
    {
        source_restore(keymark);
        source_table_bind(key, iteration.key, 0, 1);
    }
    source_restore(valuemark);
    source_table_bind(value, iteration.value, 0, 1);
}


/* Admit one new function/block/loop statement on a still-open compiler CFG path. This
 * translation-only check preserves the first error; it emits no runtime code. */
void source_statement(void)
{
    if (!failed && path_closed)
    {
        reject("unreachable statement after path-closing transfer");
    }
}


/* Close the current path with an innermost-loop jump. is_continue selects the
 * test (while) or cursor step (for-in); otherwise jump to loop exit. IDs remain
 * compiler-owned. Invalid context/closed path is a sticky source error. */
void source_loop_transfer(int is_continue)
{
    int active_loop;
    const char *target;
    source_statement();
    if (!loop_depth)
    {
        reject("break/continue requires an enclosing loop");
    }
    if (failed)
    {
        return;
    }
    active_loop = loop_depth - 1;
    target = is_continue ? (loop_iteration[active_loop] ? "step" : "test") : "end";
    if (is_continue)
    {
        int context = loop_for_context[active_loop];
        int name;
        loop_continues[active_loop] = 1;
        if (context >= 0)
        {
            for (name = 0; name < for_contexts[context].names; name++)
            {
                for_contexts[context].ready[name] &= bindings[name] >= 0;
            }
        }
    }
    fprintf(destination, "    jump ^loop%d_%s\n", loop_id[active_loop], target);
    source_debug_event(is_continue ? "continue" : "break", -1, "", debug_scopes[scope_depth], scope_depth, location);
    path_closed = 1;
}


/* Prepare one structured conditional before its condition executes. Promote
 * initialized locals, snapshot incoming IDs and reserve true/false blocks.
 * Return a copied parser choice. Limits/errors are sticky; no VM/GC runs here. */
SourceChoice source_choice_begin(void)
{
    SourceChoice choice;
    SourceChoiceState *state;
    choice.choice = ++conditional_serial;
    choice.yes_closed = 0;
    if (failed)
    {
        return choice;
    }
    if (conditional_depth >= 64 || choice.choice >= FIR_BLOCKS / 2 || blocks_used + 2 > FIR_BLOCKS)
    {
        reject("conditional nesting/block limit exceeded");
        return choice;
    }
    source_promote_locals(1);
    state = &choices[choice.choice];
    state->names = name_count;
    memcpy(state->incoming, bindings, (size_t)name_count * sizeof(bindings[0]));
    memcpy(state->incoming_cells, table_cells, (size_t)name_count * sizeof(table_cells[0]));
    conditional_scope[conditional_depth++] = scope_depth;
    blocks_used += 2;
    return choice;
}


/* Emit normalized condition edges and open the true arm of a copied choice.
 * condition and choice are parser values. Initialized outer cells carry branch
 * writes; authored if spans remain stable. No runtime value is evaluated here. */
void source_choice_test(SourceChoice choice, SourceCondition condition)
{
    if (!failed)
    {
        fprintf(destination,
                "    if %%v%d, ^loopif%d_%s, ^loopif%d_%s\n^loopif%d_yes:\n",
                condition.value,
                choice.choice,
                condition.invert ? "no" : "yes",
                choice.choice,
                condition.invert ? "yes" : "no",
                choice.choice);
        source_debug_event(
            loop_depth ? "loop-if" : "branch", condition.value, "", debug_scopes[scope_depth], scope_depth, location);
    }
    path_closed = 0;
}


/* Finish a borrowed choice's true arm, save its exit and restore incoming IDs
 * before opening the false arm. Closed paths need no edge to the eventual join.
 * Only compiler arrays are copied; actual payload mutation happens in the VM. */
void source_choice_else(SourceChoice *choice)
{
    choice->yes_closed = path_closed;
    if (!failed)
    {
        SourceChoiceState *state = &choices[choice->choice];
        size_t bytes = (size_t)state->names * sizeof(bindings[0]);
        memcpy(state->yes, bindings, bytes);
        memcpy(state->yes_cells, table_cells, bytes);
        memcpy(bindings, state->incoming, bytes);
        memcpy(table_cells, state->incoming_cells, bytes);
        if (!path_closed)
        {
            fprintf(destination, "    jump ^loopif%d_join\n", choice->choice);
        }
        fprintf(destination, "^loopif%d_no:\n", choice->choice);
    }
    path_closed = 0;
}


/* Merge open exits of a copied choice and reserve a join only when reachable.
 * One surviving arm contributes its initialization facts. Two open arms meet
 * those facts and use the same dominating typed cell. No SSA value from one
 * predecessor is emitted on the other path. Terminal sibling environments stay
 * independent. Errors discard the entire staged request, including debug data. */
void source_choice_end(SourceChoice choice)
{
    int both_closed = choice.yes_closed && path_closed;
    if (!failed)
    {
        SourceChoiceState *state = &choices[choice.choice];
        size_t bytes = (size_t)state->names * sizeof(bindings[0]);
        int name;
        if (!choice.yes_closed && !path_closed)
        {
            for (name = 0; name < state->names; name++)
            {
                /* Both open paths must initialize an incoming uninitialized local.
                 * The value ID is a last-write hint only; reads use its common cell. */
                bindings[name] = state->incoming[name] >= 0                     ? state->incoming[name]
                                 : state->yes[name] >= 0 && bindings[name] >= 0 ? state->yes[name]
                                                                                : -1;
                table_cells[name] = state->incoming_cells[name];
            }
        }
        else if (!choice.yes_closed)
        {
            memcpy(bindings, state->yes, bytes);
            memcpy(table_cells, state->yes_cells, bytes);
        }
        else if (both_closed)
        {
            memcpy(bindings, state->incoming, bytes);
            memcpy(table_cells, state->incoming_cells, bytes);
        }
        if (!both_closed)
        {
            if (blocks_used + 1 > FIR_BLOCKS)
            {
                reject("conditional join block limit exceeded");
            }
            else
            {
                blocks_used++;
            }
        }
        if (!failed && !path_closed)
        {
            fprintf(destination, "    jump ^loopif%d_join\n", choice.choice);
        }
        if (!failed && !both_closed)
        {
            fprintf(destination, "^loopif%d_join:\n", choice.choice);
        }
    }
    path_closed = both_closed;
    if (conditional_depth)
    {
        conditional_depth--;
    }
}


/* Return a borrowed big SSA result from any structured function path. The ordinary function
 * return emitter owns trace/type rules; mark this path closed so loop/if cleanup
 * emits no trailing edge. No native owner escapes and no partial statement runs. */
void source_flow_return(int value)
{
    source_statement();
    source_table_return(value);
    if (!failed)
    {
        source_debug_event(
            loop_depth ? "loop-return" : "return", value, "", debug_scopes[scope_depth], scope_depth, location);
    }
    path_closed = 1;
}


/* Require every reachable function exit to return a big value. No parameters,
 * owners or runtime calls. A potentially zero-iteration loop never proves total
 * return by itself; a missing return rejects staged FIR/debug publication. */
void source_flow_finish(void)
{
    if (!failed && !path_closed)
    {
        reject("function may finish without returning big");
    }
    if (!failed)
    {
        terminal = 1;
    }
}


/* Allocate a compiler-private step stream after initializer emission. Return a
 * copied parser continuation; failures keep context=-1 and prevent publication.
 * The enclosing source_release closes any stream left by syntax-error recovery. */
SourceFor source_for_begin(void)
{
    SourceFor control;
    int context;
    control.loop = 0;
    control.context = -1;
    if (failed)
    {
        return control;
    }
    control.loop = source_loop_begin();
    if (failed)
    {
        return control;
    }
    if (blocks_used + 1 > FIR_BLOCKS)
    {
        reject("for step block limit exceeded");
        return control;
    }
    for (context = 0; context < 64 && for_contexts[context].step; context++)
    {
    }
    if (context == 64)
    {
        reject("for step context limit exceeded");
        return control;
    }
    for_contexts[context].parent = destination;
    for_contexts[context].step = fir_temporary();
    if (!for_contexts[context].step)
    {
        reject("cannot allocate staged for step");
        return control;
    }
    control.context = context;
    loop_for_context[loop_depth - 1] = context;
    for_contexts[context].names = name_count;
    memset(for_contexts[context].required, 0, sizeof(for_contexts[context].required));
    memset(for_contexts[context].ready, 1, sizeof(for_contexts[context].ready));
    loop_iteration[loop_depth - 1] = 1;
    blocks_used++;
    return control;
}


/* Emit a constant bool for an omitted for condition. Return a copied typed
 * parser condition; no runtime allocation or implicit numeric truthiness occurs. */
SourceCondition source_for_true(void)
{
    SourceCondition condition;
    condition.value = table_operation(T_BOOL, "const.bool true", -1, -1);
    condition.invert = 0;
    return condition;
}


/* Capture a borrowed continuation's header step rather than executing it before
 * the body. Only one header can be captured at a time; nested bodies own separate
 * retained streams. Debug records keep authored source marks in their own stream. */
void source_for_step_begin(SourceFor control)
{
    if (failed || control.context < 0)
    {
        return;
    }
    if (for_step_context >= 0)
    {
        reject("nested for header capture is unsupported");
        return;
    }
    memcpy(for_contexts[control.context].incoming,
           bindings,
           (size_t)for_contexts[control.context].names * sizeof(bindings[0]));
    for_step_context = control.context;
    source_debug_defer_begin(control.context);
    destination = for_contexts[control.context].step;
}


/* Restore emission after a parsed header even when semantic validation failed.
 * control is parser-owned; do not close the retained stream until end/release.
 * Restore header-entry facts so staged writes cannot initialize the first body. */
void source_for_step_end(SourceFor control)
{
    if (control.context >= 0)
    {
        SourceForContext *context = &for_contexts[control.context];
        destination = context->parent;
        memcpy(bindings, context->incoming, (size_t)context->names * sizeof(bindings[0]));
    }
    for_step_context = -1;
    source_debug_defer_end();
}


/* Emit the buffered step only when normal fallthrough or continue reaches it.
 * control is a copied parser continuation. Release its owned stream on every
 * path, never close the borrowed parent. Intersect ordinary fallthrough with
 * captured continue facts before admitting deferred header reads. Missing proofs
 * report the original read mark; unreachable steps need no proof or debug sites.
 * Read/write/close failures are sticky source errors; the outer API discards
 * staged FIR/debug output atomically. */
void source_for_end(SourceFor control)
{
    SourceForContext *context;
    char buffer[4096];
    size_t count;
    int reachable;
    if (control.context < 0)
    {
        return;
    }
    context = &for_contexts[control.context];
    destination = context->parent;
    reachable = !failed && (!path_closed || loop_continues[loop_depth - 1]);
    if (reachable)
    {
        int name;
        for (name = 0; name < context->names && !failed; name++)
        {
            if (!path_closed)
            {
                context->ready[name] &= bindings[name] >= 0;
            }
            if (context->required[name] && !context->ready[name])
            {
                source_restore(context->origin[name]);
                reject("for step read requires initialization on every incoming path");
            }
        }
    }
    if (reachable && !failed)
    {
        if (!path_closed)
        {
            fprintf(destination, "    jump ^loop%d_step\n", control.loop);
        }
        fprintf(destination, "^loop%d_step:\n", control.loop);
        if (fflush(context->step) || fseek(context->step, 0, SEEK_SET))
        {
            reject("cannot rewind staged for step");
        }
        while (!failed && (count = fread(buffer, 1, sizeof(buffer), context->step)) != 0)
        {
            if (fwrite(buffer, 1, count, destination) != count)
            {
                reject("cannot emit staged for step");
            }
        }
        if (ferror(context->step))
        {
            reject("cannot read staged for step");
        }
        if (!failed)
        {
            fprintf(destination, "    jump ^loop%d_test\n", control.loop);
        }
    }
    source_debug_defer_flush(control.context, reachable && !failed);
    if (!failed)
    {
        fprintf(destination, "^loop%d_end:\n", control.loop);
        source_debug_event("for-end", -1, "", debug_scopes[scope_depth], scope_depth, location);
    }
    if (fclose(context->step))
    {
        reject("cannot close staged for step");
    }
    context->step = NULL;
    source_loop_leave();
    path_closed = 0;
}


/* Compute a statement-only postfix big/int update from borrowed SSA value/direction.
 * Return a new same-type SSA value, preserving modular arithmetic and source marks.
 * No value is returned to a source expression and no runtime owner is acquired. */
int source_table_increment(int value, int direction)
{
    int one;
    if (table_values[value] != T_I64 && table_values[value] != T_I32)
    {
        reject("increment/decrement requires big or int local or table entry");
        return 0;
    }
    one = source_table_position(1);
    if (table_values[value] == T_I32)
    {
        one = source_table_convert(one, 2);
    }
    return source_table_update(value, one, direction > 0 ? '+' : '-');
}
