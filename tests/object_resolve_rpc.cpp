/* Resolver interfaces execute the original C callback/context/userdata through one broker port. */
#include "flashvm/capability.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/serialization.h"
#include <cassert>
#include <cstring>
#ifdef OS_PORTS
struct ResolveSource { FVM_Context *context; unsigned calls; };
static FVM_Value rpc_resolve_value(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static FVM_Result original_resolver(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    ResolveSource *source = (ResolveSource *)userdata; assert(c == source->context && n == 1 && args[0].type == FVM_VALUE_STRING); ++source->calls;
    FVM_Value self, marker; assert(fvm_object_get(c, receiver, "self", &self) == FVM_OK && self.object == receiver);
    assert(fvm_object_get(c, receiver, "marker", &marker) == FVM_OK && marker.number == 37);
    size_t count = fvm_value_string_size(args);
    if (count == 5 && !std::memcmp(args[0].string, "throw", 5)) { fvm_value_string_bytes(out, "N\0E", 3); return fvm_exception_raise(c, out); }
    if (count == 11 && !std::memcmp(args[0].string, "unsupported", 11)) return FVM_UNSUPPORTED;
    if (count == 8 && !std::memcmp(args[0].string, "notfound", 8)) return FVM_NOT_FOUND;
    *out = args[0]; return FVM_OK;
}
#endif
int main()
{
#ifdef OS_PORTS
    for (unsigned version = 6; version <= 8; version += 2) {
        FVM_Config cfg; FVM_Runtime *source_runtime = 0, *target_runtime = 0; FVM_Context *source = 0, *target = 0;
        assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &source_runtime) == FVM_OK && fvm_runtime_create(&cfg, &target_runtime) == FVM_OK);
        assert(fvm_context_create(source_runtime, &source) == FVM_OK && fvm_context_create(target_runtime, &target) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(source, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(target, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(source) == FVM_OK && fvm_standard_install(target) == FVM_OK);
        FVM_ResourceBroker *broker = 0; assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
        unsigned short port = fvm_broker_port(broker); assert(port && fvm_context_set_broker(source, broker) == FVM_OK && fvm_context_function_transfer(source, 1) == FVM_OK);
        ResolveSource state; state.context = source; state.calls = 0; FVM_Object *original = 0, *proxy = 0, *receiver = 0; char *uri = 0;
        assert(fvm_function_create_native(source, original_resolver, &state, 0, &original) == FVM_OK && fvm_broker_export(source, original, &uri) == FVM_OK);
        assert(fvm_context_function_serve(source, 1) == FVM_OK);
        /* Source heap/userdata are exclusively owned by the authority until serve(0). */
        assert(fvm_broker_import(target, uri, &proxy) == FVM_OK && fvm_construct(target, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(target, receiver) == FVM_OK);
        FVM_Value value = rpc_resolve_value(proxy), out; assert(fvm_object_set(target, receiver, "__resolve", &value) == FVM_OK);
        value = rpc_resolve_value(receiver); assert(fvm_object_set(target, receiver, "self", &value) == FVM_OK);
        fvm_value_number(&value, 37); assert(fvm_object_set(target, receiver, "marker", &value) == FVM_OK);
        fvm_value_undefined(&value); assert(fvm_object_set(target, receiver, "present", &value) == FVM_OK && fvm_gc_string_mode(target, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(target, 0) == FVM_OK);
        assert(fvm_object_get(target, receiver, "present", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        const char *names[] = {"remote\0name", "\xc3\x84\0X", "throw", "unsupported", "notfound", "recover"}; const size_t sizes[] = {11,4,5,11,8,7};
        for (unsigned i = 0; i < 6; ++i) {
            FVM_Result status = fvm_object_get_bytes(target, receiver, names[i], sizes[i], &out);
            assert(status == (i == 2 ? FVM_EXCEPTION : i == 3 ? FVM_UNSUPPORTED : i == 4 ? FVM_NOT_FOUND : FVM_OK));
            if (i == 2) { assert(fvm_exception_get(target, &out) == FVM_OK && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "N\0E", 3) && fvm_exception_clear(target) == FVM_OK); }
            else if (i != 3 && i != 4) assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == sizes[i] && !std::memcmp(out.string, names[i], sizes[i]));
            else assert(out.type == FVM_VALUE_UNDEFINED);
            assert(fvm_gc_collect(target, 0) == FVM_OK && fvm_broker_port(broker) == port);
        }
        FVM_BrokerStats stats; assert(fvm_broker_stats(broker, &stats) == FVM_OK && stats.exported_resources == 1);
        assert(fvm_context_function_serve(source, 0) == FVM_OK && state.calls == 6);
        assert(fvm_context_function_revoke(source, original) == FVM_OK);
        assert(fvm_object_get(target, receiver, "stale", &out) == FVM_NOT_FOUND);
        fvm_broker_stop(broker); assert(state.calls == 6);
        assert(fvm_context_set_broker(source, 0) == FVM_OK); fvm_serialized_free(uri);
        fvm_context_destroy(target); fvm_context_destroy(source); fvm_runtime_destroy(target_runtime); fvm_runtime_destroy(source_runtime); fvm_broker_release(broker);
    }
#endif
}
