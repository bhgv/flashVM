#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/value.h"
#include "flashvm/swf.h"
#include "flashvm/input.h"
#include "flashvm/input_queue.h"
#include <map>
#include <string>
#include <vector>
struct FVM_Object {
    std::map<std::string, FVM_Value> properties;
    std::map<unsigned long, FVM_Value> indexed;
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
    std::vector<unsigned char> body;
    std::vector<std::string> parameters;
};
struct FVM_QueuedInput { FVM_InputEvent event; FVM_InputPhase phase; };
struct FVM_Context {
    FVM_Runtime *runtime;
    FVM_Object *global;
    std::vector<FVM_Object *> objects;
    std::vector<char *> strings;
    std::map<std::string, FVM_NativeEntry> functions;
    std::vector<unsigned char> swf_bytes;
    FVM_SWFInfo swf_info;
    bool swf_loaded;
    std::vector<FVM_InputRule> input_rules;
    std::vector<FVM_QueuedInput> queued_input;
    bool pumping_input;
    FVM_InputCallback input_callback;
    void *input_userdata;
    std::map<std::string, FVM_ScriptFunction> script_functions;
    unsigned int script_depth;
};
#endif
