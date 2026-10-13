/* registerClass validates constructor first and retains loaded empty/countеd NUL names through GC/transfer. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <cassert>
static unsigned mode, calls;
static FVM_Result name(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; return fvm_value_string_bytes(out, mode ? "n\0m" : "", mode ? 3 : 0); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *key = 0, *ctor = 0, *found = 0; FVM_Value klass, args[2], out, copy;
        assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK); assert(fvm_gc_pin(c, key) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, key, "toString", name, 0, 0) == FVM_OK);
        fvm_value_undefined(args); args[0].type = FVM_VALUE_OBJECT; args[0].object = key;
        fvm_value_number(args + 1, 81); calls = 0;
        assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && !out.boolean && calls == 0);
        args[1] = args[0]; assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && !out.boolean && calls == 0);
        const unsigned char code[] = {0x96,5,0,7,11,0,0,0,0x3e,0};
        assert(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &ctor) == FVM_OK); args[1].object = ctor;
        mode = 0; calls = 0; assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean == (version != 0) && calls == 1);
        assert(fvm_registered_class_bytes(c, "", 0, &found) == (version ? FVM_OK : FVM_NOT_FOUND));
        if (version) assert(found == ctor);
        fvm_value_undefined(args);
        assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean);
        assert(fvm_registered_class(c, version && version < 7 ? "" : "undefined", &found) == FVM_OK && found == ctor);
        args[0].type = FVM_VALUE_OBJECT; args[0].object = key;
        mode = 1; calls = 0; assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean && calls == 1);
        assert(fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_registered_class_bytes(c, "n\0m", 3, &found) == FVM_OK && found == ctor);
        assert(fvm_function_invoke(c, found, 0, 0, 0, &out) == FVM_OK && out.number == 11);
        FVM_Channel *ch = 0; assert(fvm_context_create(rt, &other) == FVM_OK); assert(fvm_channel_create(&ch) == FVM_OK);
        assert(fvm_channel_send_value(c, ch, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK);
        assert(fvm_set_global(other, "Object", &copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_registered_class_bytes(other, "n\0m", 3, &found) == FVM_OK);
        assert(fvm_function_invoke(other, found, 0, 0, 0, &out) == FVM_OK && out.number == 11);
        if (version) { assert(fvm_registered_class_bytes(other, "", 0, &found) == FVM_OK); assert(fvm_function_invoke(other, found, 0, 0, 0, &out) == FVM_OK && out.number == 11); }
        mode = 0; fvm_value_undefined(args + 1); assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean == (version != 0));
        assert(fvm_registered_class_bytes(c, "", 0, &found) == FVM_NOT_FOUND);
        fvm_channel_destroy(ch); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
