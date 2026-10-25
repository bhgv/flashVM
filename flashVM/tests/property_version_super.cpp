/* A hidden own field must not become the defining owner of an inherited super call. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned route = 0; route < 5; ++route)
        {
            if (route >= 3 && !fvm_jit_available())
            {
                continue;
            }
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_load(c, version);
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *root = 0, *parent = 0, *child = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &root) == FVM_OK &&
                  fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                  fvm_construct(c, "Object", 0, 0, &child) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value, out, name;
            value = class_value(version_function(c, 91));
            if (!(fvm_object_set(c, root, "field", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program body;
            body.variable("this");
            body.text("calls");
            body.variable("this");
            body.text("calls");
            body.op(0x4e);
            body.number(1);
            body.op(0x0a);
            body.op(0x4f);
            body.number(0);
            body.variable("super");
            body.text("field");
            body.op(0x52);
            body.finish();
            FVM_Object *function = 0;
            if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(function);
            if (!(fvm_object_set(c, parent, "field", &value) == FVM_OK &&
                  fvm_object_set_prototype(c, parent, root) == FVM_OK &&
                  fvm_object_set_prototype(c, child, parent) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 777);
            if (!(fvm_object_set(c, child, "field", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 0);
            if (!(fvm_object_set(c, child, "calls", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_flags(c, child, "field", 0x2000, 0);
            value = class_value(child);
            if (!(fvm_set_global(c, "versionTarget", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&name, "field");
            if (!route)
            {
                if (!(fvm_object_call(c, child, "field", 0, 0, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else if (route == 1)
            {
                if (!(fvm_exec_invoke_member(c, child, &name, 0, 0, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                Program code;
                code.number(0);
                code.variable("versionTarget");
                code.text("field");
                code.op(0x52);
                code.finish();
                out = boxing_run(c, code, route - 2, FVM_OK);
            }
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 91 &&
                  fvm_object_get(c, child, "calls", &out) == FVM_OK && out.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
