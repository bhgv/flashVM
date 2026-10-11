#include "flashvm/value.h"
#include "flashvm/gc.h"
#include "flashvm/userdata.h"
#include "flashvm/avm1.h"
#include "flashvm/transfer.h"
#include <assert.h>

static FVM_Object *receiver;
static unsigned int released, calls;
static void release(void *p)
{
    assert(p == &released);
    ++released;
}
static FVM_Result compute(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                          FVM_Value *out, void *userdata)
{
    FVM_Value base, capture;
    assert(self == receiver && n == 1 && args[0].number == 7);
    assert(fvm_object_get(c, self, "base", &base) == FVM_OK);
    assert(fvm_object_get(c, (FVM_Object *)userdata, "marker", &capture) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    ++calls;
    fvm_value_number(out, base.number + args[0].number + capture.number);
    return FVM_OK;
}
static FVM_Result second(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *userdata)
{
    (void)c;
    (void)self;
    (void)n;
    (void)args;
    (void)userdata;
    fvm_value_number(out, 20);
    return FVM_OK;
}
static FVM_Result swapping(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                           FVM_Value *out, void *userdata)
{
    (void)n;
    (void)args;
    (void)userdata;
    assert(fvm_object_define_method_rooted(c, self, "swap", second, 0, 0) == FVM_OK);
    fvm_value_number(out, 10);
    return FVM_OK;
}
int main(void)
{
    /* A same-name script function returns 42; inherited compute must win and
     * return 79. Installed from the existing AVM1 script-method fixture below. */
    static const unsigned char actions[] = {
        0x9b, 0x2b, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x00, 0x01, 0x00, 0x6e,
        0x00, 0x1d, 0x00, 0x96, 0x06, 0x00, 0x00, 0x74, 0x68, 0x69, 0x73, 0x00, 0x1c, 0x96,
        0x06, 0x00, 0x00, 0x62, 0x61, 0x73, 0x65, 0x00, 0x4e, 0x96, 0x03, 0x00, 0x00, 0x6e,
        0x00, 0x1c, 0x0a, 0x3e, 0x96, 0x1c, 0x00, 0x00, 0x72, 0x65, 0x73, 0x75, 0x6c, 0x74,
        0x00, 0x07, 0x07, 0x00, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x72, 0x65,
        0x63, 0x65, 0x69, 0x76, 0x65, 0x72, 0x00, 0x1c, 0x96, 0x09, 0x00, 0x00, 0x63, 0x6f,
        0x6d, 0x70, 0x75, 0x74, 0x65, 0x00, 0x52, 0x1d, 0x00};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *prototype = 0, *capture = 0, *foreign = 0, *chain[1025];
    FVM_Value v, args[1], out;
    FVM_Channel *channel = 0;
    unsigned int i;
    int own;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_object_create(c, &prototype) == FVM_OK);
    assert(fvm_object_create(c, &receiver) == FVM_OK);
    assert(fvm_object_create(c, &capture) == FVM_OK);
    assert(fvm_object_create(other, &foreign) == FVM_OK);
    assert(fvm_userdata_attach(c, capture, &released, release) == FVM_OK);
    fvm_value_number(&v, 37);
    assert(fvm_object_set(c, capture, "marker", &v) == FVM_OK);
    fvm_value_number(&v, 35);
    assert(fvm_object_set(c, receiver, "base", &v) == FVM_OK);
    assert(fvm_object_set_prototype(c, receiver, prototype) == FVM_OK);
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = receiver;
    assert(fvm_set_global(c, "receiver", &v) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, prototype, "compute", compute, capture, capture) ==
           FVM_OK);
    assert(fvm_object_has_own(c, prototype, "compute", &own) == FVM_OK && own);
    assert(fvm_object_has_own(c, receiver, "compute", &own) == FVM_OK && !own);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 0);
    fvm_value_number(args, 7);
    assert(fvm_object_call(c, receiver, "compute", 1, args, &out) == FVM_OK && out.number == 79);
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 79 && calls == 2);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &v, 0) == FVM_UNSUPPORTED);
    fvm_channel_destroy(channel);
    assert(fvm_object_define_method_rooted(c, receiver, "", second, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_method_rooted(c, receiver, "bad", 0, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_method_rooted(c, foreign, "bad", second, 0, 0) ==
           FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_method_rooted(c, receiver, "bad", second, 0, foreign) ==
           FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_method_rooted(c, receiver, "base", second, 0, 0) ==
           FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_accessor_rooted(c, receiver, "blocked", second, 0, 0, 0) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, receiver, "blocked", second, 0, 0) ==
           FVM_INVALID_ARGUMENT);
    /* Own callback overrides inherited callback. Replacing the prototype entry
     * releases its old capture, even while the prototype remains reachable. */
    assert(fvm_object_define_method_rooted(c, receiver, "compute", second, 0, 0) == FVM_OK);
    assert(fvm_object_call(c, receiver, "compute", 0, 0, &out) == FVM_OK && out.number == 20);
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 20);
    assert(fvm_object_define_method_rooted(c, prototype, "compute", second, 0, 0) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 1);
    /* Own data/accessors stop lookup and legacy global-function fallback. */
    fvm_value_number(&v, 99);
    assert(fvm_object_set(c, receiver, "compute", &v) == FVM_OK);
    assert(fvm_object_call(c, receiver, "compute", 0, 0, &out) == FVM_UNSUPPORTED);
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_UNSUPPORTED);
    assert(fvm_object_call(c, receiver, "blocked", 0, 0, &out) == FVM_UNSUPPORTED);
    assert(fvm_object_define_method_rooted(c, prototype, "masked", second, 0, 0) == FVM_OK);
    assert(fvm_register_function(c, "masked", second, 0) == FVM_OK);
    assert(fvm_object_set(c, receiver, "masked", &v) == FVM_OK);
    assert(fvm_object_call(c, receiver, "masked", 0, 0, &out) == FVM_UNSUPPORTED);
    assert(fvm_register_function(c, "fallback", second, 0) == FVM_OK);
    assert(fvm_object_call(c, receiver, "fallback", 0, 0, &out) == FVM_OK && out.number == 20);
    assert(fvm_object_define_method_rooted(c, receiver, "swap", swapping, 0, 0) == FVM_OK);
    assert(fvm_object_call(c, receiver, "swap", 0, 0, &out) == FVM_OK && out.number == 10);
    assert(fvm_object_call(c, receiver, "swap", 0, 0, &out) == FVM_OK && out.number == 20);
    /* Bounded resolution matches the 1024-object property-chain budget. */
    for (i = 0; i < 1025; ++i)
    {
        assert(fvm_object_create(c, &chain[i]) == FVM_OK);
        if (i)
            assert(fvm_object_set_prototype(c, chain[i], chain[i - 1]) == FVM_OK);
    }
    assert(fvm_gc_pin(c, chain[1024]) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, chain[0], "depth", second, 0, 0) == FVM_OK);
    assert(fvm_object_call(c, chain[1023], "depth", 0, 0, &out) == FVM_OK && out.number == 20);
    assert(fvm_object_call(c, chain[1024], "depth", 0, 0, &out) == FVM_ERROR);
    assert(fvm_object_call(c, chain[1024], "fallback", 0, 0, &out) == FVM_ERROR);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, chain[1023], "depth", 0, 0, &out) == FVM_OK);
    assert(fvm_gc_unpin(c, chain[1024]) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    assert(released == 1);
    return 0;
}
