/* Independent soundness checks for the shared int32 bit-analysis C99 API. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Parse a verified graph whose producer appears after its consumer in text.
 * left/right are independent mask constants; operation selects a bit or modular arithmetic formula.
 * Return one only if every reported fact holds for all sampled full-width inputs
 * and module bytes remain unchanged. All streams/modules/scratch are owned here;
 * calls are serialized, bounded, and execute no VM or user callbacks. */
static int exercise(int32_t left, int32_t right, int operation)
{
    static const char *const operators[] = {
        "and", "or", "xor", "add.wrap", "sub.wrap"}; /* Existing FIR opcode suffixes. */
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
    if ((operation < 3 && (facts.zero[value] != (operation == 0 ? ~((uint32_t)left & (uint32_t)right)
                                                                : ~((uint32_t)left | (uint32_t)right)) ||
                           facts.one[value] != (operation == 0   ? 0
                                                : operation == 1 ? (uint32_t)right
                                                                 : ~(uint32_t)left & (uint32_t)right))) ||
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
                 : operation == 2 ? inner ^ (uint32_t)right
                 : operation == 3 ? inner + (uint32_t)right
                                  : inner - (uint32_t)right;
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
 * count is a legal0..63 shift, right selects arithmetic right versus left;
 * negate selects modular negation instead of complement of the shifted value.
 * Own/close parser input and module. Return zero with a diagnostic on mismatch;
 * no VM, callbacks, mutable-cell inference or signed host shifts are used. */
static int exercise_big(unsigned count, int right, int negate)
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
                "%%r: i64 = big.%s %%shift\ntrace %%r\nreturn\n}\n",
                right ? (long long)INT64_MIN : 1LL,
                count,
                right ? "shr" : "shl",
                negate ? "neg" : "not") < 0 ||
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
    if (negate && !right && !fir_big_nonzero(&facts, value))
    {
        goto done;
    }
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
        actual = negate ? UINT64_C(0) - actual : ~actual;
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


/* Check modular carry/borrow facts against concrete unsigned evaluations.
 * width is32/64, count chooses a low-bit boundary and subtract selects a-b.
 * Parse independent unknown cells, mask each operand, then sample1024 pairs.
 * Require retained low-bit information and disjoint masks; return zero with
 * diagnostics on failure. Own/free the module and stream; never execute a VM. */
static int exercise_arithmetic(unsigned width, unsigned count, int subtract)
{
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FirIntegerFacts facts;
    char error[256] = "";
    const char *kind = width == 32 ? "i32" : "i64";
    const char *prefix = width == 32 ? "i32" : "big";
    const char *suffix = width == 32 ? ".wrap" : "";
    uint64_t mask = width == 32 ? UINT32_MAX : UINT64_MAX;
    uint64_t low = (UINT64_C(1) << count) - 1;
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    uint64_t zero;
    uint64_t one;
    int value = -1;
    int index;
    int ok = 0;
    if (!module || !input ||
        fprintf(input,
                "FIR 1.0\nmodule @carry\nfunc @main() -> void {\n^entry:\n"
                "%%cx: %s.slot = %s.local\n%%cy: %s.slot = %s.local\n"
                "%%x: %s = %s.load %%cx\n%%y: %s = %s.load %%cy\n"
                "%%mask: %s = const.%s %lld\n"
                "%%a: %s = %s.and %%x, %%mask\n%%b: %s = %s.and %%y, %%mask\n"
                "%%r: %s = %s.%s%s %%a, %%b\ntrace %%r\nreturn\n}\n",
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                kind,
                -(long long)low - 1,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                subtract ? "sub" : "add",
                suffix) < 0 ||
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
    zero = width == 32 ? facts.zero[value] : facts.big_zero[value];
    one = width == 32 ? facts.one[value] : facts.big_one[value];
    if ((zero & one) || (zero & low) != low || (one & ~mask) || (zero & ~mask))
    {
        goto done;
    }
    for (index = 0; index < 1024; index++)
    {
        uint64_t a;
        uint64_t b;
        uint64_t actual;
        state = state * UINT64_C(6364136223846793005) + 1;
        a = state & ~low & mask;
        state = state * UINT64_C(6364136223846793005) + 1;
        b = state & ~low & mask;
        actual = (subtract ? a - b : a + b) & mask;
        if ((actual & zero) || (actual & one) != one)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "carry facts mismatch: width%u count%u sub%d: %s\n", width, count, subtract, error);
    }
    if (input)
    {
        fclose(input);
    }
    free(module);
    return ok;
}


/* Exhaust all four-bit independent abstract domains for two operands.
 * width is32/64 and subtract selects modular subtraction. Parse one owned
 * graph, vary only its four literal masks, and compare every inferred bit with
 * the intersection of256 concrete operand assignments per domain pair.
 * Return zero on any missing/unsound fact. The graph remains verified because
 * edits replace same-type literals; each analysis starts fresh. No VM executes. */
static int exhaustive_arithmetic(unsigned width, int subtract)
{
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FirIntegerFacts facts;
    char error[256] = "";
    const char *kind = width == 32 ? "i32" : "i64";
    const char *prefix = width == 32 ? "i32" : "big";
    const char *names[] = {"%am", "%as", "%bm", "%bs", "%r"};
    int values[] = {-1, -1, -1, -1, -1};
    uint64_t mask = width == 32 ? UINT32_MAX : UINT64_MAX;
    unsigned ap;
    unsigned bp;
    int index;
    int ok = 0;
    if (!module || !input ||
        fprintf(input,
                "FIR 1.0\nmodule @exhaustive\nfunc @main() -> void {\n^entry:\n"
                "%%cx: %s.slot = %s.local\n%%cy: %s.slot = %s.local\n"
                "%%x: %s = %s.load %%cx\n%%y: %s = %s.load %%cy\n"
                "%%am: %s = const.%s 0\n%%as: %s = const.%s 0\n"
                "%%bm: %s = const.%s 0\n%%bs: %s = const.%s 0\n"
                "%%aa: %s = %s.and %%x, %%am\n%%a: %s = %s.or %%aa, %%as\n"
                "%%bb: %s = %s.and %%y, %%bm\n%%b: %s = %s.or %%bb, %%bs\n"
                "%%r: %s = %s.%s%s %%a, %%b\ntrace %%r\nreturn\n}\n",
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                kind,
                kind,
                kind,
                kind,
                kind,
                kind,
                kind,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                kind,
                prefix,
                subtract ? "sub" : "add",
                width == 32 ? ".wrap" : "") < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    for (index = 0; index < module->value_count; index++)
    {
        unsigned name;
        for (name = 0; name < 5; name++)
        {
            if (!strcmp(module->values[index].name, names[name]))
            {
                values[name] = index;
            }
        }
    }
    for (index = 0; index < 5; index++)
    {
        if (values[index] < 0)
        {
            goto done;
        }
    }
    for (ap = 0; ap < 81; ap++)
    {
        for (bp = 0; bp < 81; bp++)
        {
            unsigned payloads[] = {0, 0, 0, 0};
            unsigned patterns[] = {ap, bp};
            uint64_t zero = mask;
            uint64_t one = mask;
            unsigned operand;
            unsigned x;
            unsigned y;
            for (operand = 0; operand < 2; operand++)
            {
                unsigned bit;
                unsigned pattern = patterns[operand];
                for (bit = 0; bit < 4; bit++)
                {
                    unsigned digit = pattern % 3;
                    pattern /= 3;
                    if (digit == 2)
                    {
                        payloads[operand * 2] |= 1U << bit;
                    }
                    else if (digit == 1)
                    {
                        payloads[operand * 2 + 1] |= 1U << bit;
                    }
                }
            }
            for (index = 0; index < 4; index++)
            {
                FirInstruction *literal = &module->instructions[module->values[values[index]].definition];
                literal->integer = (int32_t)payloads[index];
                literal->big = payloads[index];
            }
            fir_integer_facts(module, &facts);
            for (x = 0; x < 16; x++)
            {
                for (y = 0; y < 16; y++)
                {
                    uint64_t a = (x & payloads[0]) | payloads[1];
                    uint64_t b = (y & payloads[2]) | payloads[3];
                    uint64_t actual = (subtract ? a - b : a + b) & mask;
                    zero &= ~actual;
                    one &= actual;
                }
            }
            if ((width == 32 ? facts.zero[values[4]] : facts.big_zero[values[4]]) != zero ||
                (width == 32 ? facts.one[values[4]] : facts.big_one[values[4]]) != one)
            {
                fprintf(stderr, "abstract carry mismatch: width%u sub%d domains%u/%u\n", width, subtract, ap, bp);
                goto done;
            }
        }
    }
    ok = 1;
done:
    if (!ok && error[0])
    {
        fprintf(stderr, "abstract carry graph: %s\n", error);
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
            for (operation = 0; operation < 5; operation++)
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
        if (!exercise_big(counts[left], 0, 0) || !exercise_big(counts[left], 1, 0) ||
            !exercise_big(counts[left], 0, 1) || !exercise_big(counts[left], 1, 1))
        {
            return 1;
        }
    }
    for (left = 1; left < 64; left++)
    {
        if (!exercise_arithmetic(64, left, 0) || !exercise_arithmetic(64, left, 1) ||
            (left < 32 && (!exercise_arithmetic(32, left, 0) || !exercise_arithmetic(32, left, 1))))
        {
            return 1;
        }
    }
    if (!exhaustive_arithmetic(32, 0) || !exhaustive_arithmetic(32, 1) || !exhaustive_arithmetic(64, 0) ||
        !exhaustive_arithmetic(64, 1))
    {
        return 1;
    }
    puts("PASS integer facts C99: 6718464 exhaustive arithmetic evaluations, "
         "192512 carry/borrow and204800 mask/shift/negation samples; immutable IR and CFG");
    return 0;
}
