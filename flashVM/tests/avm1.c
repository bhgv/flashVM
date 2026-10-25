#include "flashvm/avm1.h"


int main(void)
{
    static const unsigned char ok[] = {0x96, 4, 0, 0, 'x', 0, 0, 0};
    static const unsigned char bad[] = {0x96, 255, 255, 0};
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    if (fvm_avm1_inspect(ok, sizeof(ok), &info) != FVM_OK)
    {
        return 1;
    }
    if (info.instruction_count != 2 || info.push_count != 1)
    {
        return 2;
    }
    if (fvm_avm1_inspect(bad, sizeof(bad), &info) != FVM_INVALID_ARGUMENT)
    {
        return 3;
    }
    return 0;
}
