/* Verify opt-in frame-free posts using a frozen clock and the real scheduler. */
#include "timers_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include "flashvm/async.h"
#include "flashvm/stage.h"


/* Explicitly terminate on a broken contract, keeping calls outside assertions. */
static void check(bool valid, const char *message)
{
    if (!valid)
    {
        std::fprintf(stderr, "Async: %s\n", message);
        std::exit(1);
    }
}


/* Owner-local test state; queue/callback references are rooted by the scheduler. */
struct Probe
{
    FVM_Object *function; /* Borrow while pending/active callback retains itself. */
    unsigned calls; /* Actual callback invocation count. */
    bool again; /* Repost once during delivery to test next-poll isolation. */
};


/* Observe copied args, request redraw and optionally repost without recursion.
 * Context/args/out are borrowed. No timeline advancement or native worker is used. */
static FVM_Result posted(FVM_Context *c, FVM_Object *, unsigned count,
                          const FVM_Value *args, FVM_Value *out, void *user)
{
    Probe *probe = static_cast<Probe *>(user);
    ++probe->calls;
    check(count == 1 && args[0].type == FVM_VALUE_STRING &&
          fvm_value_string_size(&args[0]) == 3 && !std::memcmp(args[0].string, "old", 3), "copied args");
    FVM_Value ignored;
    check(fvm_call(c, "updateAfterEvent", 0, 0, &ignored) == FVM_OK, "callback redraw");
    unsigned delivered = 77;
    check(fvm_timers_poll(c, &delivered) == FVM_INVALID_ARGUMENT && delivered == 77, "nested poll");
    if (probe->again)
    {
        unsigned long id;
        probe->again = false;
        check(fvm_async_post(c, probe->function, count, args, &id) == FVM_OK, "repost");
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Check C89 bridge and script method dispatch with no clock/frame advancement.
 * Own every context/runtime; pending/active GC roots must survive host collection. */
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    check(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK, "runtime");
    FVM_Context *c = timer_context(runtime);
    check(fvm_async_install(c) == FVM_OK && fvm_async_install(c) == FVM_INVALID_ARGUMENT, "explicit install conflict");
    Probe probe = {0, 0, true};
    check(fvm_function_create_native(c, posted, &probe, 0, &probe.function) == FVM_OK, "function");
    char text[] = "old";
    FVM_Value arg;
    check(fvm_value_string_bytes(&arg, text, 3) == FVM_OK, "argument");
    unsigned long first = 777;
    check(fvm_async_post(c, probe.function, 1, &arg, &first) == FVM_OK && probe.calls == 0, "deferred post");
    text[0] = 'X';
    check(fvm_gc_collect(c, 0) == FVM_OK, "pending roots");
    unsigned long wait = 99;
    int pending = 0;
    check(fvm_timers_next_delay(c, 1000, &wait, &pending) == FVM_OK && wait == 0 && pending, "wake immediately");
    check(timer_poll(c, 1000) == 1 && probe.calls == 1, "frozen clock, first poll");
    check(timer_poll(c, 1000) == 1 && probe.calls == 2, "repost waits for next poll");
    int redraw = 0;
    check(fvm_stage_take_update(c, &redraw) == FVM_OK && redraw, "coalesced redraw");
    FVM_Value space;
    FVM_Value args[2];
    FVM_Value id;
    check(fvm_get_global(c, "Async", &space) == FVM_OK, "namespace");
    args[0] = timer_object(probe.function);
    check(fvm_value_string_bytes(&args[1], "old", 3) == FVM_OK, "script argument");
    check(fvm_object_call(c, space.object, "post", 2, args, &id) == FVM_OK, "script post");
    check(fvm_object_call(c, space.object, "cancel", 1, &id, &arg) == FVM_OK && timer_poll(c, 1000) == 0, "script cancel");
    first = 777;
    check(fvm_async_post(c, 0, 0, 0, &first) == FVM_INVALID_ARGUMENT && first == 777, "atomic rejection");
    check(fvm_async_cancel(c, 0) == FVM_OK, "missing ID");
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    std::puts("PASS Async.post/cancel: frozen frames/clock, roots, copy, next poll and script methods");
    return 0;
}
