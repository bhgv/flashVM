#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include <assert.h>
#include <string.h>
static FVM_Result custom(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *userdata)
{
    (void)c;
    (void)self;
    (void)n;
    (void)args;
    (void)userdata;
    return fvm_value_string(out, "custom message");
}
static FVM_Result reject(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *userdata)
{
    FVM_Value value;
    (void)self;
    (void)n;
    (void)args;
    (void)out;
    (void)userdata;
    fvm_value_string(&value, "constructor setter failed");
    return fvm_exception_raise(c, &value);
}
static void text(FVM_Context *c, FVM_Object *object, const char *name, const char *expected)
{
    FVM_Value value;
    assert(fvm_object_get(c, object, name, &value) == FVM_OK);
    assert(value.type == FVM_VALUE_STRING && !strcmp(value.string, expected));
}
static void stringify(FVM_Context *c, FVM_Object *object, const char *expected)
{
    FVM_Value value;
    assert(fvm_object_call(c, object, "toString", 0, 0, &value) == FVM_OK);
    assert(value.type == FVM_VALUE_STRING && !strcmp(value.string, expected));
}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *error = 0, *prototype = 0, *plain = 0, *lock = 0, *foreign = 0, *replacement = 0;
    FVM_Mutex *mutex = 0;
    FVM_Channel *channel = 0;
    FVM_Value value, klass, saved_proto, args[2], copy;
    int own, matches;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_construct(c, "Error", 0, 0, &error) == FVM_UNSUPPORTED && !error);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Error", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, klass.object, "prototype", &saved_proto) == FVM_OK);
    prototype = saved_proto.object;
    text(c, prototype, "name", "Error");
    text(c, prototype, "message", "Error");
    stringify(c, prototype, "Error");
    assert(fvm_construct(c, "Error", 0, 0, &error) == FVM_OK);
    assert(fvm_gc_pin(c, error) == FVM_OK);
    assert(fvm_object_has_own(c, error, "message", &own) == FVM_OK && !own);
    assert(fvm_object_has_own(c, error, "toString", &own) == FVM_OK && !own);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = error;
    assert(fvm_value_instance_of(c, &value, &klass, &matches) == FVM_OK && matches);
    fvm_value_string(&value, "Error");
    assert(fvm_value_instance_of(c, &value, &klass, &matches) == FVM_OK && !matches);
    text(c, error, "name", "Error");
    stringify(c, error, "Error");
    fvm_value_string(&value, "ChangedName");
    assert(fvm_object_set(c, error, "name", &value) == FVM_OK);
    stringify(c, error, "Error"); /* AS2 returns message, without the name. */
    fvm_value_string(&value, "");
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    stringify(c, error, "");
    fvm_value_number(&value, 0.1);
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    stringify(c, error, "0.1");
    fvm_value_boolean(&value, 1);
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    stringify(c, error, "true");
    fvm_value_undefined(&value);
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    stringify(c, error, "undefined");
    args[0] = value;
    assert(fvm_construct(c, "Error", 1, args, &plain) == FVM_OK);
    assert(fvm_object_has_own(c, plain, "message", &own) == FVM_OK && !own);
    stringify(c, plain, "Error");
    fvm_value_string(args, "first");
    fvm_value_string(args + 1, "ignored");
    assert(fvm_construct(c, "Error", 2, args, &plain) == FVM_OK);
    text(c, plain, "message", "first");
    assert(fvm_object_has_own(c, plain, "message", &own) == FVM_OK && own);
    assert(fvm_object_create(c, &plain) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, plain, "toString", custom, 0, 0) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = plain;
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    stringify(c, error, "custom message");
    assert(fvm_object_set(c, error, "message", &klass) == FVM_OK);
    stringify(c, error, "[type Function]");
    /* Prototype and native constructor capture survive removing the global. */
    fvm_value_undefined(&value);
    assert(fvm_set_global(c, "Error", &value) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_construct(c, "Error", 0, 0, &plain) == FVM_OK);
    stringify(c, plain, "Error");
    assert(fvm_set_global(c, "Error", &klass) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    /* Constructor uses the current prototype and propagates setter throws. */
    assert(fvm_object_create(c, &replacement) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, replacement, "message", 0, reject, 0, 0) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = replacement;
    assert(fvm_object_set(c, klass.object, "prototype", &value) == FVM_OK);
    fvm_value_string(args, "setter input");
    assert(fvm_construct(c, "Error", 1, args, &plain) == FVM_EXCEPTION && !plain);
    assert(fvm_exception_get(c, &value) == FVM_OK &&
           !strcmp(value.string, "constructor setter failed"));
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_object_set(c, klass.object, "prototype", &saved_proto) == FVM_OK);
    /* Cycles and aliases, inherited callback, and metadata survive memory/file transfer. */
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    fvm_value_string(&value, "transported");
    assert(fvm_object_set(c, error, "message", &value) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = error;
    assert(fvm_object_set(c, error, "self", &value) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    stringify(other, copy.object, "transported");
    assert(fvm_object_get(other, copy.object, "constructor", &value) == FVM_OK);
    assert(fvm_value_instance_of(other, &copy, &value, &matches) == FVM_OK && matches);
    assert(fvm_object_get(other, copy.object, "self", &value) == FVM_OK &&
           value.object == copy.object);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    stringify(other, copy.object, "transported");
    fvm_channel_release(channel);
    /* Standalone resource exceptions stringify and serialize without installation. */
    assert(fvm_mutex_create(other, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(other, mutex, &lock) == FVM_OK);
    assert(fvm_mutex_close(mutex) == FVM_OK);
    assert(fvm_object_call(other, lock, "touch", 0, 0, &value) == FVM_EXCEPTION);
    assert(fvm_exception_get(other, &value) == FVM_OK);
    stringify(other, value.object, "resource.closed: Resource is closed.");
    assert(fvm_exception_clear(other) == FVM_OK);
    fvm_mutex_release(mutex);
    /* Foreign arguments and conflicting globals reject without partial installation. */
    assert(fvm_object_create(other, &foreign) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = foreign;
    assert(fvm_construct(c, "Error", 1, &value, &plain) == FVM_INVALID_ARGUMENT && !plain);
    fvm_value_number(&value, 17);
    assert(fvm_set_global(other, "Error", &value) == FVM_OK);
    assert(fvm_standard_install(other) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(other, "Math", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
