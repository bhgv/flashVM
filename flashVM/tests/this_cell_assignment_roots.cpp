/* Active/suspended this strings and metadata survive child mutation, GC and errors. */
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "this_cell_assignment.cpp"
#include "../src/core/this_cell.hpp"
#include "flashvm/exception.h"

static FVM_Value expected_this;
static FVM_Object *this_capture;
static unsigned this_failure;


static FVM_Result this_probe(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (!(c->script_this_cell && c->script_this_cell->previous))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    boxing_equal(c->script_this_cell->value, expected_this);
    FVM_ThisCell *parent = c->script_this_cell->previous;
    boxing_equal(parent->value, expected_this);
    int removed = 0;
    if (!(fvm_object_delete(c, c->global, "replacement", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "probe", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "undefinedThisFunction2", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    char bytes[] = {'a', 0, 'b'};
    FVM_Value name, value;
    fvm_value_string(&name, "this");
    fvm_value_string_bytes(&value, bytes, sizeof(bytes));
    if (!(fvm_exec_variable_set(c, 0, c->global, c->global, &name, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bytes[0] = '?';
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = c->script_this_cell->value;
    if (!(value.type == FVM_VALUE_STRING && fvm_string_size(value) == 3 && !std::memcmp(value.string, "a\0b", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    boxing_equal(parent->value, expected_this);
    if (!(parent->value.metatable == expected_this.metatable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool capture_alive = false, edge_alive = expected_this.type != FVM_VALUE_OBJECT && !expected_this.metatable;
    FVM_Object *edge = expected_this.type == FVM_VALUE_OBJECT ? expected_this.object : expected_this.metatable;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        capture_alive |= c->objects[i] == this_capture;
        edge_alive |= c->objects[i] == edge;
    }
    if (!(capture_alive && edge_alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (this_failure == 1)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (this_failure == 2)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 7; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (this_failure = 0; this_failure < 3; ++this_failure)
            {
                for (unsigned mode = 0; mode < 3; ++mode)
                {
                    if (mode && !fvm_jit_available())
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
                    if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *edge = 0;
                    if (!(fvm_construct(c, "Object", 0, 0, &edge) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (type == 0)
                    {
                        expected_this = class_value(edge);
                    }
                    if (type == 1)
                    {
                        fvm_value_string_bytes(&expected_this, "p\0q", 3);
                        expected_this.metatable = edge;
                    }
                    if (type == 2)
                    {
                        fvm_value_number(&expected_this, 19);
                        expected_this.metatable = edge;
                    }
                    if (!(fvm_set_global(c, "replacement", &expected_this) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &this_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *probe = undefined_this_function(c, 5);
                    if (!(fvm_function_set_body(c, probe, this_probe, 0, this_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(probe);
                    if (!(fvm_set_global(c, "probe", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program body;
                    body.text("this");
                    body.variable("replacement");
                    body.op(0x1d);
                    body.number(0);
                    body.text("probe");
                    body.op(0x3d);
                    body.op(0x17);
                    body.variable("this");
                    body.finish();
                    FVM_Result status = this_failure == 1 ? FVM_EXCEPTION : this_failure == 2 ? FVM_ERROR : FVM_OK;
                    value = boxing_run(c, body, mode, status);
                    if (!this_failure)
                    {
                        boxing_equal(value, expected_this);
                        if (!(value.metatable == expected_this.metatable))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->script_this_cell && !c->script_this && !c->script_this_undefined && !c->gc_frames &&
                          !c->script_depth && !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
