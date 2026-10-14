/* FlashVM: C89 value-based execution services shared by interpreter and generated code.
 * No service accepts bytecode, Action codes or an interpreter operand stack. */
#ifndef FLASHVM_EXECUTION_H
#define FLASHVM_EXECUTION_H /* Public execution service guard. */
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Poll worker cancellation and scheduled GC after accounting work. */
#define FVM_EXEC_POLL 1U
    /* [GC:] Account semantic instruction units in an active interpreted/generated activation.
     * location is a debugger mapping token; no instruction encoding is required. flags=0 only
     * accounts work and updates location. FVM_EXEC_POLL also polls stop requests and GC.
     * All live temporaries must be rooted before polling. Return failure immediately; generated
     * code must not ignore exhausted budget/cancellation statuses. A zero cost can poll alone.
     * Top-level entry resets the budget; nested interpreter/generated calls share it. */
    FVM_API FVM_Result fvm_exec_checkpoint(FVM_Context *, unsigned long units,
                                           unsigned long location, unsigned int flags);
    /* Numeric operations; values are ABI identifiers, not AVM1 instruction codes. */
    typedef enum FVM_ExecutionNumeric
    {
        FVM_EXEC_ADD = 1,       /* Legacy numeric addition, including metamethods. */
        FVM_EXEC_SUBTRACT,      /* Numeric subtraction. */
        FVM_EXEC_MULTIPLY,      /* Numeric multiplication. */
        FVM_EXEC_DIVIDE,        /* Numeric division, including pre-SWF5 #ERROR#. */
        FVM_EXEC_REMAINDER,     /* Floating point remainder. */
        FVM_EXEC_BIT_AND,       /* Signed 32-bit AND. */
        FVM_EXEC_BIT_OR,        /* Signed 32-bit OR. */
        FVM_EXEC_BIT_XOR,       /* Signed 32-bit XOR. */
        FVM_EXEC_SHIFT_LEFT,    /* Masked 32-bit left shift. */
        FVM_EXEC_SHIFT_RIGHT,   /* Arithmetic right shift. */
        FVM_EXEC_SHIFT_UNSIGNED /* Logical right shift with SWF8 signed-result policy. */
    } FVM_ExecutionNumeric;
    /* Dynamic addition and comparison policies, separate from legacy numeric operations. */
    typedef enum FVM_ExecutionCompare
    {
        FVM_EXEC_ADD_DYNAMIC = 1, /* Primitive/string-aware addition. */
        FVM_EXEC_LESS,            /* Version-aware relational comparison. */
        FVM_EXEC_EQUAL,           /* Loose equality with coercion. */
        FVM_EXEC_STRICT_EQUAL,    /* Same-type equality without coercion. */
        FVM_EXEC_GREATER          /* Greater-than with reversed primitive conversion order. */
    } FVM_ExecutionCompare;
    /* Execute dynamic addition or comparison; output may alias either input. */
    FVM_API FVM_Result fvm_exec_compare(FVM_Context *, FVM_ExecutionCompare, const FVM_Value *,
                                        const FVM_Value *, FVM_Value *);
    /* Built-in timeline commands independent of instruction encoding. */
    typedef enum FVM_ExecutionFrame
    {
        FVM_EXEC_NEXT_FRAME = 1, /* Advance one frame. */
        FVM_EXEC_PREVIOUS_FRAME, /* Rewind one frame. */
        FVM_EXEC_PLAY,           /* Enable timeline playback. */
        FVM_EXEC_STOP,           /* Disable timeline playback. */
        FVM_EXEC_GOTO_FRAME      /* Seek a numeric frame or label; playing selects playback. */
    } FVM_ExecutionFrame;
    /* [GC:] Services root validated operands and intermediates during synchronous execution.
     * Context supplies runtime objects, SWF version and host services only. Result aliases are
     * allowed; callers retain results across subsequent collections. Failures leave result intact.
     * Inputs and output pointers are mandatory; no callbacks may destroy the context. */
    /* Execute numeric semantics and preserve the operation's historical coercion order. */
    FVM_API FVM_Result fvm_exec_numeric(FVM_Context *, FVM_ExecutionNumeric, const FVM_Value *,
                                        const FVM_Value *, FVM_Value *);
    /* Read/write indexed display properties; write selects SetProperty and ignores output value. */
    FVM_API FVM_Result fvm_exec_display_property(FVM_Context *, int write, const FVM_Value *target,
                                                 const FVM_Value *index, const FVM_Value *value,
                                                 FVM_Value *result);
    /* Extract one-based string ranges; ordinary and MB variants share supported UTF-16 policy. */
    FVM_API FVM_Result fvm_exec_string_extract(FVM_Context *, const FVM_Value *input,
                                               const FVM_Value *start, const FVM_Value *length,
                                               FVM_Value *result);
    /* Select a display target from a value; empty paths reset and undefined follows SWF policy. */
    FVM_API FVM_Result fvm_exec_select_target(FVM_Context *, const FVM_Value *);
    /* Coerce and send a URL request; flags follow the public FVM_AVM1_URL_* contract. */
    FVM_API FVM_Result fvm_exec_url(FVM_Context *, const FVM_Value *url, const FVM_Value *target,
                                    unsigned int flags);
    /* Coerce and deliver trace, including when its host sink is disabled. */
    FVM_API FVM_Result fvm_exec_trace(FVM_Context *, const FVM_Value *);
    /* Read or write a value member; native raw tables retain their indexed-slot policy. */
    FVM_API FVM_Result fvm_exec_member(FVM_Context *, int write, const FVM_Value *owner,
                                       const FVM_Value *member, const FVM_Value *value,
                                       FVM_Value *result);
    /* Execute a built-in frame command on an explicit clip. */
    FVM_API FVM_Result fvm_exec_frame_control(FVM_Context *, FVM_Object *, FVM_ExecutionFrame,
                                              const FVM_Value *frame, int playing);
    /* Call frame actions, including clip:frame paths, without seeking the timeline. */
    FVM_API FVM_Result fvm_exec_call_frame(FVM_Context *, FVM_Object *scope_receiver,
                                           const FVM_Value *frame);
#ifdef __cplusplus
}
#endif
#endif
