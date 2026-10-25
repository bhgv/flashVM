#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define main exceptions_fixture_main
#include "actionscript_exceptions.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body, caught, empty, code;
    integer(body, 42);
    body.push_back(0x2a);
    string(caught, "observed");
    variable(caught, "error");
    caught.push_back(0x1d);
    attempt(code, body, caught, empty, 1, "error");
    code.push_back(0);
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 3000 && !success; ++boundary)
    {
        FVM_GCRootFrame *roots = c->gc_frames;
        size_t scopes = c->lexical_scopes.size();
        unsigned native = c->gc_native_depth, script = c->script_depth;
        allowance = boundary;
        FVM_Result r = fvm_avm1_execute(c, code.begin(), code.size());
        allowance = -1;
        if (!(c->gc_frames == roots && c->lexical_scopes.size() == scopes && c->gc_native_depth == native &&
              c->script_depth == script && !c->avm1_target_active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            if (!(global(c, "observed") == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(success && failures > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
