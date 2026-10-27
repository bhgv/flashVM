/* FIR 1.0: standalone C99, no FlashVM headers or private ABI dependency. */
#ifndef FIR_H
#define FIR_H

/* SWO is an octet format. Numeric lowering stores i32 in a normal C int and
 * the object backend writes IEEE binary64 bits; reject incompatible targets. */
#include <limits.h>
#include <float.h>

#if CHAR_BIT != 8 || INT_MAX < 2147483647
#error "FIR requires 8-bit bytes and at least 32-bit int"
#endif
#if FLT_RADIX != 2 || DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024
#error "FIR SWO requires an IEEE binary64 double target"
#endif

/* Reject known mixed-word-order targets: memcpy into uint64_t would swap words. */
#if defined(__FLOAT_WORD_ORDER__) && defined(__BYTE_ORDER__)
#if __FLOAT_WORD_ORDER__ != __BYTE_ORDER__
#error "FIR SWO requires matching floating/integer word order"
#endif
#endif
#include <stdio.h>
#include <stdint.h>


/* C99 compile-time assertion: binary64 payload copying must read exactly 8 bytes. */
typedef char FirDoubleMustBeEightBytes[(sizeof(double) == 8) ? 1 : -1];
#ifdef __cplusplus
extern "C"
{
#endif

/* Initial hard bounds; exceeding them is a diagnostic, never truncation. */
#define FIR_LIMIT 4096
#define FIR_BLOCKS 256
#define FIR_TEXT 256


/* Optional source provenance; this is not a claim of complete language support. */
typedef enum
{
    FIR_SOURCE_UNSPECIFIED = 0,
    FIR_AS1,
    FIR_AS2,
    FIR_AS3,
    FIR_LIMBO
} FirLanguage;

typedef enum
{
    FIR_SEM_UNSPECIFIED = 0,
    FIR_AVM1_NUMERIC,
    FIR_LIMBO_I32,
    FIR_LIMBO_REAL,  /* Homogeneous real numeric tutorial; no implicit integer conversions. */
    FIR_LIMBO_TABLES /* Typed big-table source profile; no implicit dynamic conversions. */
} FirSemantics;

typedef enum
{
    T_VOID,
    T_I32,
    T_F64,
    T_BOOL,
    T_STRING,
    T_ANY,
    T_CHANNEL,
    T_I64,         /* Exact signed Limbo big. */
    T_TABLE_BIG,   /* Owner reference to a mutable big-valued hybrid table. */
    T_TABLE_SLOT,  /* Checked table/key location, not a native address. */
    T_BIG_RANGE,   /* Half-open big bounds for a sequence slice. */
    T_KEYS,        /* Independent immutable key sequence snapshot. */
    T_TABLE_KEY,   /* Snapshot owner plus checked position, preserving numeric/text identity. */
    T_BOOL_SLOT,   /* Function-local scalar bool cell; no pointer, copy or escape. */
    T_TABLE_RANGE, /* Captured table owner and absolute exclusive slice bounds. */
    T_STRING_RANGE /* Captured immutable string and absolute rune bounds. */
} FirType;

typedef enum
{
    O_I32,
    O_F64,
    O_BOOL,
    O_STRING,
    O_COPY,
    O_ADD,
    O_SUB,
    O_MUL,
    O_LT,
    O_EQ,
    O_FADD,
    O_FSUB,
    O_FMUL,
    O_FDIV,
    O_ASADD,
    O_NUMBER,
    O_CHANNEL,
    O_RECEIVE,
    O_TRACE,
    O_SEND,
    O_CLOSE,
    O_IF,
    O_JUMP,
    O_RETURN,
    O_I64,               /* Exact big constant. */
    O_TABLE_NEW,         /* Allocate an empty table. */
    O_TABLE_INDEX,       /* Capture table plus numeric/string key without loading. */
    O_TABLE_LOAD,        /* Read a captured location; missing raises. */
    O_TABLE_STORE,       /* Store big at a captured location. */
    O_TABLE_LEN,         /* Contiguous numeric prefix length. */
    O_BIG_RANGE,         /* Construct checked slice bounds. */
    O_TABLE_SLICE,       /* Copy a sequence slice, reindexed from zero. */
    O_TABLE_KEYS,        /* Copy all keys in deterministic order. */
    O_KEYS_LEN,          /* Count every copied key. */
    O_KEYS_AT,           /* Capture one checked immutable snapshot key. */
    O_VALUE_TYPE,        /* Reflect a supported value; snapshot keys report their runtime key kind. */
    O_TO_BIG,            /* Exact explicit conversion from big/text/snapshot key. */
    O_TO_TEXT,           /* Copied explicit conversion from big/text/snapshot key. */
    O_BIG_ADD,           /* Exact modulo-2^64 addition. */
    O_BIG_SUB,           /* Exact modulo-2^64 subtraction. */
    O_BIG_MUL,           /* Exact modulo-2^64 multiplication. */
    O_BIG_NEG,           /* Exact modulo-2^64 unary negation. */
    O_BIG_DIV,           /* Signed truncating division; zero raises, MIN/-1 wraps. */
    O_BIG_MOD,           /* Signed remainder; zero raises, MIN%-1 is zero. */
    O_BIG_AND,           /* Exact full-width bit intersection. */
    O_BIG_OR,            /* Exact full-width bit union. */
    O_BIG_XOR,           /* Exact full-width exclusive-or. */
    O_BIG_NOT,           /* Exact full-width complement. */
    O_BIG_LT,            /* Signed big less-than, returning bool. */
    O_BIG_EQ,            /* Full-width big equality, returning bool. */
    O_TO_INT,            /* Explicit low-32-bit signed int normalization from int/big. */
    O_BIG_SHL,           /* Checked left shift: big value, int count 0..63. */
    O_BIG_SHR,           /* Checked arithmetic right shift: big value, int count 0..63. */
    O_TABLE_COUNT,       /* Number of all numeric and string entries, independently of sequence length. */
    O_TABLE_HAS,         /* Test a captured location without raising for an absent key. */
    O_TABLE_DELETE,      /* Delete a captured location without numeric compaction; bool reports presence. */
    O_BOOL_LOCAL,        /* Define a zero-initialized function-frame bool cell. */
    O_BOOL_STORE,        /* Store a normalized bool in a dominating local cell. */
    O_BOOL_LOAD,         /* Read a local cell into a new immutable bool SSA value. */
    O_STRING_LEN,        /* Count Unicode scalars in a string, returning int. */
    O_STRING_AT,         /* Read one Unicode scalar at an int index, returning int. */
    O_STRING_SLICE,      /* Copy a half-open string range without changing the source. */
    O_I32_DIV,           /* Signed truncating int division; zero raises, MIN/-1 wraps. */
    O_I32_MOD,           /* Signed int remainder; zero raises, MIN%-1 is zero. */
    O_STRING_CONCAT,     /* Concatenate rooted UTF-8 strings; allocates, may throw. */
    O_STRING_EQ,         /* Compare string contents without implicit conversion. */
    O_I32_SHL,           /* Checked int left shift; count 0..31, result wraps to signed32. */
    O_I32_SHR,           /* Checked arithmetic int right shift; count 0..31. */
    O_STRING_LT,         /* Unicode scalar lexicographic ordering without normalization. */
    O_TABLE_RANGE,       /* Capture owner and normalize a range before evaluating RHS. */
    O_TABLE_SPLICE,      /* Transactional resized replacement through a captured range. */
    O_STRING_RANGE,      /* Capture immutable source and normalize rune bounds before RHS. */
    O_STRING_SPLICE,     /* Build an independent resized string from a captured range. */
    O_STRING_CHAR_RANGE, /* Capture one checked rune index as an absolute range. */
    O_RUNE_TEXT,         /* Convert a valid int Unicode scalar to a one-rune string. */
    O_TABLE_LOCK,        /* Retain process ownership across destination/RHS evaluation. */
    O_TABLE_UNLOCK       /* Release one process-owned retained table level. */
} FirOp;


/* Conservative operation effects. Unknown opcodes have every flag set. */
enum
{
    FIR_READS_MEMORY = 1,
    FIR_WRITES_MEMORY = 2,
    FIR_ALLOCATES = 4,
    FIR_MAY_THROW = 8,
    FIR_MAY_CALL_USER = 16,
    FIR_MAY_SUSPEND = 32,
    FIR_ALL_EFFECTS = 63
};

unsigned fir_effects(FirOp op);


/* Operands are symbol-table indices; -1 means absent. Source lines survive
     * optimisation. Strings are UTF-8 bytes without embedded NUL. */
typedef struct
{
    FirOp op;        /* Exact operation: effects and target lowering dispatch on this tag. */
    FirType type;
    int result;      /* SSA result index, or -1 for an instruction without a result. */
    int a, b;        /* Operand value indices; unused operands remain -1. */
    int yes, no;     /* Block-table indices for jump/conditional successors. */
    int block;       /* Containing block-table index used by dominance analysis. */
    int line;        /* Physical source line retained for diagnostics and dumps. */
    int removed;     /* Optimizer tombstone; printers/backends omit this instruction. */
    int32_t integer; /* Signed, fixed-width integer constant payload. */
    int64_t big;     /* Exact signed payload for O_I64, independent of host long width. */
    double real;     /* Floating constant payload; SWO encodes its binary64 bits. */
    char text[FIR_TEXT];
} FirInstruction;


/* Symbol-table entry: printable spelling, defining instruction index and type. */
typedef struct
{
    char name[FIR_TEXT];
    int definition;
    FirType type;
} FirValue;


/* Label entry: forward references are resolved before verification. */
typedef struct
{
    char name[FIR_TEXT];
    int defined;
} FirBlock;

typedef struct
{
    FirInstruction instructions[FIR_LIMIT];
    FirValue values[FIR_LIMIT];
    FirBlock blocks[FIR_BLOCKS];
    int count, value_count, block_count, channels;
    FirLanguage source_language;
    FirSemantics source_semantics;
    char module[FIR_TEXT];
} FirModule;


/* All functions return 1 on success and 0 with a diagnostic on failure.
     * The caller owns the module, streams and diagnostic buffer. The generated
     * parser has global state: sequential calls reset state; parsing is not thread-safe. */



/* Read and construct one FIR translation unit from a binary input stream. */
int fir_parse(FILE *input, FirModule *module, char *error, size_t capacity);



/* Reject malformed control flow, invalid SSA uses, duplicate symbol spellings and incompatible types. */
int fir_verify(FirModule *module, char *error, size_t capacity);



/* Verify caller-owned IR and reject cyclic CFG. No module mutation, allocation or parser state.
 * Returns 1 for a verified DAG, 0 with an optional bounded diagnostic otherwise. */
int fir_acyclic(FirModule *module, char *error, size_t capacity);



/* Print canonical, reparsable FIR text; keep diagnostic line annotations. */
int fir_print(FILE *output, const FirModule *module);



/* Apply conservative rewrites in place and verify the result. */
int fir_optimise(FirModule *module, char *error, size_t capacity);



/* Compile verified FIR into a versioned SWO1 object, never a SWF container. */
int fir_object(FILE *output, FirModule *module, char *error, size_t capacity);



/* Emit verified i32/bool/f64/i64 CFG or the supported acyclic big-table CFG as NC64 Dis.
 * Streams/module/error are caller-owned; emission needs no SDK/runtime. Coverage errors write nothing.
 * N trace values occupy the first N eight-byte cells; the next N cells mark executed slots as 0/1.
 * Numeric slots retain their latest value if revisited. Table images import the checked hosted Bigtable
 * ABI and carry precise frame/data GC maps; the scalar CFG path has no imports. No public exports or SBL.
 * This is experimental NC64 magic 819264, not classic Dis or complete Limbo module ABI support. */
int fir_dis(FILE *output, FirModule *module, char *error, size_t capacity);



/* Optional caller-owned temporary stream; honors TMPDIR on POSIX. */
FILE *fir_temporary(void);


/* Non-console entry points; caller owns every stream and diagnostic buffer.
     * Parser calls must be serialized; sequential calls reset state after errors. */


typedef enum
{
    FIR_CHECK,
    FIR_PRINT,
    FIR_OPT,
    FIR_OBJECT,
    FIR_DIS /* Direct, verified scalar-CFG NC64 capture harness. */
} FirTask;

typedef int (*FirSnapshot)(void *user, const char *phase, const FirModule *module);

typedef struct
{
    int optimise;
    FirSnapshot snapshot;
    void *user;
} FirRunOptions;


/* First source failure, copied into caller storage. Locations use original
     * UTF-8 bytes: zero-based offset/length, one-based physical line/byte column.
     * LF alone starts a line. No location means all location fields are zero. */
typedef enum
{
    FIR_DIAG_NONE = 0,
    FIR_DIAG_ARGUMENT,
    FIR_DIAG_INPUT,
    FIR_DIAG_ENCODING,
    FIR_DIAG_LIMIT,
    FIR_DIAG_RESOURCE,
    FIR_DIAG_SYNTAX,
    FIR_DIAG_SEMANTIC,
    FIR_DIAG_OUTPUT
} FirSourceDiagnosticKind;

typedef struct
{
    FirSourceDiagnosticKind kind;
    int code;         /* Coco/R syntax code; zero for other failures. */
    int has_location; /* A zero-length location is an insertion/EOF point. */
    size_t byte_offset;
    size_t byte_length;
    size_t line;
    size_t column;
} FirSourceDiagnostic;


/* Additive source API: diagnostic may be NULL, cleared on every call.
     * No borrowed source/error/stream pointer survives return. Parser calls must
     * be serialized, including calls through the legacy wrapper below. */
int fir_source_ex(FILE *, FILE *, FirLanguage, const char *module_name, char *, size_t, FirSourceDiagnostic *);



/* Compile with an optional FIRDBG1 source snapshot/value/binding sidecar.
     * This maps unoptimized FIR SSA identities, not final SWF/Dis/native PCs.
     * Streams are caller-owned/disjoint. Rejected translation writes neither;
     * final copy failures can leave prefixes and are not a two-file transaction. */
int fir_source_debug(
    FILE *, FILE *, FirLanguage, const char *module_name, char *, size_t, FirSourceDiagnostic *, FILE *debug_output);


int fir_source(FILE *, FILE *, FirLanguage, const char *module_name, char *, size_t);


int fir_run(FirTask, FILE *, FILE *, const FirRunOptions *, char *, size_t);
#ifdef __cplusplus
}
#endif
#endif
