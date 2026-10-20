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
        FIR_LIMBO_I32
    } FirSemantics;
    typedef enum
    {
        T_VOID,
        T_I32,
        T_F64,
        T_BOOL,
        T_STRING,
        T_ANY,
        T_CHANNEL
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
        O_RETURN
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
        FirOp op; /* Exact operation: effects and target lowering dispatch on this tag. */
        FirType type;
        int result;      /* SSA result index, or -1 for an instruction without a result. */
        int a, b;        /* Operand value indices; unused operands remain -1. */
        int yes, no;     /* Block-table indices for jump/conditional successors. */
        int block;       /* Containing block-table index used by dominance analysis. */
        int line;        /* Physical source line retained for diagnostics and dumps. */
        int removed;     /* Optimizer tombstone; printers/backends omit this instruction. */
        int32_t integer; /* Signed, fixed-width integer constant payload. */
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
    /* Reject malformed control flow, invalid SSA uses and incompatible types. */
    int fir_verify(FirModule *module, char *error, size_t capacity);
    /* Print canonical, reparsable FIR text; keep diagnostic line annotations. */
    int fir_print(FILE *output, const FirModule *module);
    /* Apply conservative rewrites in place and verify the result. */
    int fir_optimise(FirModule *module, char *error, size_t capacity);
    /* Compile verified FIR into a versioned SWO1 object, never a SWF container. */
    int fir_object(FILE *output, FirModule *module, char *error, size_t capacity);
    /* Optional caller-owned temporary stream; honors TMPDIR on POSIX. */
    FILE *fir_temporary(void);
    /* Non-console entry points; caller owns every stream and diagnostic buffer.
     * Parser calls must be serialized; sequential calls reset state after errors. */

    typedef enum
    {
        FIR_CHECK,
        FIR_PRINT,
        FIR_OPT,
        FIR_OBJECT
    } FirTask;
    typedef int (*FirSnapshot)(void *user, const char *phase, const FirModule *module);
    typedef struct
    {
        int optimise;
        FirSnapshot snapshot;
        void *user;
    } FirRunOptions;
    int fir_source(FILE *, FILE *, FirLanguage, const char *module_name, char *, size_t);
    int fir_run(FirTask, FILE *, FILE *, const FirRunOptions *, char *, size_t);
#ifdef __cplusplus
}
#endif
#endif
