#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include "flashvm/mutex.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void write_values(const char *path)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *error = 0, *lock = 0;
    FVM_Mutex *mutex = 0;
    FVM_Channel *file = 0;
    FVM_Value message, value;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_string(&message, "persisted Error");
    assert(fvm_construct(c, "Error", 1, &message, &error) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = error;
    assert(fvm_object_set(c, error, "self", &value) == FVM_OK);
    assert(fvm_channel_open_file(path, "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(c, file, &value, 0) == FVM_OK);
    assert(fvm_mutex_create(c, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(c, mutex, &lock) == FVM_OK);
    assert(fvm_mutex_close(mutex) == FVM_OK);
    assert(fvm_object_call(c, lock, "touch", 0, 0, &value) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &value) == FVM_OK);
    /* Only the error/context snapshot travels, never the closed native handle. */
    assert(fvm_channel_send_value(c, file, &value, 0) == FVM_OK);
    assert(fvm_exception_clear(c) == FVM_OK);
    fvm_value_number(&message, 4095);
    assert(fvm_construct(c, "Number", 1, &message, &error) == FVM_OK);
    value.type = FVM_VALUE_OBJECT;
    value.object = error;
    assert(fvm_channel_send_value(c, file, &value, 0) == FVM_OK);
    fvm_channel_release(file);
    fvm_mutex_release(mutex);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
static void read_values(const char *path)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Channel *file = 0;
    FVM_Value value, self, text, code;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    /* No standard installation or producer address/callback registry. */
    assert(fvm_channel_open_file(path, "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(c, file, &value, 0) == FVM_OK);
    assert(fvm_gc_pin(c, value.object) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, value.object, "toString", 0, 0, &text) == FVM_OK);
    assert(!strcmp(text.string, "persisted Error"));
    assert(fvm_object_get(c, value.object, "self", &self) == FVM_OK && self.object == value.object);
    assert(fvm_channel_receive_value(c, file, &value, 0) == FVM_OK);
    assert(fvm_gc_pin(c, value.object) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, value.object, "toString", 0, 0, &text) == FVM_OK);
    assert(!strcmp(text.string, "resource.closed: Resource is closed."));
    assert(fvm_object_get(c, value.object, "code", &code) == FVM_OK && code.number == 1001);
    assert(fvm_object_get(c, value.object, "context", &self) == FVM_OK &&
           self.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, self.object, "resourceKind", &text) == FVM_OK &&
           !strcmp(text.string, "mutex"));
    assert(fvm_channel_receive_value(c, file, &value, 0) == FVM_OK);
    assert(fvm_gc_pin(c, value.object) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, value.object, "valueOf", 0, 0, &code) == FVM_OK &&
           code.number == 4095);
    fvm_value_number(&code, 16);
    assert(fvm_object_call(c, value.object, "toString", 1, &code, &text) == FVM_OK &&
           !strcmp(text.string, "fff"));
    fvm_channel_release(file);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
int main(int argc, char **argv)
{
    if (argc == 3)
    {
        if (!strcmp(argv[1], "write"))
            write_values(argv[2]);
        else
        {
            assert(!strcmp(argv[1], "read"));
            read_values(argv[2]);
        }
    }
    else
    {
        assert(argc == 1);
        write_values("error-transfer-test.dat");
        read_values("error-transfer-test.dat");
        assert(remove("error-transfer-test.dat") == 0);
    }
    return 0;
}
