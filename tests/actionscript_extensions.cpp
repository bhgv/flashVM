#include "flashvm/extensions.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/worker.h"
#include "flashvm/table.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void byte(Bytes &b, unsigned int x)
{
    b.push_back((unsigned char)x);
}
static void word(Bytes &b, size_t n)
{
    byte(b, (unsigned int)n);
    byte(b, (unsigned int)(n >> 8));
}
static void text(Bytes &b, const char *s)
{
    do
    {
        byte(b, (unsigned char)*s);
    } while (*s++);
}
static void str(Bytes &b, const char *s)
{
    byte(b, 0x96);
    word(b, std::strlen(s) + 2);
    byte(b, 0);
    text(b, s);
}
static void number(Bytes &b, unsigned long n)
{
    byte(b, 0x96);
    word(b, 5);
    byte(b, 7);
    for (int i = 0; i < 4; ++i)
        byte(b, (unsigned int)(n >> (i * 8)));
}
static void var(Bytes &b, const char *s)
{
    str(b, s);
    byte(b, 0x1c);
}
static void call(Bytes &b, const char *receiver, const char *name, unsigned int n)
{
    number(b, n);
    var(b, receiver);
    str(b, name);
    byte(b, 0x52);
}
static void define(Bytes &b, const char *name, const char *param, const Bytes &body)
{
    byte(b, 0x9b);
    word(b, std::strlen(name) + 1 + 2 + (param ? std::strlen(param) + 1 : 0) + 2 + body.size());
    text(b, name);
    word(b, param ? 1 : 0);
    if (param)
        text(b, param);
    word(b, body.size());
    for (size_t i = 0; i < body.size(); ++i)
        byte(b, body[i]);
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value gc, thread, ch, handle, v, args[3];
    Bytes defs, body, code;
    unsigned long freed;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_extensions_install(c) == FVM_OK);
    assert(fvm_extensions_install(c) == FVM_OK);
    assert(fvm_get_global(c, "gc", &gc) == FVM_OK);
    assert(fvm_get_global(c, "thread", &thread) == FVM_OK);
    /* Script named send must not intercept an object's built-in method. */
    number(body, 999);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "send", 0, body);
    body.clear();
    var(body, "input");
    call(body, "sharedChannel", "send", 1);
    byte(body, 0x17);
    var(body, "input");
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "sender", "input", body);
    body.clear();
    str(body, "worker string");
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "stringResult", 0, body);
    body.clear();
    byte(body, 0x99);
    word(body, 2);
    word(body, 65531);
    byte(body, 0);
    define(defs, "forever", 0, body);
    body.clear();
    call(body, "sharedChannel", "receive", 0);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "blocked", 0, body);
    body.clear();
    str(body, "child");
    str(body, "forever");
    call(body, "thread", "run", 1);
    byte(body, 0x3c);
    number(body, 1);
    call(body, "sharedChannel", "send", 1);
    byte(body, 0x17);
    call(body, "child", "join", 0);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "nestedJoin", 0, body);
    body.clear();
    str(body, "local");
    number(body, 0);
    str(body, "Object");
    byte(body, 0x40);
    byte(body, 0x3c);
    call(body, "gc", "collect", 0);
    byte(body, 0x17);
    var(body, "local");
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "localObject", 0, body);
    body.clear();
    byte(defs, 0);
    assert(fvm_avm1_execute(c, &defs[0], defs.size()) == FVM_OK);
    /* Execute actual AVM1 for the public namespace and wrapper methods. */
    str(code, "pipe");
    number(code, 2);
    call(code, "channel", "create", 1);
    byte(code, 0x1d);
    number(code, 7);
    call(code, "pipe", "send", 1);
    byte(code, 0x17);
    str(code, "received");
    call(code, "pipe", "receive", 0);
    byte(code, 0x1d);
    call(code, "gc", "collect", 0);
    byte(code, 0x17);
    byte(code, 0);
    assert(fvm_avm1_execute(c, &code[0], code.size()) == FVM_OK);
    assert(fvm_get_global(c, "received", &v) == FVM_OK && v.number == 7);
    assert(fvm_get_global(c, "pipe", &ch) == FVM_OK);
    assert(fvm_value_string(&args[0], "sender") == FVM_OK);
    fvm_value_number(&args[1], 42);
    args[2] = ch;
    assert(fvm_object_call(c, thread.object, "run", 3, args, &handle) == FVM_OK);
    assert(fvm_gc_pin(c, handle.object) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK);
    assert(fvm_object_call(c, thread.object, "join", 1, &handle, &v) == FVM_OK && v.number == 42);
    assert(fvm_object_call(c, ch.object, "receive", 0, 0, &v) == FVM_OK && v.number == 42);
    assert(fvm_object_call(c, handle.object, "ret", 0, 0, &v) == FVM_OK && v.number == 42);
    assert(fvm_gc_unpin(c, handle.object) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK && freed == 1);
    assert(fvm_value_string(&args[0], "stringResult") == FVM_OK);
    assert(fvm_object_call(c, thread.object, "run_suspended", 1, args, &handle) == FVM_OK);
    assert(fvm_object_call(c, handle.object, "state", 0, 0, &v) == FVM_OK &&
           v.number == FVM_WORKER_SUSPENDED);
    assert(fvm_object_call(c, handle.object, "ret", 0, 0, &v) == FVM_OK &&
           v.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_call(c, handle.object, "join", 0, 0, &v) == FVM_OK &&
           std::strcmp(v.string, "worker string") == 0);
    assert(fvm_gc_collect(c, &freed) == FVM_OK && freed == 1);
    assert(std::strcmp(v.string, "worker string") == 0); /* Copied into caller context. */
    assert(fvm_value_string(&args[0], "forever") == FVM_OK);
    assert(fvm_object_call(c, thread.object, "run", 1, args, &handle) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK &&
           freed == 1); /* GC cancels and joins the orphan. */
    assert(fvm_value_string(&args[0], "blocked") == FVM_OK);
    fvm_value_undefined(&args[1]);
    args[2] = ch;
    assert(fvm_object_call(c, thread.object, "run", 3, args, &handle) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK &&
           freed == 1); /* Cancellation also exits receive polling. */
    fvm_value_number(&args[0], -1);
    assert(fvm_object_call(c, thread.object, "sleepms", 1, args, &v) == FVM_INVALID_ARGUMENT);
    fvm_value_number(&args[0], 0);
    assert(fvm_object_call(c, thread.object, "sleepus", 1, args, &v) == FVM_OK);
    assert(fvm_avm1_call(c, "stringResult", 0, 0, 0, &v) == FVM_OK);
    assert(std::strcmp(v.string, "worker string") == 0);
    assert(fvm_avm1_call(c, "localObject", 0, 0, 0, &v) == FVM_OK && v.type == FVM_VALUE_OBJECT);
    assert(fvm_gc_collect(c, &freed) == FVM_OK && freed == 1);
    assert(fvm_value_string(&args[0], "nestedJoin") == FVM_OK);
    fvm_value_undefined(&args[1]);
    args[2] = ch;
    assert(fvm_object_call(c, thread.object, "run", 3, args, &handle) == FVM_OK);
    assert(fvm_object_call(c, ch.object, "receive", 0, 0, &v) == FVM_OK && v.number == 1);
    assert(fvm_gc_collect(c, &freed) == FVM_OK &&
           freed == 1); /* Stop nested join and child safely. */
    /* Object inputs and results are copied with cycle identity preserved. */
    FVM_Object *source = 0, *child = 0;
    FVM_Value source_value, child_value, result, back;
    assert(fvm_object_create(c, &source) == FVM_OK);
    assert(fvm_object_create(c, &child) == FVM_OK);
    fvm_value_undefined(&source_value);
    source_value.type = FVM_VALUE_OBJECT;
    source_value.object = source;
    child_value = source_value;
    child_value.object = child;
    assert(fvm_object_set(c, source, "child", &child_value) == FVM_OK);
    assert(fvm_object_set(c, child, "parent", &source_value) == FVM_OK);
    assert(fvm_value_string(&v, "snapshot") == FVM_OK);
    assert(fvm_object_set(c, source, "label", &v) == FVM_OK);
    assert(fvm_value_string(&args[0], "sender") == FVM_OK);
    args[1] = source_value;
    args[2] = ch;
    assert(fvm_object_call(c, thread.object, "run_suspended", 3, args, &handle) == FVM_OK);
    assert(fvm_gc_pin(c, handle.object) == FVM_OK);
    assert(fvm_value_string(&v, "changed") == FVM_OK);
    assert(fvm_object_set(c, source, "label", &v) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK && freed == 2);
    assert(fvm_object_call(c, handle.object, "join", 0, 0, &result) == FVM_OK &&
           result.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, result.object, "label", &v) == FVM_OK &&
           std::strcmp(v.string, "snapshot") == 0);
    assert(fvm_object_get(c, result.object, "child", &child_value) == FVM_OK);
    assert(fvm_object_get(c, child_value.object, "parent", &back) == FVM_OK &&
           back.object == result.object);
    assert(fvm_gc_collect(c, &freed) == FVM_OK &&
           freed == 0); /* Cached result is traced by handle. */
    assert(fvm_object_call(c, handle.object, "ret", 0, 0, &v) == FVM_OK &&
           v.object == result.object);
    assert(fvm_object_call(c, ch.object, "receive", 0, 0, &v) == FVM_OK &&
           v.type == FVM_VALUE_OBJECT);
    assert(fvm_gc_unpin(c, handle.object) == FVM_OK);
    assert(fvm_gc_collect(c, &freed) == FVM_OK && freed == 5);
    FVM_Value handles[2];
    assert(fvm_value_string(&args[0], "stringResult") == FVM_OK);
    assert(fvm_object_call(c, thread.object, "run_suspended", 1, args, &handles[0]) == FVM_OK);
    handles[1] = handles[0];
    assert(fvm_object_call(c, thread.object, "join", 2, handles, &v) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_call(c, thread.object, "run_suspended", 1, args, &handles[1]) == FVM_OK);
    assert(fvm_object_call(c, thread.object, "join", 2, handles, &result) == FVM_OK);
    assert(fvm_table_get_index(c, result.object, 0, &v) == FVM_OK &&
           std::strcmp(v.string, "worker string") == 0);
    assert(fvm_table_get_index(c, result.object, 1, &v) == FVM_OK &&
           std::strcmp(v.string, "worker string") == 0);
    assert(fvm_object_call(c, gc.object, "count", 0, 0, &v) == FVM_OK && v.number >= 4);
    /* Isolated script workers install the numeric library when the parent opted in. */
    assert(fvm_standard_install(c) == FVM_OK);
    defs.clear();
    body.clear();
    number(body, 3);
    var(body, "input");
    call(body, "Math", "pow", 2);
    byte(body, 0x3e);
    byte(body, 0);
    define(defs, "standardWorker", "input", body);
    byte(defs, 0);
    assert(fvm_avm1_execute(c, &defs[0], defs.size()) == FVM_OK);
    assert(fvm_value_string(&args[0], "standardWorker") == FVM_OK);
    fvm_value_number(&args[1], 2);
    assert(fvm_object_call(c, thread.object, "run", 2, args, &handle) == FVM_OK);
    assert(fvm_object_call(c, handle.object, "join", 0, 0, &v) == FVM_OK && v.number == 8);
    fvm_context_destroy(c);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    fvm_value_number(&v, 123);
    assert(fvm_set_global(c, "gc", &v) == FVM_OK);
    assert(fvm_extensions_install(c) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(c, "gc", &v) == FVM_OK && v.number == 123);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    assert(fvm_avm1_execute(0, &defs[0], defs.size()) == FVM_INVALID_ARGUMENT);
    return 0;
}
