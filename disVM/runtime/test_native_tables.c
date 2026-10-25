/* Exercise the actual NC64 table adapter with unaligned slots and invalid full-width PCs. */
#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define nil NULL                   /* Fixture's native null pointer. */
#define H ((void *)(uintptr) - 1)  /* Retained VM nil-reference sentinel. */
#define OP(name) void name(void)   /* Same parameterless opcode interface as the interpreter. */
#define exModule "module"          /* Stable fixture error identity. */
#define exBounds "bounds"          /* Stable fixture error identity. */

typedef long WORD;                 /* Required eight-byte NC64 cell on this accepted host. */
typedef long long LONG;            /* Signed 64-bit big cell. */
typedef unsigned long long uvlong; /* Address/product comparison width. */
typedef uintptr_t uintptr;         /* Native address integer. */


/* Minimal instruction storage; tests compare selected entries, never execute fixture instructions. */
typedef struct Inst
{
    int operation; /* Distinct storage position only. */
} Inst;


/* Borrowed program description used by the adapter's validated PC resolver. */
typedef struct Module
{
    int nprog;  /* Number of valid instruction destinations. */
    Inst *prog; /* Borrowed fixture instruction vector. */
} Module;


/* Borrowed link description; relocated legacy native-PC images are deliberately rejected. */
typedef struct Modlink
{
    int compiled; /* Nonzero identifies an unsupported relocated image. */
    Module *m;    /* Borrowed ordinary module metadata. */
} Modlink;


/* Fixture register view supplies the fields actually consumed by the reviewed adapter. */
static struct
{
    void *s;    /* Borrowed possibly unaligned source cell. */
    void *d;    /* Borrowed possibly unaligned table payload. */
    Modlink *M; /* Borrowed current ordinary module link. */
    Inst *PC;   /* Only field the adapter may change on success. */
} DVM_R;

static jmp_buf failure; /* Native fixture error boundary. */


/* Raise a fixture VM error; message is borrowed, and execution never returns normally. */
static void error(const char *message)
{
    (void)message;
    longjmp(failure, 1);
}

#include "native_tables.c"


/* Store one exact eight-byte cell without alignment assumptions; no ownership transfer. */
static void cell(unsigned char *bytes, size_t offset, WORD value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}


/* Execute an opcode and return whether an explicit VM error was raised.
 * operation is borrowed; callers inspect PC independently after the error boundary. */
static int rejected(void (*operation)(void))
{
    if (setjmp(failure))
    {
        return 1;
    }
    operation();
    return 0;
}


/* Verify independently packed layouts and failure atomicity. No arguments or resources escape. */
int main(void)
{
    Inst code[8];
    Module module = {8, code};
    Modlink link = {0, &module};
    unsigned char storage[128];
    unsigned char input[16];
    unsigned char *table = storage + 1;
    if (sizeof(WORD) != 8 || sizeof(LONG) != 8)
    {
        fprintf(stderr, "NC64 table fixture requires eight-byte cells\n");
        return 1;
    }
    memset(storage, 0, sizeof(storage));
    DVM_R.s = input + 1;
    DVM_R.d = table;
    DVM_R.M = &link;
    DVM_R.PC = code;
    cell(input + 1, 0, -4);
    cell(table, 0, 2);
    cell(table, 8, -7);
    cell(table, 16, -1);
    cell(table, 24, 3);
    cell(table, 32, 10);
    cell(table, 40, 16);
    cell(table, 48, 4);
    cell(table, 56, 7);
    if (rejected(icase) || DVM_R.PC != code + 3)
    {
        goto bad;
    }
    cell(input + 1, 0, 9);
    if (rejected(icase) || DVM_R.PC != code + 7)
    {
        goto bad;
    }
    cell(table, 56, (WORD)4294967297LL);
    DVM_R.PC = code + 2;
    if (!rejected(icase) || DVM_R.PC != code + 2)
    {
        goto bad;
    }
    cell(table, 0, -1);
    if (!rejected(icase) || DVM_R.PC != code + 2)
    {
        goto bad;
    }
    memset(storage, 0, sizeof(storage));
    cell(table, 0, 1);
    cell(table, 16, (WORD)4294967296LL);
    cell(table, 24, (WORD)4294967301LL);
    cell(table, 32, 5);
    cell(table, 48, 7);
    cell(input + 1, 0, (WORD)4294967298LL);
    if (rejected(casel) || DVM_R.PC != code + 5)
    {
        goto bad;
    }
    DVM_R.d = table + 8;
    cell(table, 0, 2);
    cell(table, 8, 4);
    cell(table, 16, 6);
    cell(input + 1, 0, 1);
    if (rejected(igoto) || DVM_R.PC != code + 6)
    {
        goto bad;
    }
    cell(input + 1, 0, LONG_MIN);
    if (!rejected(igoto) || DVM_R.PC != code + 6)
    {
        goto bad;
    }
    cell(input + 1, 0, 1);
    cell(table, 16, (WORD)4294967297LL);
    if (!rejected(igoto) || DVM_R.PC != code + 6)
    {
        goto bad;
    }
    puts("PASS actual NC64 table adapter: unaligned rows, full-width bounds and unchanged error PCs");
    return 0;
bad:
    fprintf(stderr, "NC64 numeric table selected an incorrect or unvalidated PC\n");
    return 1;
}
