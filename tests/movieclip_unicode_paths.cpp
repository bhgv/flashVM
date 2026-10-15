/* Legacy display paths fold Flash UCS2 names without losing counted storage. */
#define main unicode_path_base_main
#include "movieclip_paths.cpp"
#undef main
#include "../src/avm1/scope_keywords.hpp"
static void unicode_resolve(FVM_Context *c, FVM_Object *root, const char *name, size_t size, FVM_Object *expected)
{
    FVM_Value value; FVM_Object *out = root;
    assert(fvm_value_string_bytes(&value, name, size) == FVM_OK);
    assert(fvm_movieclip_resolve_path(c, root, value, &out) == FVM_OK && out == expected);
}
static void unicode_named(FVM_Context *c, FVM_Object *root, const char *name, FVM_Object *expected)
{ unicode_resolve(c, root, name, std::strlen(name), expected); }
static void unicode_rename(FVM_Context *c, FVM_Object *clip, const char *name, size_t size)
{
    FVM_Value value; assert(fvm_value_string_bytes(&value, name, size) == FVM_OK);
    assert(fvm_object_set(c, clip, "_name", &value) == FVM_OK);
}
#ifndef FVM_UNICODE_PATH_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *root = 0; assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
        bool folded = version && version < 7;
        FVM_Object *a = child(c, root, "seed", 1);
        unicode_rename(c, a, "\xc3\x84", 2);
        unicode_named(c, root, "\xc3\x84", a); unicode_named(c, root, "\xc3\xa4", folded ? a : 0);
        unicode_rename(c, a, "\xc4\xb0x", 3);
        unicode_named(c, root, "iX", folded ? a : 0); unicode_named(c, root, "\xc4\xb0x", a);
        unicode_rename(c, a, "\xd0\x94", 2);
        unicode_named(c, root, "\xd0\xb4", folded ? a : 0);
        unicode_rename(c, a, "\xce\xa3", 2);
        unicode_named(c, root, "\xcf\x83", folded ? a : 0); unicode_named(c, root, "\xcf\x82", 0);
        unicode_rename(c, a, "\xc3\x84\0Z", 4);
        unicode_resolve(c, root, "\xc3\xa4\0z", 4, folded ? a : 0);
        unicode_resolve(c, root, "\xc3\x84\0Z", 4, a); unicode_named(c, root, "\xc3\xa4", 0);
        unicode_rename(c, a, "\xed\xa0\x80", 3); unicode_named(c, root, "\xed\xa0\x80", a);
        unicode_rename(c, a, "\xf0\x90\x90\x80", 4);
        unicode_named(c, root, "\xed\xa0\x81\xed\xb0\x80", a);
        unicode_named(c, root, "\xf0\x90\x90\xa8", 0);
        FVM_Object *alias = 0; assert(fvm_construct(c, "Object", 0, 0, &alias) == FVM_OK);
        FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = alias;
        assert(fvm_object_set(c, root, "\xc3\x84lias", &value) == FVM_OK);
        value.object = a; assert(fvm_object_set(c, alias, "\xd0\x94", &value) == FVM_OK);
        unicode_named(c, root, "\xc3\xa4LIAS/\xd0\xb4", folded ? a : 0);
        unicode_named(c, root, "\xc3\x84lias/\xd0\x94", a);
        FVM_Object *priority = child(c, root, "\xc3\x84lias", 2);
        value.object = alias; assert(fvm_object_set(c, root, "\xc3\x84lias", &value) == FVM_OK);
        unicode_named(c, root, "\xc3\xa4LIAS", folded ? priority : 0);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        unicode_named(c, root, "\xc3\x84lias", priority);
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
