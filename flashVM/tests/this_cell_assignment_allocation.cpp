/* Borrowed this strings publish atomically and retain old/new metadata across OOM/retry. */
#define main this_assignment_allocator_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include "this_cell_assignment.cpp"
#include "../src/core/this_cell.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool failed = false, success = false;
    for (long boundary = 0; boundary < 256 && !success; ++boundary)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        version_load(c, 8);
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *old_meta = 0, *new_meta = 0;
        if (!(fvm_object_create(c, &old_meta) == FVM_OK && fvm_object_create(c, &new_meta) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value old, value, name, link = class_value(new_meta);
        if (!(fvm_set_global(c, "newMeta", &link) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&old, "o\0d", 3);
        old.metatable = old_meta;
        if (!(fvm_native_result(c, FVM_OK, &old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        {


            FVM_ThisCellScope cell(c, old);
            char bytes[] = {'n', 0, 'w'};
            fvm_value_string_bytes(&value, bytes, 3);
            value.metatable = new_meta;
            fvm_value_string(&name, "this");
            allowance = boundary;
            FVM_Result status = fvm_exec_variable_set(c, 0, c->global, c->global, &name, &value);
            allowance = -1;
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                boxing_equal(c->script_this_cell->value, old);
                if (!(c->script_this_cell->value.metatable == old_meta))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                boxing_equal(c->script_this_cell->value, old);
                if (!(fvm_exec_variable_set(c, 0, c->global, c->global, &name, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(status == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = true;
            }
            bytes[0] = '?';
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value out = c->script_this_cell->value;
            if (!(out.type == FVM_VALUE_STRING && out.metatable == new_meta && fvm_string_size(out) == 3 &&
                  !std::memcmp(out.string, "n\0w", 3)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->script_this_cell && !c->script_this && !c->script_this_undefined))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(failed && success))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
}
