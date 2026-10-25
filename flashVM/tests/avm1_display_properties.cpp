/* Classic display-property indices test target resolution, coercion ordering, scalar rules and GC. */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_target_scope.cpp"

static unsigned conversions, sequence[3];


static FVM_Result convert(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    sequence[conversions++] = id;
    if (id == 2)
    {
        return fvm_value_string(out, "child");
    }
    fvm_value_number(out, id == 1 ? 0 : 2.12);
    return FVM_OK;
}


static void inputs(FVM_Context *c, const FVM_Value &target, const FVM_Value &index, const FVM_Value *value)
{
    if (!(fvm_set_global(c, "p_target", &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "p_index", &index) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (value)
    {
        if (!(fvm_set_global(c, "p_value", value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


static FVM_Value get(FVM_Context *c, FVM_Object *root, const FVM_Value &path, const FVM_Value &index)
{
    inputs(c, path, index, 0);
    Bytes b;
    text(b, "result");
    global(b, "p_target");
    global(b, "p_index");
    b.push_back(0x22);
    b.push_back(0x1d);
    if (!(execute(c, root, b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_object_get(c, root, "result", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


static FVM_Result
set(FVM_Context *c, FVM_Object *root, const FVM_Value &path, const FVM_Value &index, const FVM_Value &value)
{
    inputs(c, path, index, &value);
    Bytes b;
    global(b, "p_target");
    global(b, "p_index");
    global(b, "p_value");
    b.push_back(0x23);
    return execute(c, root, b);
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
        if (!(fvm_movieclip_create(c, "root", 3, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value root_value;
        fvm_value_undefined(&root_value);
        root_value.type = FVM_VALUE_OBJECT;
        root_value.object = root;
        if (!(fvm_set_global(c, "root", &root_value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *selected = child(c, root, "child", 1);
        FVM_Value path, index, value, out;
        fvm_value_string(&path, "child");
        fvm_value_number(&index, 0);
        fvm_value_number(&value, 2.12);
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out = get(c, root, path, index);
        if (!(out.type == FVM_VALUE_NUMBER && out.number == 2.1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, -2.12);
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == -2.1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, std::numeric_limits<double>::infinity());
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == -2147483648.0 / 20))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 1e300);
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == 2147483647.0 / 20))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 7);
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_NULL;
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN());
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 7);
        fvm_value_string(&value, "false");
        if (!(set(c, root, path, index, value) == FVM_OK &&
              get(c, root, path, index).boolean == (version == 4 ? 0 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 0);
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out = get(c, root, path, index);
        if (!(out.type == FVM_VALUE_BOOLEAN && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 10);
        fvm_value_number(&value, 181);
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == -179))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, -540);
        if (!(set(c, root, path, index, value) == FVM_OK && get(c, root, path, index).number == -180))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 4);
        if (!(get(c, root, path, index).number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 5);
        if (!(get(c, root, path, index).number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 12);
        if (!(get(c, root, path, index).number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 11);
        if (!(!std::strcmp(get(c, root, path, index).string, "/child")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, -0.5);
        if (!(get(c, root, path, index).number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, -1);
        if (!(get(c, root, path, index).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 22);
        if (!(get(c, root, path, index).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 0);
        fvm_value_string(&path, "missing");
        if (!(get(c, root, path, index).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 33);
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Index, target string, then assigned value are converted in that order. */
        unsigned ids[] = {1, 2, 3};
        FVM_Object *objects[3];
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!(fvm_construct(c, "Object", 0, 0, objects + i) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(
                      c, objects[i], i == 1 ? "toString" : "valueOf", convert, ids + i, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_undefined(&index);
        index.type = FVM_VALUE_OBJECT;
        index.object = objects[0];
        path = index;
        path.object = objects[1];
        value = index;
        value.object = objects[2];
        conversions = 0;
        if (!(set(c, root, path, index, value) == FVM_OK && conversions == 3 && sequence[0] == 1 && sequence[1] == 2 &&
              sequence[2] == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, selected, "_x", 2.1);
        fvm_value_number(&index, 4);
        conversions = 0;
        if (!(set(c, root, path, index, value) == FVM_OK && conversions == 1 && sequence[0] == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 8);
        conversions = 0;
        if (!(set(c, root, path, index, value) == FVM_OK && conversions == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&index, 14);
        if (!(set(c, root, path, index, value) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&path, "child");
        fvm_value_number(&index, 13);
        fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN());
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::strcmp(get(c, root, path, index).string, version >= 7 ? "0" : "NaN")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "child");
        if (!(set(c, root, path, index, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char empty_get[] = {0x22, 0}, empty_set[] = {0x23, 0};
        if (!(fvm_avm1_execute_receiver(c, empty_get, sizeof(empty_get), root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_execute_receiver(c, empty_set, sizeof(empty_set), root) == FVM_OK))
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
            Bytes body;
            integer(body, 0);
            text(body, "detach");
            body.push_back(0x3d);
            body.push_back(0x17);
            text(body, "collected");
            integer(body, 0);
            global(body, "gc");
            text(body, "collect");
            body.push_back(0x52);
            body.push_back(0x1d);
            integer(body, 13);
            body.push_back(0x3e);
            body.push_back(0);
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value function;
            fvm_value_undefined(&function);
            function.type = FVM_VALUE_OBJECT;
            function.object = fn;
            if (!(fvm_object_set(c, objects[2], "valueOf", &function) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = objects[2];
            fvm_value_number(&index, 0);
            fvm_value_string(&path, "child");
            FVM_Object *garbage = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &garbage) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, selected, "_x", 13);
            FVM_Value collected;
            if (!(fvm_object_get(c, root, "collected", &collected) == FVM_OK && collected.number >= 1))
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
