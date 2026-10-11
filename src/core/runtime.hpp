#ifndef FLASHVM_INTERNAL_RUNTIME_HPP
#define FLASHVM_INTERNAL_RUNTIME_HPP
#include "flashvm/value.h"
#include "flashvm/swf.h"
#include "flashvm/input.h"
#include "flashvm/input_queue.h"
#include "../details/hash_table.hpp"
#include "../details/string.hpp"
#include "../details/vector.hpp"
struct FVM_FontSource;
struct FVM_Stack;
struct FVM_Worker;
typedef FVM_Result (*FVM_StackCallbackInternal)(FVM_Context *, FVM_Stack *, void *);
struct FVM_StackEntry
{
    FVM_StackCallbackInternal callback;
    int (*cfunction)(FVM_Stack *);
    void *userdata;
    FVM_Object *root;
    FVM_StackEntry() : callback(0), cfunction(0), userdata(0), root(0)
    {
    }
};
struct FVM_NativeEntry
{
    bool collector_entry;
    FVM_Object *root;
    FVM_NativeEntry() : collector_entry(false), root(0), callback(0), userdata(0)
    {
    }
    FVM_NativeFunction callback;
    void *userdata;
};
struct FVM_AccessorEntry
{
    FVM_NativeFunction getter, setter;
    void *userdata;
    FVM_Object *root;
    FVM_AccessorEntry() : getter(0), setter(0), userdata(0), root(0) {}
};
struct FVM_Object
{
    fvm_detail::map<fvm_detail::string, FVM_Value> properties;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry> accessors;
    fvm_detail::map<unsigned long, FVM_Value> indexed;
    FVM_Object *metatable;
    FVM_Context *owner;
    FVM_Object *prototype;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry> metamethods;
    bool finalized;
    bool gc_marked;
    unsigned long gc_pins;
    FVM_StackEntry callable;
    bool resource_result_valid;
    FVM_Value resource_result;
    int resource_kind;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry> methods;
    void *resource;
    void (*release_resource)(void *);
};
struct FVM_Runtime
{
    FVM_Config config;
};
struct FVM_ScriptFunction
{
    fvm_detail::vector<fvm_detail::string> constants;
    fvm_detail::vector<unsigned char> body;
    fvm_detail::vector<fvm_detail::string> parameters;
};
struct FVM_QueuedInput
{
    FVM_InputEvent event;
    FVM_InputPhase phase;
};
struct FVM_GCRootFrame
{
    FVM_GCRootFrame *previous;
    fvm_detail::vector<FVM_Value> *stack;
    fvm_detail::map<fvm_detail::string, FVM_Value> *locals;
    FVM_Object *receiver;
};
struct FVM_Context
{
    FVM_Worker *worker;
    bool standard_library;
    bool standard_text_library;
    fvm_detail::map<fvm_detail::string, FVM_FontSource *> fonts;
    fvm_detail::string default_font;
    unsigned long random_state;
    FVM_Runtime *runtime;
    fvm_detail::map<fvm_detail::string, FVM_StackEntry> stack_functions;
    fvm_detail::vector<fvm_detail::vector<FVM_Value> *> gc_stacks;
    FVM_Object *global;
    fvm_detail::vector<FVM_Object *> objects;
    fvm_detail::vector<char *> strings;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry> functions;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry> constructors;
    fvm_detail::map<fvm_detail::string, FVM_NativeFunction> serialization_callbacks;
    fvm_detail::vector<unsigned char> swf_bytes;
    FVM_SWFInfo swf_info;
    bool swf_loaded;
    fvm_detail::vector<FVM_InputRule> input_rules;
    fvm_detail::vector<FVM_QueuedInput> queued_input;
    bool pumping_input;
    FVM_InputCallback input_callback;
    void *input_userdata;
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction> script_functions;
    unsigned int script_depth;
    int gc_mode;
    unsigned long gc_allocations;
    bool gc_collecting;
    bool destroying;
    bool gc_releasing;
    FVM_GCRootFrame *gc_frames;
    unsigned int gc_native_depth;
};
void fvm_standard_text_cleanup(FVM_Context *);
/* Shared bounded method lookup for C API and AVM1. Noncallable own properties
 * shadow inherited callbacks; NOT_FOUND permits legacy named-function fallback. */
FVM_Result fvm_lookup_native_method(FVM_Object *, const char *, FVM_NativeEntry *);
unsigned long fvm_numeric_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_numeric_callback_from_id(unsigned long);
unsigned long fvm_text_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_text_callback_from_id(unsigned long);
FVM_Result fvm_release_object_resource(FVM_Context *, FVM_Object *);
FVM_Result fvm_validate_value(FVM_Context *, const FVM_Value *);
#endif
