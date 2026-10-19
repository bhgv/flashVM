/* Broadcaster graphs across memory/file/TCP channels and independent process images. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/broadcaster.h"
#include "flashvm/extensions.h"
#include "flashvm/transfer.h"
#include "flashvm/capability.h"
#include "flashvm/serialization.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static Program handler_program()
{
    Program p;
    p.variable("this"); p.text("received"); p.variable("payload"); p.op(0x4f);
    p.variable("this"); p.text("count"); p.variable("this"); p.text("count"); p.op(0x4e);
    p.number(1); p.op(0x47); p.op(0x4f);
    p.number(0); p.variable("gc"); p.text("collect"); p.op(0x52); p.op(0x17);
    p.op(0); return p;
}
static FVM_Context *context(FVM_Runtime *r, unsigned version)
{
    FVM_Context *c = 0; assert(fvm_context_create(r, &c) == FVM_OK);
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
    swf[3] = static_cast<unsigned char>(version);
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK && fvm_extensions_install(c) == FVM_OK);
    assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
    return c;
}
static FVM_Value graph(FVM_Context *c)
{
    FVM_Object *source = 0, *listener = 0, *fn = 0;
    assert(fvm_object_create(c, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
    assert(fvm_object_create(c, &listener) == FVM_OK && fvm_gc_pin(c, listener) == FVM_OK);
    assert(fvm_broadcaster_initialize(c, source) == FVM_OK);
    FVM_Value root = class_value(source), v = class_value(listener), out;
    assert(fvm_object_set(c, source, "listener", &v) == FVM_OK);
    assert(fvm_object_set(c, listener, "source", &root) == FVM_OK);
    assert(fvm_object_call(c, source, "addListener", 1, &v, &out) == FVM_OK);
    fvm_value_number(&v, 0); assert(fvm_object_set(c, listener, "count", &v) == FVM_OK);
    Program body = handler_program(); const char *parameters[] = {"payload"};
    assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &fn) == FVM_OK);
    v = class_value(fn); assert(fvm_object_set(c, listener, "onValue", &v) == FVM_OK);
    assert(fvm_object_get(c, source, "broadcastMessage", &v) == FVM_OK);
    assert(fvm_object_set(c, source, "savedBroadcast", &v) == FVM_OK);
    return root;
}
static void verify(FVM_Context *c, FVM_Value root, unsigned mode)
{
    assert(fvm_gc_pin(c, root.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value v, other, listener, handler, args[2], out;
    assert(fvm_object_get(c, root.object, "listener", &listener) == FVM_OK);
    assert(fvm_object_get(c, listener.object, "source", &v) == FVM_OK && v.object == root.object);
    assert(fvm_object_get(c, listener.object, "count", &v) == FVM_OK && v.number == 0);
    assert(fvm_object_get(c, root.object, "broadcastMessage", &v) == FVM_OK);
    assert(fvm_object_get(c, root.object, "savedBroadcast", &other) == FVM_OK && v.object == other.object);
    assert(fvm_object_get(c, listener.object, "onValue", &handler) == FVM_OK);
    FVM_JitCode *code = 0;
    if (mode == 1) {
        Program p = handler_program();
        assert(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK);
        assert(fvm_function_set_body(c, handler.object, fvm_jit_body, code, 0) == FVM_OK);
    }
    assert(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    unsigned long entries = c->execution_mode.stats.native_entries;
    assert(fvm_value_string(args, "onValue") == FVM_OK); fvm_value_number(args + 1, 91);
    assert(fvm_object_call(c, root.object, "savedBroadcast", 2, args, &out) == FVM_OK);
    assert(fvm_object_call(c, root.object, "broadcastMessage", 2, args, &out) == FVM_OK);
    assert(fvm_object_get(c, listener.object, "received", &v) == FVM_OK && v.number == 91);
    assert(fvm_object_get(c, listener.object, "count", &v) == FVM_OK && v.number == 2);
    assert(mode ? c->execution_mode.stats.native_entries > entries : c->execution_mode.stats.native_entries == entries);
    assert(fvm_object_call(c, root.object, "removeListener", 1, &listener, &out) == FVM_OK && out.boolean);
    assert(fvm_object_call(c, root.object, "broadcastMessage", 2, args, &out) == FVM_OK);
    assert(fvm_object_get(c, listener.object, "count", &v) == FVM_OK && v.number == 2);
    assert(fvm_broadcaster_initialize(c, root.object) == FVM_OK);
    assert(fvm_object_get(c, root.object, "_listeners", &v) == FVM_OK);
    assert(fvm_object_get(c, v.object, "length", &other) == FVM_OK && other.number == 0);
    if (code) {
        assert(!code->active && fvm_function_set_body(c, handler.object, 0, 0, 0) == FVM_OK);
        assert(fvm_jit_destroy(code) == FVM_OK);
    }
    assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth);
}
int main(int argc, char **argv)
{
    FVM_Config cfg; FVM_Runtime *r = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK);
    if (argc == 2 && !std::strcmp(argv[1], "--modes")) {
        std::printf("%u\n", fvm_jit_available() ? 3U : 1U); fvm_runtime_destroy(r); return 0;
    }
    if (argc == 5) {
        unsigned version = static_cast<unsigned>(std::atoi(argv[3]));
        unsigned mode = static_cast<unsigned>(std::atoi(argv[4]));
        FVM_Context *c = context(r, version); FVM_Channel *file = 0; FVM_Value value;
        assert(mode == 0 || fvm_jit_available());
        assert(fvm_channel_open_file(argv[2], argv[1][0] == 'w' ? "w" : "r", &file) == FVM_OK);
        if (argv[1][0] == 'w') {
            value = graph(c); assert(fvm_channel_send_value(c, file, &value, 0) == FVM_OK);
        } else {
            assert(fvm_channel_receive_value(c, file, &value, 0) == FVM_OK); verify(c, value, mode);
        }
        fvm_channel_destroy(file); fvm_context_destroy(c); fvm_runtime_destroy(r); return 0;
    }
    for (unsigned version = 6; version <= 8; ++version)
    for (unsigned mode = 0; mode < (fvm_jit_available() ? 3U : 1U); ++mode)
    for (unsigned transport = 0; transport < 3; ++transport) {
#ifndef OS_PORTS
        if (transport == 2) continue;
#endif
        FVM_Context *src = context(r, version), *dst = context(r, version), *owner = 0;
        FVM_Channel *queue = 0, *input = 0; FVM_ResourceBroker *broker = 0; char *uri = 0;
        const char *path = "broadcaster-transport-test.bin";
        if (transport == 1) {
            assert(fvm_channel_open_file(path, "w", &queue) == FVM_OK);
        } else {
            assert(fvm_channel_create(&queue) == FVM_OK);
            assert(fvm_channel_set_capacity(queue, 1) == FVM_OK);
        }
#ifdef OS_PORTS
        if (transport == 2) {
            assert(fvm_context_create(r, &owner) == FVM_OK);
            assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
            assert(fvm_context_set_broker(owner, broker) == FVM_OK);
            FVM_Object *wrapped = 0;
            assert(fvm_extensions_wrap_channel(owner, queue, &wrapped) == FVM_OK);
            assert(fvm_broker_export(owner, wrapped, &uri) == FVM_OK);
            assert(fvm_channel_open_network(dst, uri, &input) == FVM_OK);
        }
#endif
        FVM_Value value = graph(src), out;
        assert(fvm_channel_send_value(src, queue, &value, 0) == FVM_OK);
        if (transport != 1) assert(fvm_channel_send_value(src, queue, &value, 0) == FVM_NOT_FOUND);
        /* The queued message must survive a reset and destruction of the source heap. */
        assert(fvm_broadcaster_initialize(src, value.object) == FVM_OK);
        fvm_context_destroy(src);
        if (transport == 1) {
            fvm_channel_destroy(queue); queue = 0;
            assert(fvm_channel_open_file(path, "r", &input) == FVM_OK);
        } else if (!transport) input = queue;
        assert(fvm_channel_receive_value(dst, input, &out, 0) == FVM_OK);
        verify(dst, out, mode);
        if (input != queue) fvm_channel_destroy(input);
        if (broker) { fvm_broker_stop(broker); fvm_broker_release(broker); }
        if (owner) fvm_context_destroy(owner);
        if (uri) fvm_serialized_free(uri);
        if (queue) fvm_channel_destroy(queue);
        fvm_context_destroy(dst);
        if (transport == 1) assert(std::remove(path) == 0);
    }
    fvm_runtime_destroy(r);
}
