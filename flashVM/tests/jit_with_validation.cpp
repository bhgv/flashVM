/* With record framing, byte targets, nesting and exact nested checkpoint budgets. */
#define FVM_WITH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_with.cpp"
#include "flashvm/debug.h"

static unsigned breakpoint_hits;
static unsigned long breakpoint_offset;


static FVM_Result with_breakpoint(FVM_Context *, const FVM_DebugBreakpoint *point, void *)
{
    if (!(point->offset == breakpoint_offset && !std::strcmp(point->function, "<function>")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++breakpoint_hits;
    return FVM_OK;
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char good[] = {0};
    FVM_JitCode *code = 0, *keep = 0;
    if (!(fvm_jit_compile(good, sizeof(good), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    keep = code;
    const unsigned char invalid[][10] = {
        {0x94, 1, 0, 0, 0}, {0x94, 2, 0, 2, 0, 0}, {0x94, 4, 0, 1, 0, 0, 0, 0}, {0x94, 2, 0, 2, 0, 0, 0, 0}, {0, 0x17}};
    const unsigned sizes[] = {5, 6, 8, 8, 2};
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        if (!(fvm_jit_compile(invalid[i], sizes[i], &code) == FVM_INVALID_ARGUMENT && code == keep))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Program body;
    body.number(42);
    body.op(0x17);
    Program entering;
    entering.op(0x99);
    definition_word(entering, 2);
    definition_word(entering, 5);
    with_block(entering, body);
    entering.op(0);
    if (!(fvm_jit_compile(entering.bytes.begin(), entering.bytes.size(), &code) == FVM_INVALID_ARGUMENT &&
          code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program past;
    past.op(0x99);
    definition_word(past, 2);
    definition_word(past, 0);
    Program escaping;
    escaping.op(0x96);
    definition_word(escaping, 1);
    escaping.op(2);
    with_block(escaping, past);
    escaping.op(0);
    if (!(fvm_jit_compile(escaping.bytes.begin(), escaping.bytes.size(), &code) == FVM_INVALID_ARGUMENT &&
          code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program unsupported;
    unsupported.op(0x16);
    Program fallback;
    fallback.op(0x96);
    definition_word(fallback, 1);
    fallback.op(2);
    with_block(fallback, unsupported);
    fallback.op(0);
    if (!(fvm_jit_compile(fallback.bytes.begin(), fallback.bytes.size(), &code) == FVM_UNSUPPORTED && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program deep;
    deep.number(42);
    deep.op(0x3e);
    for (unsigned i = 0; i < 65; ++i)
    {
        Program outer;
        outer.variable("owner");
        with_block(outer, deep);
        deep = outer;
    }
    deep.op(0);
    if (!(fvm_jit_compile(deep.bytes.begin(), deep.bytes.size(), &code) == FVM_UNSUPPORTED && code == keep))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            for (unsigned explicit_end = 0; explicit_end < 2; ++explicit_end)
            {
                for (unsigned early = 0; early < 2; ++early)
                {
                    for (unsigned success = 0; success < 2; ++success)
                    {
                        FVM_Config cfg;
                        FVM_Runtime *rt = 0;
                        FVM_Context *c = 0;
                        if (!(fvm_config_init(&cfg) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        unsigned exact = early ? 6 : 7 + explicit_end;
                        cfg.instruction_limit = exact - (success ? 0 : 1);
                        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK && fvm_context_create(rt, &c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                        if (loaded)
                        {
                            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        with_owner(c);
                        Program inner;
                        inner.number(42);
                        inner.op(early ? 0x3e : 0x17);
                        if (explicit_end)
                        {
                            inner.op(0);
                        }
                        Program p;
                        p.number(77);
                        p.variable("owner");
                        size_t body_at = p.bytes.size() + 5;
                        with_block(p, inner);
                        FVM_Value out = with_run(c, p, success ? FVM_OK : FVM_ERROR);
                        if (success)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == (early ? 42 : 77)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!success && early)
                        {
                            p.finish();
                            if (!(fvm_execution_set_mode(
                                      c, class_mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_avm1_execute(c, p.bytes.begin(), p.bytes.size()) == FVM_ERROR &&
                                  c->debug_offset == body_at + 8))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(c->avm1_steps == exact && c->lexical_scopes.empty()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                        fvm_runtime_destroy(rt);
                    }
                }
            }
        }
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    with_owner(c);
    Program rejected;
    rejected.number(77);
    rejected.op(0x96);
    definition_word(rejected, 1);
    rejected.op(2);
    with_block(rejected, unsupported);
    rejected.finish();
    FVM_Object *fallback_fn = 0;
    FVM_Value result;
    if (!(fvm_function_create_avm1(c, rejected.bytes.begin(), rejected.bytes.size(), 0, 0, 0, &fallback_fn) == FVM_OK &&
          fvm_gc_pin(c, fallback_fn) == FVM_OK && fvm_execution_set_mode(c, FVM_EXECUTION_JIT, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long before = c->execution_mode.stats.native_entries;
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_function_invoke(c, fallback_fn, 0, 0, 0, &result) == FVM_OK && result.number == 77 &&
              c->execution_mode.stats.native_entries == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Program stops;
    stops.number(42);
    stops.op(0x3e);
    Program breakpoint;
    breakpoint.variable("owner");
    breakpoint_offset = breakpoint.bytes.size() + 5;
    with_block(breakpoint, stops);
    breakpoint.number(99);
    breakpoint.finish();
    FVM_Object *break_fn = 0;
    if (!(fvm_function_create_avm1(c, breakpoint.bytes.begin(), breakpoint.bytes.size(), 0, 0, 0, &break_fn) ==
              FVM_OK &&
          fvm_gc_pin(c, break_fn) == FVM_OK && fvm_function_invoke(c, break_fn, 0, 0, 0, &result) == FVM_OK &&
          result.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = c->execution_mode.stats.native_entries;
    breakpoint_hits = 0;
    if (!(fvm_debug_set_breakpoint_handler(c, with_breakpoint, 0) == FVM_OK &&
          fvm_debug_set_breakpoint(c, "<function>", breakpoint_offset, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, break_fn, 0, 0, 0, &result) == FVM_OK && result.number == 42 && breakpoint_hits == 1 &&
          c->execution_mode.stats.native_entries == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_set_breakpoint(c, "<function>", breakpoint_offset, 0) == FVM_OK &&
          fvm_function_invoke(c, break_fn, 0, 0, 0, &result) == FVM_OK && result.number == 42 &&
          c->execution_mode.stats.native_entries == before + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program nested;
    nested.number(42);
    nested.op(0x3e);
    for (unsigned i = 0; i < 64; ++i)
    {
        Program outer;
        outer.variable("owner");
        with_block(outer, nested);
        nested = outer;
    }
    class_mode = 2;
    if (!(with_run(c, nested).number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
