#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/value.h"
#include <map>
#include <string>
#include <vector>
struct FVM_Object {
    std::map<std::string, FVM_Value> properties;
    FVM_Context *owner;
};
struct FVM_Runtime { FVM_Config config; };
struct FVM_NativeEntry {
    FVM_NativeFunction callback;
    void *userdata;
};
struct FVM_Context {
    FVM_Runtime *runtime;
    FVM_Object *global;
    std::vector<FVM_Object *> objects;
    std::vector<char *> strings;
    std::map<std::string, FVM_NativeEntry> functions;
};
#endif
