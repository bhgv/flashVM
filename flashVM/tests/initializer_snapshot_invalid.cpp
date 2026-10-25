/* Ownership/topology/cap failures leave the cursor untouched and restore initializer/tick guards. */
#include <stdio.h>
#include <stdlib.h>
#include "initializer_progress_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 9; ++mode)
    {
        FVM_Context *c = tick_context(rt), *other = tick_context(rt);
        FVM_Object *root = init_root(c, 8, 2), *foreign = tick_root(other);
        InitProbe p;
        init_bind(c, root, p);
        FVM_Object *data = root->resource_result.object;
        FVM_Value library = data->properties["library"], table = p.library->properties["initializers"],
                  source = p.library->properties["root"];
        if (mode == 0)
        {
            data->properties["library"] = tick_object(foreign);
        }
        if (mode == 1)
        {
            p.library->properties["initializers"] = tick_object(foreign);
        }
        if (mode == 2)
        {
            p.library->properties["initializers"] = tick_object(0);
        }
        if (mode == 3)
        {
            p.library->properties["root"] = tick_object(foreign);
        }
        if (mode == 4)
        {
            p.library->properties["root"] = tick_object(0);
        }
        if (mode == 5)
        {
            FVM_Value code = p.table->indexed[1];
            p.table->indexed.erase(1);
            p.table->indexed[2] = code;
        }
        if (mode == 6)
        {
            FVM_Value code = p.table->indexed[0];
            for (unsigned long i = 2; i <= FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS; ++i)
            {
                p.table->indexed[i] = code;
            }
        }
        if (mode == 7)
        {
            fvm_value_number(&p.library->properties["initializers"], 3);
        }
        if (mode == 8)
        {
            p.library->swf_initializer_next = 2;
            p.library->properties["root"] = tick_object(foreign);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(c, p);
        if (!(!p.calls[0] && !p.calls[1] && p.library->swf_initializer_next == (mode == 8 ? 2UL : 0UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        data->properties["library"] = library;
        p.library->properties["initializers"] = table;
        p.library->properties["root"] = source;
        p.table->indexed.clear();
        p.table->indexed[0] = init_hex(c, init_call(0));
        p.table->indexed[1] = init_hex(c, init_call(1));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.library->swf_initializer_next == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(c, p);
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    fvm_runtime_destroy(rt);
}
