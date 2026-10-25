/* Authored RECT reads are independent of object bounds, rendering and compressed-file storage. */
#define FVM_SCENE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_scene.cpp"
#include "flashvm/swf.h"


/* Encode one signed rectangle without reusing the production reader. */
static Bytes document(long xmin, long xmax, long ymin, long ymax, unsigned width)
{
    Bits rect;
    rect.put(width, 5);
    rect.put((unsigned long)xmin, width);
    rect.put((unsigned long)xmax, width);
    rect.put((unsigned long)ymin, width);
    rect.put((unsigned long)ymax, width);
    Bytes result;
    result.push_back('F');
    result.push_back('W');
    result.push_back('S');
    result.push_back(8);
    u32(result, (unsigned long)(8 + rect.b.size() + 8));
    append(result, rect.b);
    u16(result, 12 * 256);
    u16(result, 1);
    u16(result, 64);
    u16(result, 0);
    return result;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStageBounds bounds;
    bounds.struct_size = sizeof(bounds);
    bounds.x_min = 99;
    if (!(fvm_swf_stage_bounds(context, &bounds) == FVM_UNSUPPORTED && bounds.x_min == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stage_bounds(0, &bounds) == FVM_INVALID_ARGUMENT && bounds.x_min == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stage_bounds(context, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned width = 2; width <= 31; ++width)
    {
        long edge = (long)((1UL << (width - 1)) - 1);
        Bytes bytes = document(-edge, edge, -1, 1, width);
        if (!(fvm_load_memory(context, bytes.begin(), bytes.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_stage_bounds(context, &bounds) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(bounds.x_min == -edge && bounds.x_max == edge && bounds.y_min == -1 && bounds.y_max == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes reversed = document(1, -1, 0, 0, 2);
    if (!(fvm_load_memory(context, reversed.begin(), reversed.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds.x_min = 99;
    if (!(fvm_swf_stage_bounds(context, &bounds) == FVM_INVALID_ARGUMENT && bounds.x_min == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bounds.struct_size = 0;
    if (!(fvm_swf_stage_bounds(context, &bounds) == FVM_INVALID_ARGUMENT && !bounds.struct_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
