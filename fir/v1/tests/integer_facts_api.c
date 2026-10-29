/* Independent soundness checks for the shared int32 bit-analysis C99 API. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Parse a verified graph whose producer appears after its consumer in text.
 * left/right are independent mask constants; operation selects one bit formula.
 * Return one only if every reported fact holds for all sampled full-width inputs
 * and module bytes remain unchanged. All streams/modules/scratch are owned here;
 * calls are serialized, bounded, and execute no VM or user callbacks. */
static int exercise(int32_t left, int32_t right, int operation)
{
    static const char *const operators[] = {"and", "or", "xor"}; /* Existing FIR opcode suffixes. */
    FirModule *module = malloc(sizeof(*module));
    FirModule *saved = malloc(sizeof(*saved));
    FirIntegerFacts facts;
    FILE *input = fir_temporary();
    char error[256] = "";
    int value = -1;
    int index;
    int ok = 0;
    uint32_t state = 0x91e10da5U;
    if (!module || !saved || !input ||
        fprintf(input,
                "FIR 1.0\nmodule @facts\nfunc @main() -> void {\n^entry:\n"
                "%%cell: i32.slot = i32.local\njump ^producer\n^consumer:\n"
                "%%b: i32 = const.i32 %ld\n%%r: i32 = i32.%s %%inner, %%b\ntrace %%r\nreturn\n"
                "^producer:\n%%x: i32 = i32.load %%cell\n%%a: i32 = const.i32 %ld\n"
                "%%inner: i32 = i32.and %%x, %%a\njump ^consumer\n}\n",
                (long)right,
                operators[operation],
                (long)left) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    for (index = 0; index < module->value_count; index++)
    {
        if (!strcmp(module->values[index].name, "%r"))
        {
            value = index;
        }
    }
    if (value < 0)
    {
        goto done;
    }
    memcpy(saved, module, sizeof(*saved));
    memset(&facts, 0xff, sizeof(facts));
    fir_integer_facts(module, &facts);
    /* Require propagation through the later dominating block, not merely
     * sound unknown output. These masks are derived independently from x&left. */
    if (facts.zero[value] !=
            (operation == 0 ? ~((uint32_t)left & (uint32_t)right) : ~((uint32_t)left | (uint32_t)right)) ||
        facts.one[value] != (operation == 0   ? 0
                             : operation == 1 ? (uint32_t)right
                                              : ~(uint32_t)left & (uint32_t)right) ||
        memcmp(saved, module, sizeof(*saved)) || (facts.zero[value] & facts.one[value]))
    {
        goto done;
    }
    for (index = 0; index < 1024; index++)
    {
        uint32_t actual;
        uint32_t inner;
        state = state * 1664525U + 1013904223U;
        inner = state & (uint32_t)left;
        actual = operation == 0   ? inner & (uint32_t)right
                 : operation == 1 ? inner | (uint32_t)right
                                  : inner ^ (uint32_t)right;
        if ((actual & facts.zero[value]) || (actual & facts.one[value]) != facts.one[value] ||
            (fir_integer_nonzero(&facts, value) && actual == 0) ||
            (fir_integer_shift_count(&facts, value, 32) && actual >= 32) ||
            (fir_integer_shift_count(&facts, value, 64) && actual >= 64) || fir_integer_shift_count(&facts, value, 16))
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "integer facts mismatch: %ld/%ld op%d: %s\n", (long)left, (long)right, operation, error);
    }
    if (input)
    {
        fclose(input);
    }
    free(module);
    free(saved);
    return ok;
}


/* Check big shift/complement facts against unsigned mathematical payload bits.
 * count is a legal0..63 shift, right selects arithmetic right versus left.
 * Own/close parser input and module. Return zero with a diagnostic on mismatch;
 * no VM, callbacks, mutable-cell inference or signed host shifts are used. */
static int exercise_big(unsigned count, int right)
{
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FirIntegerFacts facts;
    char error[256] = "";
    uint64_t state = UINT64_C(0xa395ce12918fbd07);
    int value = -1;
    int index;
    int ok = 0;
    if (!module || !input ||
        fprintf(input,
                "FIR 1.0\nmodule @bigfacts\nfunc @main() -> void {\n^entry:\n"
                "%%cell: i64.slot = big.local\n%%x: i64 = big.load %%cell\n"
                "%%mask: i64 = const.i64 %lld\n%%count: i32 = const.i32 %u\n"
                "%%a: i64 = big.or %%x, %%mask\n%%shift: i64 = big.%s %%a, %%count\n"
                "%%r: i64 = big.not %%shift\ntrace %%r\nreturn\n}\n",
                right ? (long long)INT64_MIN : 1LL,
                count,
                right ? "shr" : "shl") < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    for (index = 0; index < module->value_count; index++)
    {
        if (!strcmp(module->values[index].name, "%r"))
        {
            value = index;
        }
    }
    if (value < 0)
    {
        goto done;
    }
    fir_integer_facts(module, &facts);
    for (index = 0; index < 1024; index++)
    {
        uint64_t actual;
        state = state * UINT64_C(6364136223846793005) + 1;
        actual = state | (right ? UINT64_C(0x8000000000000000) : 1);
        if (right)
        {
            actual >>= count;
            if (count != 0)
            {
                actual |= UINT64_MAX << (64 - count);
            }
        }
        else
        {
            actual <<= count;
        }
        actual = ~actual;
        if ((actual & facts.big_zero[value]) || (actual & facts.big_one[value]) != facts.big_one[value] ||
            (fir_big_nonzero(&facts, value) && actual == 0))
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "big facts mismatch: count%u right%d: %s\n", count, right, error);
    }
    if (input)
    {
        fclose(input);
    }
    free(module);
    return ok;
}


/* Exercise high/sign bits, zero masks, bounds and reuse in non-topological CFGs.
 * No arguments or borrowed owners. Return nonzero at the first explicit failure. */
int main(void)
{
    static const int32_t masks[] = {0, 1, 31, 63, INT32_MIN, -1};
    unsigned left;
    unsigned right;
    int operation;
    for (left = 0; left < sizeof(masks) / sizeof(masks[0]); left++)
    {
        for (right = 0; right < sizeof(masks) / sizeof(masks[0]); right++)
        {
            for (operation = 0; operation < 3; operation++)
            {
                if (!exercise(masks[left], masks[right], operation))
                {
                    return 1;
                }
            }
        }
    }
    for (left = 0; left < 5; left++)
    {
        static const unsigned counts[] = {0, 1, 31, 32, 63}; /* Width and sign boundaries. */
        if (!exercise_big(counts[left], 0) || !exercise_big(counts[left], 1))
        {
            return 1;
        }
    }
    puts("PASS integer facts C99: 120832 soundness samples, big shifts/complement, immutable IR and CFG");
    return 0;
}
