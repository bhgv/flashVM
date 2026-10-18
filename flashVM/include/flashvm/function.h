/* FlashVM: Public embedding API declarations: function.
 * File: include/flashvm/function.h.
 */

#ifndef FLASHVM_FUNCTION_H
/* Define FLASHVM FUNCTION H constant or compilation switch. */
#define FLASHVM_FUNCTION_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Prepared script activation ABI; independent of instruction encoding and VM stack. */
#define FVM_SCRIPT_FRAME_ABI 1UL
    /* [GC:] Borrowed activation values remain rooted until the body returns. */
    typedef struct FVM_ScriptFrame
    {
        unsigned long struct_size;   /* Byte size for ABI checks. */
        unsigned long abi_version;   /* FVM_SCRIPT_FRAME_ABI. */
        FVM_Object *function;        /* Active first-class script function. */
        FVM_Object *receiver;        /* Actual call receiver. */
        FVM_Object *implicit_this;   /* Object-valued entry snapshot; NULL for primitive this. Read current this via execution helpers. */
        FVM_Object *activation;      /* Named locals and retained lexical environment. */
        FVM_Object *arguments;       /* Prepared arguments object, including formal aliases. */
        FVM_Value super_value;       /* Prepared super facade, or undefined if suppressed. */
        unsigned int argc;           /* Actual input argument count. */
        const FVM_Value *argv;       /* Borrowed original argument values. */
        unsigned int register_count; /* Number of mutable register slots, including slot zero. */
        FVM_Value *registers;        /* [GC:] Rooted private slots or a live context-global bank; adopt borrowed text through execution.h. */
        FVM_Object
            *capture; /* [GC:] Optional retained body capture, stable during self-unbinding. */
    } FVM_ScriptFrame;
    /* [GC:] A generated body may collect; root additional temporaries with gc_roots.h.
     * Do not retain frame pointers, unwind across C, destroy the context or change threads.
     * Return status/exception through ordinary FlashVM C APIs. Code/userdata are host-owned. */
    typedef FVM_Result (*FVM_ScriptBody)(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *,
                                         void *);
    /* Attach a context-local native body to an existing script function, preserving bytecode.
     * NULL body requires NULL userdata/capture and restores interpretation. Default graph cloning
     * drops this process-local attachment and retains the original script body. Live
     * interfaces (capability.h) invoke the original function, including its attachment. An active
     * invocation keeps its entry/capture snapshot if it rebinds itself. */
    FVM_API FVM_Result fvm_function_set_body(FVM_Context *, FVM_Object *, FVM_ScriptBody,
                                             void *userdata, FVM_Object *capture);
    /* First-class, context-owned native or AVM1 functions. Default graph cloning of native code requires a
     * stable callback codec. Live interfaces (capability.h) need no callback codec;
     * they execute the retained original callback on its authority thread. */
    /* Create function native. */
    FVM_API FVM_Result fvm_function_create_native(FVM_Context *, FVM_NativeFunction, void *,
                                                  FVM_Object *capture, FVM_Object **);
    /* Create function avm1. */
    FVM_API FVM_Result fvm_function_create_avm1(FVM_Context *, const void *actions, size_t size,
                                                unsigned int parameter_count,
                                                const char *const *parameters,
                                                FVM_Object *environment, FVM_Object **);
    /* Create AVM1 with string-valued counted parameter names, including embedded NUL.
     * UTF-16-equivalent formals share one lexical binding and rightmost arguments alias. */
    FVM_API FVM_Result fvm_function_create_avm1_bytes(FVM_Context *, const void *, size_t,
                                                      unsigned int, const FVM_Value *, FVM_Object *,
                                                      FVM_Object **);
    /* [GC:] Invoke with active function/capture roots. Successful native strings become
       context-owned snapshots; returned objects and primitive metatables must belong to this
       context. Callers must root retained results across GC, including strings in traced mode. */
    /* Invoke function. */
    FVM_API FVM_Result fvm_function_invoke(FVM_Context *, FVM_Object *function,
                                           FVM_Object *receiver, unsigned int argc,
                                           const FVM_Value *, FVM_Value *);
    /* Implement function construct for this subsystem. */
    FVM_API FVM_Result fvm_function_construct(FVM_Context *, FVM_Object *, unsigned int,
                                              const FVM_Value *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
