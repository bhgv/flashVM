/* Versioned flag mutation and deletion use stored spelling without evaluating values. */
#define main property_case_query_fixture_main
#include "object_property_case_queries.cpp"
#undef main
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK); touched = 0;
        const bool folded = version && version <= 6;
        const char *stored[] = {"DaTa", "Getter", "Method", "N\0M", "\xc3\x84rea"};
        const char *query[] = {"data", "getter", "method", "n\0m", "\xc3\xa4rea"};
        const size_t sizes[] = {4, 6, 6, 3, 5};
        for (unsigned kind = 0; kind < 5; ++kind) {
            FVM_Object *object = 0; FVM_Value value, out; fvm_value_number(&value, 17);
            assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
            if (kind == 1) assert(fvm_object_define_accessor_rooted(c, object, stored[kind], forbidden, 0, 0, 0) == FVM_OK);
            else if (kind == 2) assert(fvm_object_define_method_rooted(c, object, stored[kind], forbidden, 0, 0) == FVM_OK);
            else assert(fvm_object_set_bytes(c, object, stored[kind], sizes[kind], &value) == FVM_OK);
            assert(fvm_object_set_property_flags_bytes(c, object, stored[kind], sizes[kind], FVM_PROPERTY_DONT_ENUM, 0) == FVM_OK);
            assert(fvm_object_set_property_flags_bytes(c, object, query[kind], sizes[kind], FVM_PROPERTY_DONT_DELETE, 0) == FVM_OK);
            unsigned flags = 0; int deleted = 1, own = 0;
            assert(fvm_object_property_flags_bytes(c, object, stored[kind], sizes[kind], &flags) == FVM_OK && flags == (folded ? 3U : 1U));
            assert(fvm_object_delete_bytes(c, object, query[kind], sizes[kind], &deleted) == FVM_OK && !deleted);
            assert(fvm_object_set_property_flags_bytes(c, object, query[kind], sizes[kind], 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK);
            assert(fvm_object_delete_bytes(c, object, query[kind], sizes[kind], &deleted) == FVM_OK && deleted);
            assert(fvm_object_has_own_bytes(c, object, stored[kind], sizes[kind], &own) == FVM_OK && !!own == !folded);
            assert(fvm_object_property_flags_bytes(c, object, stored[kind], sizes[kind], &flags) == FVM_OK && flags == (folded ? 0U : 1U));
            assert(fvm_object_get_bytes(c, object, query[kind], sizes[kind], &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        }
        FVM_Object *object = 0, *parent = 0; FVM_Value value; fvm_value_number(&value, 17);
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, object, parent) == FVM_OK);
        assert(fvm_object_set(c, parent, "Inherited", &value) == FVM_OK);
        int removed = 0, own = 0;
        assert(fvm_object_delete(c, object, "inherited", &removed) == FVM_OK && removed);
        assert(fvm_object_has_own(c, parent, "Inherited", &own) == FVM_OK && own);
        assert(!touched && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
