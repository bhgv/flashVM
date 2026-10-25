/* Counted paths distinguish slash/dot modes, child shadowing, prototype fallback and live GC roots. */
#define main paths_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_paths.cpp"

#undef main
#include "flashvm/extensions.h"
#include "flashvm/table.h"


static void resolve(FVM_Context *c, FVM_Object *start, const char *text, size_t size, FVM_Object *expected)
{
    FVM_Value value;
    if (!(fvm_value_string_bytes(&value, text, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *out = start;
    if (!(fvm_movieclip_resolve_path(c, start, value, &out) == FVM_OK && out == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void named(FVM_Context *c, FVM_Object *start, const char *text, FVM_Object *expected)
{
    resolve(c, start, text, std::strlen(text), expected);
}


static FVM_Result factory(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *obj = 0;
    FVM_Result result = fvm_construct(c, "Object", 0, 0, &obj);
    if (result == FVM_OK)
    {
        result = fvm_table_set_metatable(c, obj, (FVM_Object *)raw);
    }
    FVM_Value alive;
    fvm_value_number(&alive, 42);
    if (result == FVM_OK)
    {
        result = fvm_object_set(c, obj, "alive", &alive);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = obj;
    return result;
}


static void index(FVM_Context *c, FVM_Object *meta, Bytes b, bool owner)
{
    b.push_back(0x3e);
    b.push_back(0);
    const char *parameters[] = {"owner"};
    FVM_Object *function = 0;
    if (!(fvm_function_create_avm1(c, b.begin(), b.size(), owner ? 1 : 0, owner ? parameters : 0, 0, &function) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    if (!(fvm_object_set(c, meta, "__index", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


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
        if (!(fvm_movieclip_create(c, "host", 3, &root) == FVM_OK))
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
        FVM_Object *a = child(c, root, "a", 1), *b = child(c, a, "b", 2), *dot = child(c, a, "x.y", 3),
                   *literal = child(c, root, "a.b", 4);
        named(c, root, "", root);
        named(c, b, "/", root);
        named(c, root, "a/b", b);
        named(c, root, "a.b", version >= 5 ? b : literal);
        named(c, root, "a/x.y", dot);
        named(c, b, "../x.y", dot);
        named(c, root, "../a", 0);
        named(c, root, "missing", 0);
        named(c, root, "A/B", version < 7 ? b : 0);
        if (version >= 5)
        {
            named(c, b, "_root.a.b", b);
            named(c, b, "_level0.a.b", b);
            named(c, root, ":::a:b", b);
            named(c, b, "..:_parent", root);
        }
        FVM_Object *plain = 0, *destination = child(c, root, "destination", 5);
        if (!(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = plain;
        if (!(fvm_object_set(c, root, "alias", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = destination;
        if (!(fvm_object_set(c, plain, "target", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        named(c, root, "alias/target", destination);
        named(c, root, "alias/..", 0);
        value.object = destination;
        if (!(fvm_object_set(c, root, "a", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        named(c, root, "a/b", b);
        if (version >= 5)
        {
            named(c, root, "alias.target", destination);
        }
        if (version >= 7)
        {
            fvm_value_string_bytes(&value, "x\0y", 3);
            if (!(fvm_object_set(c, b, "_name", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            resolve(c, root, "a/x\0y", 5, b);
            fvm_value_string(&value, "\xc3\xa9");
            if (!(fvm_object_set(c, dot, "_name", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            named(c, root, "a/\xc3\xa9", dot);
        }
        else
        {
            fvm_value_string(&value, "\xc3\xa9");
            FVM_Object *out = 0;
            if (!(fvm_movieclip_resolve_path(c, root, value, &out) == FVM_OK && !out))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (version == 8)
        {
            if (!(fvm_extensions_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *first = 0, *second = 0;
            if (!(fvm_object_create(c, &first) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_create(c, &second) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_register_function_rooted(c, "factory", factory, second, second) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes body;
            integer(body, 0);
            text(body, "factory");
            body.push_back(0x3d);
            index(c, first, body, false);
            body.clear();
            text(body, "collected");
            integer(body, 0);
            global(body, "gc");
            text(body, "collect");
            body.push_back(0x52);
            body.push_back(0x1d);
            global(body, "owner");
            index(c, second, body, true);
            if (!(fvm_table_set_metatable(c, root, first) == FVM_OK))
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
            fvm_value_string(&value, "unknown.again");
            FVM_Object *out = 0;
            if (!(fvm_movieclip_resolve_path(c, root, value, &out) == FVM_OK && out))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value alive;
            if (!(fvm_object_get(c, out, "alive", &alive) == FVM_OK && alive.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "collected", &alive) == FVM_OK && alive.number >= 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_set_metatable(c, root, 0) == FVM_OK))
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
