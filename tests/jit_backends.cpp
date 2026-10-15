/* Offline encoders use synthetic helpers, never truncated host addresses. */
#include "../src/jit/internal.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
static bool resolve(FVM_JitStep step, unsigned char *address, void *userdata)
{
    unsigned value = step == fvm_jit_checkpoint ? 0x200000 : step == fvm_jit_push ? 0x200100 : 0x200200;
    std::memset(address, 0, 8);
    for (unsigned i = 0; i < 4; ++i) address[i] = (unsigned char)(value >> (i * 8));
    unsigned mode = userdata ? *(unsigned *)userdata : 0;
    if (mode == 1) address[4] = 1;
    if (mode == 2) address[0] |= 1;
    return true;
}
static void save(const char *directory, int target, const char *suffix,
                 const fvm_detail::vector<unsigned char> &bytes)
{
    if (!directory) return;
    char path[1024]; assert(std::strlen(directory) < 900);
    std::sprintf(path, "%s/backend-%d%s.bin", directory, target, suffix);
    FILE *file=std::fopen(path,"wb");assert(file);
    assert(std::fwrite(bytes.begin(),1,bytes.size(),file)==bytes.size());
    assert(std::fclose(file)==0);
}
int main(int argc, char **argv)
{
    FVM_JitCode code;
    code.actions.resize(4);
    code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint, 0xffffffffU));
    code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint, 0x7fffffffU));
    code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint, 0x80000000U));
    code.actions[1].calls.push_back(FVM_JitCall(fvm_jit_push, 1));
    code.actions[1].branch = FVM_JIT_CONDITIONAL;
    code.actions[1].target = 3;
    code.actions[2].calls.push_back(FVM_JitCall(fvm_jit_push, 2));
    code.actions[2].branch = FVM_JIT_JUMP;
    code.actions[2].target = 1;
    code.actions[3].calls.push_back(FVM_JitCall(fvm_jit_push, 3));
    code.actions[3].branch = FVM_JIT_EXIT;
    for (int t = FVM_JIT_TARGET_X86; t <= FVM_JIT_TARGET_RISCV64; ++t)
    {
        fvm_detail::vector<unsigned char> bytes;
        assert(fvm_jit_encode((FVM_JitTarget)t, code, bytes, resolve, 0) == FVM_OK);
        assert(bytes.size() > 32);
        save(argc == 2 ? argv[1] : 0, t, "", bytes);
        if (t == FVM_JIT_TARGET_ARM64 || t == FVM_JIT_TARGET_RISCV64 || t == FVM_JIT_TARGET_ARM)
        {
            unsigned mode = t == FVM_JIT_TARGET_ARM ? 2 : 1;
            assert(fvm_jit_encode((FVM_JitTarget)t, code, bytes, resolve, &mode)==FVM_OK);
            save(argc == 2 ? argv[1] : 0, t, mode == 1 ? "-high" : "-thumb", bytes);
        }
        FVM_JitCode far;
        far.actions = code.actions;
        far.actions.resize(22004);
        far.actions.back() = code.actions.back();
        far.actions[1].target = far.actions.size()-1;
        far.actions[3] = FVM_JitAction();
        for(size_t i=3;i+1<far.actions.size();++i)
        {
            far.actions[i].calls.push_back(FVM_JitCall(fvm_jit_push,99));
            far.actions[i].calls.push_back(FVM_JitCall(fvm_jit_push,99));
        }
        assert(fvm_jit_encode((FVM_JitTarget)t,far,bytes,resolve,0)==FVM_OK);
        assert(bytes.size()>1048576);
        save(argc == 2 ? argv[1] : 0,t,"-far",bytes);
    }
    return 0;
}
