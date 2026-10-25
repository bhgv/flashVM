/* registerClass follows the activation base movie, preserving ambient lookup policy. */
#define main provenance_sprite_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#define FVM_CLASS_CASE_FIXTURE_ONLY
#include "object_register_class_case.cpp"
#include "../src/standard/movieclip_state.hpp"
#include "../src/standard/movieclip_provenance_constants.h"


static FVM_Object *provenance_root(FVM_Context *c, unsigned version)
{
    Bytes tags, empty;
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 1);
    swf[3] = (unsigned char)version;
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}

static FVM_Object *provenance_base, *provenance_selected;


static FVM_Result provenance_name(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(c->avm1_base_clip == provenance_base && c->avm1_target_clip == provenance_selected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_value_string(out, "Hero");
}


static void provenance_register(FVM_Context *c, FVM_Object *base, FVM_Object *selected, FVM_Object *fn)
{
    provenance_base = base;
    provenance_selected = selected;
    FVM_Object *key = 0;
    if (!(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK &&
          fvm_object_define_method_rooted(c, key, "toString", provenance_name, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v = case_object(key);
    if (!(fvm_set_global(c, "registrationName", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = case_object(selected);
    if (!(fvm_object_set(c, base, "other", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = case_object(fn);
    if (!(fvm_set_global(c, "registrationCtor", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = case_object(selected);
    if (!(fvm_set_global(c, "selectedClip", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes code;
    text(code, "other");
    code.push_back(0x20);
    variable(code, "registrationCtor");
    variable(code, "registrationName");
    integer(code, 2);
    variable(code, "Object");
    text(code, "registerClass");
    code.push_back(0x52);
    code.push_back(0x17);
    code.push_back(0);
    if (!(fvm_avm1_execute_receiver(c, code.begin(), code.size(), base) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_CLASS_PROVENANCE_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned ambient = 6; ambient <= 8; ambient += 2)
    {
        for (unsigned source = 4; source <= 8; ++source)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_loaded(c, ambient);
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *base = provenance_root(c, source), *selected = provenance_root(c, source < 7 ? 8 : 6),
                       *fn = case_ctor(c, 11), *found = 0;
            unsigned version = 0;
            if (!(fvm_movieclip_movie_version(c, base, &version) == FVM_OK && version == source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            provenance_register(c, base, selected, fn);
            bool same = (ambient < 7) == (source < 7);
            if (!(fvm_registered_class(c, "Hero", &found) == (same ? FVM_OK : FVM_NOT_FOUND)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (same)
            {
                if (!(found == fn))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            case_loaded(c, source);
            if (!(fvm_registered_class(c, "Hero", &found) == FVM_OK && found == fn))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_registered_class(c, "hERO", &found) == (source < 7 ? FVM_OK : FVM_NOT_FOUND)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->avm1_target_active && !c->avm1_base_clip && !c->gc_frames &&
                  !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Private metadata ignores public names and validates before narrowing numeric input. */
            FVM_Value value;
            fvm_value_number(&value, 99);
            if (!(fvm_object_set(c, base, "swf_version", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_movie_version(c, base, &version) == FVM_OK && version == source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *library = base->resource_result.object->properties["library"].object;
            FVM_Value saved = library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME];
            library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME] = value;
            version = 77;
            if (!(fvm_movieclip_movie_version(c, base, &version) == FVM_INVALID_ARGUMENT && version == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME] = saved;
            library->properties.erase(FVM_MOVIECLIP_LIBRARY_VERSION_NAME);
            unsigned ambient_version = 0;
            if (!(fvm_movieclip_movie_version(c, base, &ambient_version) == FVM_OK && ambient_version == source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME] = saved;
            FVM_Context *other = 0;
            FVM_Channel *ch = 0;
            if (!(fvm_context_create(rt, &other) == FVM_OK && fvm_channel_create(&ch) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_loaded(other, ambient);
            if (!(fvm_standard_install(other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = case_object(base);
            FVM_Value copy;
            if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK &&
                  fvm_gc_pin(other, copy.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_movie_version(other, copy.object, &version) == FVM_OK && version == source))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *otherfn = case_ctor(other, 22);
            provenance_register(other, copy.object, copy.object, otherfn);
            case_loaded(other, source);
            if (!(fvm_registered_class(other, "Hero", &found) == FVM_OK && found == otherfn))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!other->gc_frames && !other->gc_native_depth && !other->avm1_target_active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(ch);
            fvm_context_destroy(other);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
