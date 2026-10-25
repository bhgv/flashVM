/* Target changes affect clip variables/controls while this, caller scopes and GC roots stay correct. */
#define main paths_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_paths.cpp"

#undef main
#include "flashvm/extensions.h"

static unsigned overridden;


static FVM_Result override_stop(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++overridden;
    return FVM_ERROR;
}


static void target(Bytes &b, const char *name)
{
    b.push_back(0x8b);
    unsigned n = (unsigned)std::strlen(name) + 1;
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
    do
    {
        b.push_back((unsigned char)*name);
    } while (*name++);
}


static void variable(Bytes &b, const char *name, unsigned n)
{
    text(b, name);
    integer(b, n);
    b.push_back(0x1d);
}


static void read(Bytes &b, const char *field, const char *name, const char *legacy_owner = 0)
{
    if (legacy_owner)
    {
        fvm_detail::vector<char> path(std::strlen(legacy_owner) + std::strlen(field) + 2);
        std::strcpy(path.begin(), legacy_owner);
        std::strcat(path.begin(), ":");
        std::strcat(path.begin(), field);
        text(b, path.begin());
        global(b, name);
        b.push_back(0x1d);
    }
    else
    {
        global(b, "this");
        text(b, field);
        global(b, name);
        b.push_back(0x4f);
    }
}


static FVM_Result execute(FVM_Context *c, FVM_Object *clip, Bytes b)
{
    b.push_back(0);
    return fvm_avm1_execute_receiver(c, b.begin(), b.size(), clip);
}


