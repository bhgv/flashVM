#include "../core/runtime.hpp"
#include "flashvm/extensions.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/avm1.h"
#include "flashvm/worker.h"
#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include "../../ports/thread_port.h"
#include <new>
#include <climits>
#include <cstring>

namespace
{
enum
{
    CHANNEL = 1,
    WORKER = 2,
    GC_NAMESPACE = 3,
    THREAD_NAMESPACE = 4,
    CHANNEL_NAMESPACE = 5
};
struct ScriptTask
{
    FVM_Worker *handle;
    bool standard_library;
    fvm_detail::string name;
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction> functions;
    FVM_Channel *shared, *input, *output;
    ScriptTask() : handle(0), standard_library(false), shared(0), input(0), output(0)
    {
    }
};
static void release_channel(void *p)
{
    fvm_channel_release((FVM_Channel *)p);
}
static void release_worker(void *p)
{
    ScriptTask *t = (ScriptTask *)p;
    if (t->handle)
    {
        fvm_worker_stop(t->handle);
        fvm_worker_join(t->handle, 0);
        fvm_worker_destroy(t->handle);
    }
    if (t->shared)
        fvm_channel_release(t->shared);
    if (t->input)
        fvm_channel_release(t->input);
    if (t->output)
        fvm_channel_release(t->output);
    delete t;
}
static bool stopped(FVM_Context *c)
{
    int value = 0;
    return c->worker && fvm_worker_stop_requested(c->worker, &value) == FVM_OK && value;
}
static bool integer_arg(const FVM_Value &v, unsigned long max, unsigned long *out)
{
    if (v.type != FVM_VALUE_NUMBER || !(v.number >= 0) || v.number >= (double)max)
        return false;
    *out = (unsigned long)v.number;
    return (double)*out == v.number;
}
static void method(FVM_Object *o, const char *name, FVM_NativeFunction cb, int op,
                   bool collector = false)
{
    FVM_NativeEntry e;
    e.callback = cb;
    e.userdata = 0;
    e.collector_entry = collector;
    (void)op;
    o->methods[name] = e;
}
/* Distinct wrappers below pass operation codes without pointer casts. */
static FVM_Result gc_op(FVM_Context *c, unsigned int n, const FVM_Value *a, FVM_Value *out, int op)
{
    unsigned long count = 0;
    int state = 0;
    FVM_Result r;
    if (op == 0 || op == 1 || op == 3 || op == 4)
    {
        if (n)
            return FVM_INVALID_ARGUMENT;
        if (op == 0)
            r = fvm_gc_collect(c, &count);
        else if (op == 1)
            r = fvm_gc_step(c, &count);
        else if (op == 3)
        {
            r = fvm_gc_state(c, &state);
            count = (unsigned long)state;
        }
        else
            r = fvm_gc_object_count(c, &count);
        fvm_value_number(out, op == 3 ? (double)state : (double)count);
        return r;
    }
    if (n != 1 || a[0].type != FVM_VALUE_NUMBER || !(a[0].number >= -100) ||
        a[0].number > FVM_GC_FULL || a[0].number != (int)a[0].number)
        return FVM_INVALID_ARGUMENT;
    r = fvm_gc_mode(c, (int)a[0].number);
    fvm_value_undefined(out);
    return r;
}
#define GC_WRAPPER(name, op)                                                                       \
    static FVM_Result name(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,       \
                           FVM_Value *out, void *)                                                 \
    {                                                                                              \
        return gc_op(c, n, a, out, op);                                                            \
    }
GC_WRAPPER(gc_collect, 0)
GC_WRAPPER(gc_step, 1)
GC_WRAPPER(gc_mode, 2)
GC_WRAPPER(gc_state, 3)
GC_WRAPPER(gc_count, 4)
static FVM_Result channel_op(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                             FVM_Value *out, int op)
{
    if (self->resource_kind != CHANNEL || !self->resource)
        return FVM_INVALID_ARGUMENT;
    FVM_Channel *ch = (FVM_Channel *)self->resource;
    FVM_Result r;
    unsigned long capacity;
    size_t pending;
    int ready;
    FVM_Value v;
    if (op == 0 || op == 1)
    {
        if (n != 1)
            return FVM_INVALID_ARGUMENT;
        do
        {
            r = fvm_channel_send_value(c, ch, &a[0], 0);
            if (r == FVM_NOT_FOUND && op == 0 && !stopped(c))
                fvm_port_thread_sleep_us(1000);
            else
                break;
        } while (true);
        if (r == FVM_NOT_FOUND && stopped(c))
            return FVM_ERROR;
        if (op == 1 && r == FVM_NOT_FOUND)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        fvm_value_boolean(out, r == FVM_OK);
        return r;
    }
    if (op == 2 || op == 3)
    {
        if (n)
            return FVM_INVALID_ARGUMENT;
        do
        {
            r = fvm_channel_receive_value(c, ch, &v, 0);
            int file = 0;
            fvm_channel_is_file(ch, &file);
            if (r == FVM_NOT_FOUND && op == 2 && file)
                return FVM_ERROR;
            if (r == FVM_NOT_FOUND && op == 2 && !stopped(c))
                fvm_port_thread_sleep_us(1000);
            else
                break;
        } while (true);
        if (r == FVM_NOT_FOUND && stopped(c))
            return FVM_ERROR;
        if (r == FVM_NOT_FOUND && op == 3)
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        if (r != FVM_OK)
            return r;
        *out = v;
        return FVM_OK;
    }
    if (op == 4)
    {
        if (n)
            return FVM_INVALID_ARGUMENT;
        fvm_channel_close(ch);
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (op == 5)
    {
        if (n)
            return FVM_INVALID_ARGUMENT;
        r = fvm_channel_available(ch, &ready);
        fvm_value_boolean(out, ready);
        return r;
    }
    if (op == 6)
    {
        if (n)
            return FVM_INVALID_ARGUMENT;
        r = fvm_channel_pending(ch, &pending);
        fvm_value_number(out, (double)pending);
        return r;
    }
    if (n != 1 || !integer_arg(a[0], ULONG_MAX, &capacity) || (size_t)capacity != capacity)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    return fvm_channel_set_capacity(ch, (size_t)capacity);
}
#define CHANNEL_WRAPPER(name, op)                                                                  \
    static FVM_Result name(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,      \
                           FVM_Value *out, void *)                                                 \
    {                                                                                              \
        return channel_op(c, s, n, a, out, op);                                                    \
    }
CHANNEL_WRAPPER(ch_send, 0)
CHANNEL_WRAPPER(ch_try_send, 1)
CHANNEL_WRAPPER(ch_receive, 2)
CHANNEL_WRAPPER(ch_try_receive, 3)
CHANNEL_WRAPPER(ch_close, 4)
CHANNEL_WRAPPER(ch_ready, 5)
CHANNEL_WRAPPER(ch_pending, 6)
CHANNEL_WRAPPER(ch_capacity, 7)
static FVM_Result create_channel(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                                 FVM_Value *out, void *)
{
    unsigned long capacity = 0;
    FVM_Channel *ch = 0;
    FVM_Object *o = 0;
    FVM_Result r;
    if (n >= 1 && a[0].type == FVM_VALUE_STRING && a[0].string && !std::strcmp(a[0].string, "file"))
    {
        if ((n != 2 && n != 3) || a[1].type != FVM_VALUE_STRING || !a[1].string ||
            (n == 3 && (a[2].type != FVM_VALUE_STRING || !a[2].string)))
            return FVM_INVALID_ARGUMENT;
        r = fvm_channel_open_file(a[1].string, n == 3 ? a[2].string : 0, &ch);
    }
    else
    {
        if (n > 1 ||
            (n && (!integer_arg(a[0], ULONG_MAX, &capacity) || (size_t)capacity != capacity)))
            return FVM_INVALID_ARGUMENT;
        r = fvm_channel_create(&ch);
    }
    if (r != FVM_OK)
        return r;
    int file = 0;
    fvm_channel_is_file(ch, &file);
    if (!file)
        fvm_channel_set_capacity(ch, (size_t)capacity);
    r = fvm_extensions_wrap_channel(c, ch, &o);
    fvm_channel_release(ch);
    if (r == FVM_OK)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = o;
    }
    return r;
}
/* Returning the attached value lets script assignment preserve primitive metadata. */
static FVM_Result meta_attach(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                              FVM_Value *out, void *)
{
    if (n != 2 || (a[1].type != FVM_VALUE_OBJECT && a[1].type != FVM_VALUE_NULL &&
                   a[1].type != FVM_VALUE_UNDEFINED))
        return FVM_INVALID_ARGUMENT;
    FVM_Value value = a[0];
    FVM_Result result =
        fvm_value_set_metatable(c, &value, a[1].type == FVM_VALUE_OBJECT ? a[1].object : 0);
    if (result == FVM_OK)
        *out = value;
    return result;
}
static FVM_Result meta_get(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                           FVM_Value *out, void *)
{
    if (n != 1)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *meta = 0;
    FVM_Result result = fvm_value_get_metatable(c, a, &meta);
    fvm_value_undefined(out);
    if (meta)
    {
        out->type = FVM_VALUE_OBJECT;
        out->object = meta;
    }
    return result;
}
static FVM_Result worker_op(FVM_Context *c, FVM_Object *s, unsigned int n, FVM_Value *out, int op)
{
    if (n || s->resource_kind != WORKER || !s->resource)
        return FVM_INVALID_ARGUMENT;
    FVM_Worker *w = ((ScriptTask *)s->resource)->handle;
    FVM_Result r;
    FVM_Value v;
    FVM_WorkerState state;
    if (op == 0)
    {
        fvm_value_undefined(out);
        return fvm_worker_start(w);
    }
    if (op == 1)
    {
        fvm_value_undefined(out);
        return fvm_worker_stop(w);
    }
    if (op == 2)
    {
        r = fvm_worker_state(w, &state);
        fvm_value_number(out, (double)state);
        return r;
    }
    if (op == 3 && c->worker)
    {
        fvm_worker_start(w);
        while ((r = fvm_worker_result(w, &v)) == FVM_NOT_FOUND)
        {
            if (stopped(c))
            {
                fvm_worker_stop(w);
                return FVM_ERROR;
            }
            fvm_port_thread_sleep_us(1000);
        }
    }
    r = op == 3 ? fvm_worker_join(w, &v) : fvm_worker_result(w, &v);
    if (r == FVM_NOT_FOUND)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (r != FVM_OK)
        return r;
    if (s->resource_result_valid)
    {
        *out = s->resource_result;
        return FVM_OK;
    }
    ScriptTask *task = (ScriptTask *)s->resource;
    r = fvm_channel_receive_value(c, task->output, &v, 0);
    if (r != FVM_OK)
        return r;
    s->resource_result = v;
    s->resource_result_valid = true;
    *out = v;
    return FVM_OK;
}
#define WORKER_WRAPPER(name, op)                                                                   \
    static FVM_Result name(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *,       \
                           FVM_Value *out, void *)                                                 \
    {                                                                                              \
        return worker_op(c, s, n, out, op);                                                        \
    }
WORKER_WRAPPER(w_start, 0)
WORKER_WRAPPER(w_stop, 1)
WORKER_WRAPPER(w_state, 2)
WORKER_WRAPPER(w_join, 3)
WORKER_WRAPPER(w_ret, 4)
static FVM_Result script_worker(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *ud)
{
    ScriptTask *t = (ScriptTask *)ud;
    c->worker = t->handle;
    if (t->standard_library)
    {
        FVM_Result r = fvm_standard_install(c);
        if (r != FVM_OK)
            return r;
    }
    try
    {
        c->script_functions = t->functions;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Result r = fvm_extensions_install(c);
    if (r != FVM_OK)
        return r;
    if (t->shared)
    {
        FVM_Object *ch = 0;
        FVM_Value v;
        r = fvm_extensions_wrap_channel(c, t->shared, &ch);
        if (r != FVM_OK)
            return r;
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = ch;
        r = fvm_set_global(c, "sharedChannel", &v);
        if (r != FVM_OK)
            return r;
    }
    (void)in;
    FVM_Value argument, result;
    r = fvm_channel_receive_value(c, t->input, &argument, 0);
    if (r != FVM_OK)
        return r;
    r = fvm_avm1_call(c, t->name.c_str(), 0, 1, &argument, &result);
    if (r != FVM_OK)
        return r;
    r = fvm_channel_send_value(c, t->output, &result, 0);
    fvm_value_undefined(out);
    return r;
}
static FVM_Result run_worker(FVM_Context *c, unsigned int n, const FVM_Value *a, FVM_Value *out,
                             bool suspended)
{
    if (n < 1 || n > 3 || a[0].type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    if (c->script_functions.find(a[0].string) == c->script_functions.end())
        return FVM_NOT_FOUND;
    if (n == 3 && (a[2].type != FVM_VALUE_OBJECT || a[2].object->resource_kind != CHANNEL))
        return FVM_INVALID_ARGUMENT;
    ScriptTask *t = 0;
    try
    {
        t = new ScriptTask;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    try
    {
        t->name = a[0].string;
        t->functions = c->script_functions;
    }
    catch (...)
    {
        delete t;
        return FVM_OUT_OF_MEMORY;
    }
    if (n == 3)
    {
        t->shared = (FVM_Channel *)a[2].object->resource;
        FVM_Result r = fvm_channel_retain(t->shared);
        if (r != FVM_OK)
        {
            delete t;
            return r;
        }
    }
    FVM_Value in, argument;
    fvm_value_undefined(&in);
    argument = in;
    if (n >= 2)
        argument = a[1];
    FVM_Result r = fvm_channel_create(&t->input);
    if (r == FVM_OK)
        r = fvm_channel_create(&t->output);
    if (r == FVM_OK)
        r = fvm_channel_send_value(c, t->input, &argument, 0);
    if (r != FVM_OK)
    {
        release_worker(t);
        return r;
    }
    r = fvm_worker_run_suspended(&c->runtime->config, script_worker, &in, t, &t->handle);
    if (r != FVM_OK)
    {
        release_worker(t);
        return r;
    }
    FVM_Object *o = 0;
    r = fvm_object_create(c, &o);
    if (r != FVM_OK)
    {
        release_worker(t);
        return r;
    }
    t->standard_library = c->standard_library;
    o->resource = t;
    o->resource_kind = WORKER;
    o->release_resource = release_worker;
    try
    {
        method(o, "start", w_start, 0);
        method(o, "stop", w_stop, 1);
        method(o, "state", w_state, 2);
        method(o, "join", w_join, 3);
        method(o, "ret", w_ret, 4);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!suspended)
        fvm_worker_start(t->handle);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}
static FVM_Result run(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{
    return run_worker(c, n, a, out, false);
}
static FVM_Result run_suspended(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                                FVM_Value *out, void *)
{
    return run_worker(c, n, a, out, true);
}
static FVM_Result stop_requested(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *,
                                 FVM_Value *out, void *)
{
    if (n)
        return FVM_INVALID_ARGUMENT;
    fvm_value_boolean(out, stopped(c));
    return FVM_OK;
}
static FVM_Result sleep_op(FVM_Context *c, unsigned int n, const FVM_Value *a, FVM_Value *out,
                           unsigned long scale)
{
    if (n != 1 || a[0].type != FVM_VALUE_NUMBER || !(a[0].number >= 0) ||
        a[0].number >= (double)(ULONG_MAX / scale))
        return FVM_INVALID_ARGUMENT;
    unsigned long us = (unsigned long)(a[0].number * scale);
    while (us && !stopped(c))
    {
        unsigned long chunk = us > 1000UL ? 1000UL : us;
        fvm_port_thread_sleep_us(chunk);
        us -= chunk;
    }
    fvm_value_undefined(out);
    return stopped(c) ? FVM_ERROR : FVM_OK;
}
static FVM_Result sleep(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    return sleep_op(c, n, a, out, 1000000UL);
}
static FVM_Result sleepms(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                          FVM_Value *out, void *)
{
    return sleep_op(c, n, a, out, 1000UL);
}
static FVM_Result sleepus(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                          FVM_Value *out, void *)
{
    return sleep_op(c, n, a, out, 1UL);
}
static FVM_Result join(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *)
{
    if (!n)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < n; ++i)
    {
        if (a[i].type != FVM_VALUE_OBJECT || a[i].object->resource_kind != WORKER)
            return FVM_INVALID_ARGUMENT;
        for (unsigned int j = 0; j < i; ++j)
            if (a[i].object == a[j].object)
                return FVM_INVALID_ARGUMENT;
    }
    if (n == 1)
        return worker_op(c, a[0].object, 0, out, 3);
    FVM_Object *results = 0;
    FVM_Result first = fvm_object_create(c, &results);
    if (first != FVM_OK)
        return first;
    for (unsigned int i = 0; i < n; ++i)
    {
        FVM_Value v;
        fvm_value_undefined(&v);
        FVM_Result r = worker_op(c, a[i].object, 0, &v, 3);
        if (first == FVM_OK && r != FVM_OK)
            first = r;
        r = fvm_table_set_index(c, results, i, &v);
        if (first == FVM_OK && r != FVM_OK)
            first = r;
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = results;
    return first;
}
} // namespace
extern "C" FVM_Result fvm_extensions_wrap_channel(FVM_Context *c, FVM_Channel *ch, FVM_Object **out)
{
    if (!c || !ch || !out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_Object *o = 0;
    FVM_Result r = fvm_object_create(c, &o);
    if (r != FVM_OK)
        return r;
    r = fvm_channel_retain(ch);
    if (r != FVM_OK)
        return r;
    o->resource = ch;
    o->resource_kind = CHANNEL;
    o->release_resource = release_channel;
    try
    {
        method(o, "send", ch_send, 0);
        method(o, "trySend", ch_try_send, 1);
        method(o, "receive", ch_receive, 2);
        method(o, "tryReceive", ch_try_receive, 3);
        method(o, "close", ch_close, 4);
        method(o, "ready", ch_ready, 5);
        method(o, "pending", ch_pending, 6);
        method(o, "capacity", ch_capacity, 7);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    *out = o;
    return FVM_OK;
}
extern "C" FVM_Result fvm_extensions_install(FVM_Context *c)
{
    if (!c || c->destroying)
        return FVM_INVALID_ARGUMENT;
    const char *names[4] = {"gc", "thread", "channel", "metatable"};
    FVM_Object *objects[4];
    FVM_Value v;
    int existing = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (c->global->properties.find(names[i]) != c->global->properties.end())
        {
            v = c->global->properties.find(names[i])->second;
            if (v.type != FVM_VALUE_OBJECT ||
                v.object->resource_kind != (i == 3 ? 7 : GC_NAMESPACE + i))
                return FVM_INVALID_ARGUMENT;
            ++existing;
        }
    }
    if (existing)
        return existing == 4 ? FVM_OK : FVM_INVALID_ARGUMENT;
    try
    {
        for (int i = 0; i < 4; ++i)
        {
            FVM_Result r = fvm_object_create(c, &objects[i]);
            if (r != FVM_OK)
                return r;
            objects[i]->resource_kind = i == 3 ? 7 : GC_NAMESPACE + i;
        }
        method(objects[0], "collect", gc_collect, 0, true);
        method(objects[0], "step", gc_step, 1, true);
        method(objects[0], "mode", gc_mode, 2, true);
        method(objects[0], "state", gc_state, 3, true);
        method(objects[0], "count", gc_count, 4, true);
        method(objects[1], "run", run, 0);
        method(objects[1], "run_suspended", run_suspended, 1);
        method(objects[1], "join", join, 2);
        method(objects[1], "stopRequested", stop_requested, 3);
        method(objects[1], "sleep", sleep, 4);
        method(objects[1], "sleepms", sleepms, 5);
        method(objects[1], "sleepus", sleepus, 6);
        method(objects[2], "create", create_channel, 0);
        method(objects[3], "attach", meta_attach, 0);
        method(objects[3], "get", meta_get, 0);
        /* Build globals transactionally so allocation failures cannot leave partial installation.
         */
        fvm_detail::map<fvm_detail::string, FVM_Value> globals = c->global->properties;
        for (int i = 0; i < 4; ++i)
        {
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = objects[i];
            globals[names[i]] = v;
        }
        c->global->properties.swap(globals);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    return FVM_OK;
}
