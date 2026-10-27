/* C99 numeric source lowering; parser actions are declared in lower.h. */
#include "lower.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
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
/* Flat translation-unit bindings. source_config resets the live prefix. */
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
static int branch_names[FIR_BLOCKS / 2];         /* Saved binding prefix; sibling locals are never reused. */
static int result_real;             /* Interface return type, checked against the definition before expressions. */
static FirType table_values[1001];  /* Per-SSA static types in the bounded table profile. */
static FirType table_bindings[256]; /* Declared/inferred local types, reset for each translation. */
static int terminal;                /* The last emitted arm already terminates and needs no footer return. */

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
    branch_serial = terminal = logic_serial = 0;
    blocks_used = 1;
    result_real = 0;
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
        if (!text[0] || *end || errno || !isfinite(value))
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
    int result = serial++;
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
    if (profile == FIR_LIMBO && !strcmp(token, "real"))
    {
        reject("real is a Limbo type keyword, not a binding name");
        return 0;
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

/* Resolve the nearest active binding to its current SSA definition. */
int source_read(int name)
{
    if (bindings[name] < 0)
    {
        reject("read before initialized declaration");
        return 0;
    }
    if (!failed)
    {
        source_debug_access("read",
                            bindings[name],
                            names[name],
                            debug_scopes[binding_scope[name]],
                            binding_scope[name],
                            name + 1,
                            location);
    }
    return bindings[name];
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
    if (!declaration && bindings[name] < 0)
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
    if (profile != FIR_LIMBO)
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

/* Lower one admitted Limbo relation to signed i32/big comparison plus edge inversion.
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
    if ((!integer_mode && result_real != 2) || serial > 1000)
    {
        reject("terminal comparisons require the bounded Limbo int or big profile");
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
    if (!failed)
    {
        fprintf(destination,
                "    %%v%d: bool = %s.%s %%v%d, %%v%d\n",
                condition.value,
                result_real == 2 ? "big" : "i32",
                equality ? "eq" : "lt.signed",
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
    static const char *const types[] = {"void",
                                        "i32",
                                        "f64",
                                        "bool",
                                        "string",
                                        "any",
                                        "channel",
                                        "i64",
                                        "table.big",
                                        "table.slot",
                                        "big.range",
                                        "keys",
                                        "table.key",
                                        "bool.slot"};
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

/* Emit a checked long shift from borrowed SSA values. left must be big, right int;
 * direction selects '<' or '>'. Count range remains a runtime error, including unused results. */
int source_table_shift(int left, int right, int direction)
{
    if (table_values[left] != T_I64 || table_values[right] != T_I32)
    {
        reject("big shift requires big value and explicit int count");
    }
    return table_operation(T_I64, direction == '<' ? "big.shl" : "big.shr", left, right);
}

/* Create a fresh mutable table owner; no source binding is acquired here. */
int source_table_new(void)
{
    return table_operation(T_TABLE_BIG, "table.new", -1, -1);
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

/* Negate a borrowed bool with existing scalar FIR operations. The zero is a
 * compiler-owned big literal, not a parsed token or runtime owner. Return a new
 * normalized bool identity; invalid operands prevent staged output publication. */
int source_table_not(int value)
{
    int number;
    int zero;
    if (failed)
    {
        return 0;
    }
    if (table_values[value] != T_BOOL)
    {
        reject("logical negation requires bool; use an explicit comparison");
    }
    number = source_table_convert(value, 0);
    zero = table_value(T_I64);
    if (!failed)
    {
        fprintf(destination, "    %%v%d: i64 = const.i64 0\n", zero);
        source_debug_event("constant", zero, "", debug_scopes[scope_depth], scope_depth, location);
    }
    return table_operation(T_BOOL, "big.eq", number, zero);
}

/* Materialize a source comparison with Inferno precedence and exact operand types.
 * left/right are evaluated SSA values, relation is a copied parser enum. Same-type
 * int operands widen exactly; bool admits equality only. Inverted relations use
 * normalized scalar negation, never extra owner reads or duplicate expressions. */
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
    if (kind != table_values[right] || (kind != T_I64 && kind != T_I32 && !(kind == T_BOOL && equality)))
    {
        reject("comparison requires same-type big/int operands or bool equality");
    }
    if (!failed && kind != T_I64)
    {
        left = source_table_convert(left, 0);
        right = source_table_convert(right, 0);
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
 * current/right are borrowed SSA identities; op selects an existing big operation
 * or checked shift. Return a new same-type int/big identity. No owner is reacquired, expression
 * reevaluated or store published here; the parser commits only after success. */
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
    if (table_values[left] == T_I32 && table_values[right] == T_I32)
    {
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
            reject("int arithmetic supports only +, -, *, / and % in this profile");
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

/* Emit typed unary +/-/~ on a borrowed big SSA input. The parser has restored
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
        if (op != '-')
        {
            reject("int unary arithmetic supports only + and -");
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
        if (type != T_BOOL && type != T_I32 && type != T_I64)
        {
            reject("int conversion requires int or big");
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
        if (value >= 0)
        {
            source_bind(name, value, bindings[name] < 0);
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
