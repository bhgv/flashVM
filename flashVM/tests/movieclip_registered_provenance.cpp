/* Attach, duplicate and authored placement use the symbol movie's constructor environment. */
#define FVM_CLASS_PROVENANCE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "object_register_class_provenance.cpp"
#include "flashvm/execution.h"


static FVM_Object *symbol_root(FVM_Context *c, unsigned version)
{
    Bytes controls, tags, empty;
    tag(controls, 1, empty);
    tag(controls, 0, empty);
    tag(tags, 39, sprite(7, 1, controls));
    export_asset(tags, 7, "Hero");
    Bytes placement;
    placement.push_back(34);
    u16(placement, 1);
    u16(placement, 7);
    name(placement, "actor");
    tag(tags, 26, placement);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 1);
    swf[3] = (unsigned char)version;
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "symbols", &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return root;
}


static void symbol_register(FVM_Context *c, unsigned version)
{
    case_loaded(c, version);
    FVM_Value klass, ctor = registered_constructor(c), value;
    if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value proto = get(c, ctor.object, "prototype");
    fvm_value_number(&value, version);
    if (!(fvm_object_set(c, proto.object, "constructorEnvironment", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    case_register(c, klass.object, version < 7 ? "hERO" : "Hero", 4, ctor.object);
}
#ifndef FVM_SYMBOL_PROVENANCE_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned source = 5; source <= 8; ++source)
    {
        for (unsigned ambient = 6; ambient <= 8; ambient += 2)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_loaded(c, 8);
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            symbol_register(c, 6);
            symbol_register(c, 8);
            case_loaded(c, ambient);
            FVM_Object *root = symbol_root(c, source), *clip = attach(c, root, "Hero", "dynamic", 3);
            unsigned expected = source < 7 ? 6 : 8;
            if (!(clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, clip, "constructorEnvironment", expected);
            if (!(c->swf_info.version == ambient))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value instance, depth, initial, out;
            fvm_value_string(&instance, "copy");
            fvm_value_number(&depth, 4);
            fvm_value_undefined(&initial);
            if (!(fvm_exec_duplicate_clip(c, clip, &instance, &depth, &initial, &out) == FVM_OK &&
                  out.type == FVM_VALUE_OBJECT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, out.object, "constructorEnvironment", expected);
            if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value actor = get(c, root, "actor");
            if (!(actor.type == FVM_VALUE_OBJECT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, actor.object, "constructorEnvironment", expected);
            if (!(fvm_gc_collect(c, 0) == FVM_OK && c->swf_info.version == ambient))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* A transported clip retains both maps and source version without installing destination Object. */
            FVM_Channel *ch = 0;
            FVM_Context *other = 0;
            if (!(fvm_channel_create(&ch) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_loaded(other, source < 7 ? 8 : 6);
            FVM_Value value = case_object(root), copy;
            if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK &&
                  fvm_gc_pin(other, copy.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *attached = attach(other, copy.object, "Hero", "transported", 8);
            if (!(attached))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(other, attached, "constructorEnvironment", expected);
            /* Old libraries without source metadata keep ambient constructor selection. */
            FVM_Object *library = root->resource_result.object->properties["library"].object;
            FVM_Value saved = library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME];
            library->properties.erase(FVM_MOVIECLIP_LIBRARY_VERSION_NAME);
            FVM_Object *old = attach(c, root, "Hero", "oldLibrary", 9);
            if (!(old))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, old, "constructorEnvironment", ambient);
            library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME] = saved;
            if (!(!c->gc_frames && !c->gc_native_depth && !other->gc_frames && !other->gc_native_depth))
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
