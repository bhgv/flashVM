#include "flashvm/swf.h"


int main(void)
{
    static const unsigned char swf[] = {'F', 'W', 'S', 6, 16, 0, 0, 0, 0x08, 0, 0, 12, 1, 0, 0, 0};
    FVM_SWFInfo info;
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *ctx = 0;
    info.struct_size = sizeof(info);
    if (fvm_swf_inspect(swf, sizeof(swf), &info) != FVM_OK)
    {
        return 1;
    }
    if (info.version != 6 || info.frame_count != 1 || info.tag_count != 1)
    {
        return 2;
    }
    if (fvm_config_init(&cfg) != FVM_OK || fvm_runtime_create(&cfg, &rt) != FVM_OK)
    {
        return 3;
    }
    if (fvm_context_create(rt, &ctx) != FVM_OK)
    {
        return 4;
    }
    if (fvm_load_memory(ctx, swf, sizeof(swf)) != FVM_OK)
    {
        return 5;
    }
    info.struct_size = sizeof(info);
    if (fvm_swf_info(ctx, &info) != FVM_OK || info.version != 6)
    {
        return 6;
    }
    fvm_context_destroy(ctx);
    fvm_runtime_destroy(rt);
    return 0;
}
