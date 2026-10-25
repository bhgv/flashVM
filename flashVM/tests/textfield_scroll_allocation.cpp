/* Native scroll observation/export rollback under C++ allocation failures and cached zero-allocation reads. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "textfield_scroll_fixture.hpp"


int main(int argc, char **)
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = scroll_field(c);
    scroll_number(c, field, "scroll", 2);
    scroll_number(c, field, "hscroll", 7);
    if (argc == 1)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary)
        {
            scroll_number(c, field, "_width", 40);
            FVM_TextFieldScrollInfo info;
            std::memset(&info, 77, sizeof(info));
            info.struct_size = sizeof(info);
            FVM_TextFieldScrollInfo old = info;
            allowance = boundary;
            FVM_Result result = fvm_textfield_scroll_info(c, field, &info);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            if (!success)
            {
                ++failures;
                if (!(!std::memcmp(&old, &info, sizeof(info))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(scroll_info(c, field).scroll == 2 && scroll_info(c, field).hscroll == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = false;
        failures = 0;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary)
        {
            FVM_Path *path = (FVM_Path *)1;
            allowance = boundary;
            FVM_Result result = fvm_textfield_path(c, field, &path);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            if (!success)
            {
                ++failures;
                if (!(!path))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                fvm_path_destroy(path);
            }
            if (!(scroll_info(c, field).scroll == 2 && scroll_info(c, field).hscroll == 7))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_TextFieldScrollInfo info = scroll_info(c, field);
    allowance = 0;
    for (unsigned epoch = 0; epoch < 128; ++epoch)
    {
        if (!(fvm_textfield_scroll_info(c, field, &info) == FVM_OK && info.scroll == 2 && info.hscroll == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    allowance = -1;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#else
    (void)argc;
#endif
    return 0;
}
