/* FIR 1.0: standalone C99, no FlashVM headers or private ABI dependency. */
#ifndef FIR_H
#define FIR_H
#include <stdio.h>
#include <stdint.h>
/* Initial hard bounds; exceeding them is a diagnostic, never truncation. */
#define FIR_LIMIT 4096
#define FIR_BLOCKS 256
#define FIR_TEXT 256

typedef enum { T_VOID, T_I32, T_F64, T_BOOL, T_STRING, T_ANY, T_CHANNEL } FirType;
typedef enum {
    O_I32, O_F64, O_BOOL, O_STRING, O_COPY,
    O_ADD, O_SUB, O_MUL, O_LT, O_EQ,
    O_FADD, O_FSUB, O_FMUL, O_ASADD, O_NUMBER,
    O_CHANNEL, O_RECEIVE, O_TRACE, O_SEND, O_CLOSE, O_IF, O_JUMP, O_RETURN
} FirOp;
/* Conservative operation effects. Unknown opcodes have every flag set. */
enum {
    FIR_READS_MEMORY = 1, FIR_WRITES_MEMORY = 2, FIR_ALLOCATES = 4,
    FIR_MAY_THROW = 8, FIR_MAY_CALL_USER = 16, FIR_MAY_SUSPEND = 32,
    FIR_ALL_EFFECTS = 63
};
unsigned fir_effects(FirOp op);
/* Operands are symbol-table indices; -1 means absent. Source lines survive
 * optimisation. Strings are UTF-8 bytes without embedded NUL. */
typedef struct {
    FirOp op;
    FirType type;
    int result, a, b, yes, no, block, line, removed;
    int32_t integer;
    double real;
    char text[FIR_TEXT];
} FirInstruction;
/* Symbol-table entry: printable spelling, defining instruction index and type. */
typedef struct {
    char name[FIR_TEXT];
    int definition;
    FirType type;
} FirValue;
/* Label entry: forward references are resolved before verification. */
typedef struct {
    char name[FIR_TEXT];
    int defined;
} FirBlock;
typedef struct {
    FirInstruction instructions[FIR_LIMIT];
    FirValue values[FIR_LIMIT];
    FirBlock blocks[FIR_BLOCKS];
    int count, value_count, block_count, channels;
    char module[FIR_TEXT];
} FirModule;
/* All functions return 1 on success and 0 with a diagnostic on failure.
 * The caller owns the module, streams and diagnostic buffer. The generated
 * parser has global state: one input per process, not thread-safe. */
/* Read and construct one FIR translation unit from a binary input stream. */
int fir_parse(FILE *input, FirModule *module, char *error, size_t capacity);
/* Reject malformed control flow, invalid SSA uses and incompatible types. */
int fir_verify(FirModule *module, char *error, size_t capacity);
/* Print canonical, reparsable FIR text; keep diagnostic line annotations. */
int fir_print(FILE *output, const FirModule *module);
/* Apply conservative rewrites in place and verify the result. */
int fir_optimise(FirModule *module, char *error, size_t capacity);
/* Emit checked AVM1 records (raw != 0), or a complete FWS8 file. */
int fir_swf(FILE *output, const FirModule *module, int raw,
            char *error, size_t capacity);
#endif
