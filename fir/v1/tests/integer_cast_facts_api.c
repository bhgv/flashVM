/* Independent basis-input oracle for typed integer conversion facts. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Return the modulo64 representation of the mathematical signed32 low bits.
 * bits is an arbitrary unsigned payload. Decode the sign without an out-of-range
 * cast; no allocation, callbacks or mutable owners participate. */
static uint64_t extend32(uint64_t bits)
{
    uint32_t low = (uint32_t)bits;
    int64_t value = low <= INT32_MAX ? (int64_t)low : -(int64_t)(UINT32_MAX - low) - 1;
    return (uint64_t)value;
}


/* Parse and analyze one non-topological scalar conversion graph.
 * mode0 widens int32 then narrows; mode1 narrows big then widens; mode2 converts
 * a dynamic bool to both widths. keep/force are borrowed mask payloads. Compare
 * every reported bit with a complete bitwise basis, requiring exact useful masks
 * and unchanged IR. Own/free all streams, modules and scratch; return0 on error.
 * No VM evaluation, external callbacks, waits or shared mutation occurs. */
static int exercise(unsigned mode, int64_t keep, int64_t force)
{
    FirModule *module = malloc(sizeof(*module));
    FirModule *saved = malloc(sizeof(*saved));
    FirIntegerFacts facts;
    FILE *input = fir_temporary();
    const char *kind = mode == 0 ? "i32" : "i64";
    const char *cell = mode == 0 ? "i32" : "big";
    char error[256] = "";
    int values[2] = {-1, -1};
    uint64_t observed_zero[2] = {UINT64_MAX, UINT64_MAX};
    uint64_t observed_one[2] = {UINT64_MAX, UINT64_MAX};
    uint64_t output_masks[2] = {mode == 0 ? UINT64_MAX : UINT32_MAX, mode == 0 ? UINT32_MAX : UINT64_MAX};
    unsigned sample;
    int index;
    int ok = 0;
    if (!module || !saved || !input)
    {
        goto done;
    }
    if (mode == 2)
    {
        if (fputs("FIR 1.0\nmodule @cast\nfunc @main() -> void {\n^entry:\n"
                  "%cell: bool.slot = bool.local\njump ^producer\n^consumer:\n"
                  "%b: i32 = value.int %a\n%r: i64 = value.big %a\ntrace %b\ntrace %r\nreturn\n"
                  "^producer:\n%a: bool = bool.load %cell\njump ^consumer\n}\n",
                  input) < 0)
        {
            goto done;
        }
    }
    else if (fprintf(input,
                     "FIR 1.0\nmodule @cast\nfunc @main() -> void {\n^entry:\n"
                     "%%cell: %s.slot = %s.local\njump ^producer\n^consumer:\n"
                     "%%b: %s = value.%s %%a\n%%r: %s = value.%s %%b\ntrace %%b\ntrace %%r\nreturn\n"
                     "^producer:\n%%x: %s = %s.load %%cell\n"
                     "%%keep: %s = const.%s %lld\n%%force: %s = const.%s %lld\n"
                     "%%masked: %s = %s.and %%x, %%keep\n%%a: %s = %s.or %%masked, %%force\njump ^consumer\n}\n",
                     kind,
                     cell,
                     mode == 0 ? "i64" : "i32",
                     mode == 0 ? "big" : "int",
                     kind,
                     mode == 0 ? "int" : "big",
                     kind,
                     cell,
                     kind,
                     kind,
                     (long long)keep,
                     kind,
                     kind,
                     (long long)force,
                     kind,
                     cell,
                     kind,
                     cell) < 0)
    {
        goto done;
    }
    if (fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    for (index = 0; index < module->value_count; index++)
    {
        if (!strcmp(module->values[index].name, "%b"))
        {
            values[0] = index;
        }
        if (!strcmp(module->values[index].name, "%r"))
        {
            values[1] = index;
        }
    }
    if (values[0] < 0 || values[1] < 0)
    {
        goto done;
    }
    memcpy(saved, module, sizeof(*saved));
    fir_integer_facts(module, &facts);
    for (sample = 0; sample < 130; sample++)
    {
        uint64_t input_bits = sample == 0 ? 0 : sample == 1 ? UINT64_MAX : UINT64_C(1) << ((sample - 2) / 2);
        uint64_t actual[2];
        if (sample >= 2 && (sample & 1))
        {
            input_bits = ~input_bits;
        }
        input_bits = mode == 2 ? sample & 1 : (input_bits & (uint64_t)keep) | (uint64_t)force;
        actual[0] = mode == 0 ? extend32(input_bits) : input_bits & UINT32_MAX;
        actual[1] = mode == 0 ? input_bits & UINT32_MAX : extend32(actual[0]);
        for (index = 0; index < 2; index++)
        {
            uint64_t zero =
                output_masks[index] == UINT64_MAX ? facts.big_zero[values[index]] : facts.zero[values[index]];
            uint64_t one = output_masks[index] == UINT64_MAX ? facts.big_one[values[index]] : facts.one[values[index]];
            observed_zero[index] &= ~actual[index] & output_masks[index];
            observed_one[index] &= actual[index] & output_masks[index];
            if ((actual[index] & zero) || (actual[index] & one) != one || (zero & one))
            {
                goto done;
            }
        }
    }
    for (index = 0; index < 2; index++)
    {
        uint64_t zero = output_masks[index] == UINT64_MAX ? facts.big_zero[values[index]] : facts.zero[values[index]];
        uint64_t one = output_masks[index] == UINT64_MAX ? facts.big_one[values[index]] : facts.one[values[index]];
        if (zero != observed_zero[index] || one != observed_one[index])
        {
            goto done;
        }
    }
    ok = !memcmp(saved, module, sizeof(*saved));
done:
    if (!ok)
    {
        fprintf(stderr,
                "cast facts mismatch mode%u keep%lld force%lld: %s\n",
                mode,
                (long long)keep,
                (long long)force,
                error);
    }
    if (input)
    {
        fclose(input);
    }
    free(saved);
    free(module);
    return ok;
}


/* Cover independently varying low/sign/high masks and boolean outputs.
 * No borrowed state. Return nonzero at the first failed active predicate. */
int main(void)
{
    static const int64_t masks[] = {0,
                                    1,
                                    7,
                                    INT32_MAX,
                                    INT32_MIN,
                                    -1,
                                    INT64_MIN,
                                    INT64_MAX,
                                    INT64_C(-4294967296),
                                    INT64_C(4294967295)}; /* Scalar boundaries. */
    unsigned left;
    unsigned right;
    unsigned cases = 1;
    if (!exercise(2, 0, 0))
    {
        return 1;
    }
    for (left = 0; left < sizeof(masks) / sizeof(masks[0]); left++)
    {
        for (right = 0; right < sizeof(masks) / sizeof(masks[0]); right++)
        {
            if (!exercise(1, masks[left], masks[right]))
            {
                return 1;
            }
            cases++;
            if (left < 6 && right < 6)
            {
                if (!exercise(0, masks[left], masks[right]))
                {
                    return 1;
                }
                cases++;
            }
        }
    }
    printf("PASS typed cast facts: %u graphs, %u output basis checks, immutable IR and non-topological CFG\n",
           cases,
           cases * 260);
    return 0;
}
