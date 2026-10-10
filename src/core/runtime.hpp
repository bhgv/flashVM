#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/flashvm.h"
struct FVM_Runtime {
    FVM_Config config;
};
struct FVM_Context {
    FVM_Runtime *runtime;
};
#endif
