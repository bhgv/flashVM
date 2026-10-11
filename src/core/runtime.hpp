#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/value.h"
#include "flashvm/swf.h"
#include "flashvm/input.h"
#include "flashvm/input_queue.h"
#include "../details/hash_table.hpp"
#include <string>
#include "../details/vector.hpp"
struct FVM_Object {
    fvm_detail::map<std::string, FVM_Value> properties;
    fvm_detail::map<unsigned long, FVM_Value> indexed;
    FVM_Object *metatable;
    FVM_Context *owner;
    FVM_Object *prototype;
};
struct FVM_Runtime { FVM_Config config; };
struct FVM_NativeEntry {
    FVM_NativeFunction callback;
    void *userdata;
};
struct FVM_ScriptFunction {
    fvm_detail::vector<unsigned char> body;
    fvm_detail::vector<std::string> parameters;
};
struct FVM_QueuedInput { FVM_InputEvent event; FVM_InputPhase phase; };
struct FVM_Context {
    FVM_Runtime *runtime;
    FVM_Object *global;
    fvm_detail::vector<FVM_Object *> objects;
    fvm_detail::vector<char *> strings;
    fvm_detail::map<std::string, FVM_NativeEntry> functions;
    fvm_detail::vector<unsigned char> swf_bytes;
    FVM_SWFInfo swf_info;
    bool swf_loaded;
    fvm_detail::vector<FVM_InputRule> input_rules;
    fvm_detail::vector<FVM_QueuedInput> queued_input;
    bool pumping_input;
    FVM_InputCallback input_callback;
    void *input_userdata;
    fvm_detail::map<std::string, FVM_ScriptFunction> script_functions;
    unsigned int script_depth;
};
#endif