static void number(FVM_Context *c, FVM_Object *clip, const char *name, double expected)
{
    FVM_Value out;
    if (!(fvm_object_get(c, clip, name, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void private_number(FVM_Object *clip, const char *name, double expected)
{
    FVM_Value value = clip->resource_result.object->properties[name];
    if (!(value.type == FVM_VALUE_NUMBER && value.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void goto_number(Bytes &b, unsigned frame, unsigned flags)
{
    integer(b, frame);
    b.push_back(0x9f);
    b.push_back(1);
    b.push_back(0);
    b.push_back((unsigned char)flags);
}


struct Detached
{
    FVM_Object *root, *child;
};


static FVM_Result detach(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Detached *data = (Detached *)raw;
    data->root->properties.erase("child");
    FVM_Object *children = data->root->resource_result.object->properties["children"].object;
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = children->indexed.begin();
         i != children->indexed.end();
         ++i)
    {
        if (i->second.type == FVM_VALUE_OBJECT && i->second.object == data->child)
        {
            unsigned long slot = i->first;
            children->indexed.erase(slot);
            break;
        }
    }
    fvm_value_undefined(&data->child->resource_result.object->properties["parent"]);
    data->child->resource_result.object->properties["parent"].type = FVM_VALUE_NULL;
    fvm_value_undefined(out);
    return FVM_OK;
}
#ifndef FVM_AVM1_TARGET_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = 0;
        if (!(fvm_movieclip_create(c, "root", 3, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
        if (!(fvm_set_global(c, "root", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *selected = child(c, root, "child", 1), *other = child(c, root, "other", 2);
        fvm_value_number(&value, 3);
        selected->resource_result.object->properties["total"] = value;
        unsigned char empty[] = {0};
        for (unsigned frame = 1; frame <= 3; ++frame)
        {
            if (!(fvm_movieclip_define_frame(c, selected, frame, frame == 3 ? "third" : "", empty, sizeof(empty)) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&value, 5);
        if (!(fvm_object_set(c, root, "identifier", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 7);
        if (!(fvm_object_set(c, selected, "identifier", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        target(b, "child");
        variable(b, "mark", 42);
        read(b, "read", "identifier", version == 4 ? "root" : 0);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "mark", 42);
        number(c, root, "read", 7);
        if (!(!c->avm1_target_active && !c->avm1_target_clip && !c->avm1_base_clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        target(b, "missing");
        read(b, "read", "identifier", version == 4 ? "root/child" : 0);
        variable(b, "fallback", 9);
        if (!(execute(c, selected, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "read", 5);
        number(c, root, "fallback", 9);
        b.clear();
        b.push_back(0x96);
        b.push_back(1);
        b.push_back(0);
        b.push_back(3);
        b.push_back(0x20);
        read(b, "read", "identifier", version == 4 ? "root/child" : 0);
        if (!(execute(c, selected, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "read", version <= 6 ? 7 : 5);
        b.clear();
        text(b, "child");
        b.push_back(0x20);
        variable(b, "dynamic", 99);
        target(b, "");
        variable(b, "reset", 10);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "dynamic", 99);
        number(c, root, "reset", 10);
        if (!(fvm_object_define_method_rooted(c, selected, "stop", override_stop, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        target(b, "child");
        b.push_back(0x07);
        if (!(execute(c, root, b) == FVM_OK && overridden == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        private_number(selected, "playing", 0);
        private_number(root, "playing", 1);
        b.clear();
        target(b, "child");
        b.push_back(0x81);
        b.push_back(2);
        b.push_back(0);
        b.push_back(1);
        b.push_back(0);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 2);
        private_number(selected, "playing", 0);
        number(c, root, "_currentframe", 1);
        b.clear();
        target(b, "child");
        b.push_back(0x04);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 3);
        b.clear();
        target(b, "child");
        b.push_back(0x05);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 2);
        b.clear();
        target(b, "child");
        text(b, "third");
        b.push_back(0x9f);
        b.push_back(1);
        b.push_back(0);
        b.push_back(1);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 3);
        private_number(selected, "playing", 1);
        b.clear();
        target(b, "child");
        text(b, "2");
        b.push_back(0x9f);
        b.push_back(1);
        b.push_back(0);
        b.push_back(0);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 2);
        private_number(selected, "playing", 0);
        fvm_value_string_bytes(&value, "third\0suffix", 12);
        if (!(fvm_set_global(c, "frameLabel", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        target(b, "child");
        global(b, "frameLabel");
        b.push_back(0x9f);
        b.push_back(1);
        b.push_back(0);
        b.push_back(0);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_currentframe", 2);
        b.clear();
        target(b, "missing");
        b.push_back(0x07);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        private_number(root, "playing", 1);
        b.clear();
        target(b, "missing");
        goto_number(b, 2, 0);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, root, "_currentframe", 2);
        /* A fresh function's own target changes cannot replace its caller's selected clip. */
        Bytes function;
        target(function, "other");
        variable(function, "inside", 8);
        function.push_back(0);
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, function.begin(), function.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_set_global(c, "function", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        target(b, "child");
        integer(b, 0);
        text(b, "function");
        b.push_back(0x3d);
        b.push_back(0x17);
        variable(b, "after", 11);
        if (!(execute(c, root, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, other, "inside", 8);
        number(c, selected, "after", 11);
        unsigned char underflow[] = {0x20, 0}, malformed[] = {0x8b, 2, 0, 0, 0, 0};
        if (!(fvm_avm1_execute_receiver(c, underflow, sizeof(underflow), root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_execute_receiver(c, malformed, sizeof(malformed), root) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->avm1_target_active && !c->avm1_target_clip && !c->avm1_base_clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version == 8)
        {
            if (!(fvm_extensions_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Detached data = {root, selected};
            if (!(fvm_register_function_rooted(c, "detach", detach, &data, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *garbage = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &garbage) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            b.clear();
            target(b, "child");
            integer(b, 0);
            text(b, "detach");
            b.push_back(0x3d);
            b.push_back(0x17);
            text(b, "collected");
            integer(b, 0);
            global(b, "gc");
            text(b, "collect");
            b.push_back(0x52);
            b.push_back(0x1d);
            variable(b, "alive", 77);
            if (!(execute(c, root, b) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, selected, "alive", 77);
            FVM_Value collected;
            if (!(fvm_object_get(c, selected, "collected", &collected) == FVM_OK && collected.number >= 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool present = false;
            for (size_t i = 0; i < c->objects.size(); ++i)
            {
                if (c->objects[i] == selected)
                {
                    present = true;
                }
            }
            if (!(!present))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

#endif
