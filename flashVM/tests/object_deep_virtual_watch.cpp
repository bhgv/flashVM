/* A deeper inherited virtual consumes a deferred watcher exception across data owners. */
#define main deep_watch_baseline_main
#include "object_watch_throw.cpp"
#undef main
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"slot", "5", "+5", "n\0m"};
    const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array)
    for (unsigned key = 0; key < 4; ++key)
    for (setter_throws = 0; setter_throws < 2; ++setter_throws)
    for (unsigned getter_only = 0; getter_only < 2; ++getter_only) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *middle = 0, *far = 0, *watch = 0, *get = 0, *set = 0;
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, 9);
        assert(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK);
        assert(fvm_object_set_bytes(c, far, keys[key], sizes[key], &value) == FVM_OK);
        const unsigned char end[] = {0};
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK && fvm_function_set_body(c, watch, throwing_watch, 0, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, throw_get, 0, 0) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK && fvm_function_set_body(c, set, throw_set, 0, 0) == FVM_OK);
        fvm_value_string_bytes(args, keys[key], sizes[key]); args[1] = thrown_object(get);
        if (getter_only) { fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL; } else args[2] = thrown_object(set);
        assert(fvm_object_call(c, far, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, middle, far) == FVM_OK && fvm_object_set_prototype(c, receiver, middle) == FVM_OK);
        args[1] = thrown_object(watch); assert(fvm_object_call(c, receiver, "watch", 2, args, &out) == FVM_OK && out.boolean);
        setter_calls = 0; fvm_value_number(&value, 81);
        const FVM_Result result = fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value);
        assert(result == (!getter_only && setter_throws ? FVM_EXCEPTION : FVM_OK));
        assert(setter_calls == (getter_only ? 0U : 1U));
        if (result == FVM_EXCEPTION) { assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 88); assert(fvm_exception_clear(c) == FVM_OK); }
        else assert(!c->exception_pending);
        if (array) assert(fvm_object_get(c, receiver, "length", &out) == FVM_OK && out.number == 0);
        int own = 1; assert(fvm_object_has_own_bytes(c, receiver, keys[key], sizes[key], &own) == FVM_OK && !own);
        assert(fvm_object_get_bytes(c, middle, keys[key], sizes[key], &out) == FVM_OK && out.number == 9);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty());
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
