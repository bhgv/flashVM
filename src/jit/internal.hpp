/* FlashVM: immutable compilation records and rooted native invocation state. */
#ifndef FVM_JIT_INTERNAL_HPP
#define FVM_JIT_INTERNAL_HPP /* Native compiler private interface guard. */
#include "flashvm/jit.h"
#include "../avm1/handlers/support.hpp"
#include "constants.h"
#include "backend.hpp"
#include "../../ports/code_port.h"
struct FVM_JitInvocation;
/* Each emitted call has a fixed semantic helper, never a bytecode-dispatch callback. */
typedef FVM_Result (*FVM_JitStep)(FVM_JitInvocation *, unsigned int);
/* One compile-time selected helper invocation. */
struct FVM_JitCall
{
    FVM_JitStep step;     /* Direct helper address embedded in native code. */
    unsigned int operand; /* Constant index or value-based operation identifier. */
    /* Initialize an emitted helper operand or an empty vector capacity slot. */
    FVM_JitCall(FVM_JitStep s = 0, unsigned int o = 0) : step(s), operand(o)
    {
    }
};
/* One source action maps to one poll/budget checkpoint and its emitted calls. */
struct FVM_JitAction
{
    size_t offset;                         /* Source location reported to the shared debugger. */
    fvm_detail::vector<FVM_JitCall> calls; /* Compile-time unrolled operations. */
    int branch;    /* Zero fallthrough, one unconditional, two conditional, three exit. */
    size_t target; /* Validated source byte offset for control-flow relocation. */
    /* Initialize a source block before lowering its instructions. */
    FVM_JitAction() : offset(0), branch(FVM_JIT_FALLTHROUGH), target(0)
    {
    }
};
/* Immutable literal storage; strings are borrowed only until runtime adoption. */
struct FVM_JitConstant
{
    FVM_Value value;         /* Primitive numeric/null/undefined/string tag and payload. */
    fvm_detail::string text; /* Owned bytes for a string constant. */
    /* No literal initially owns managed state or borrowed source bytes. */
    FVM_JitConstant()
    {
        fvm_value_undefined(&value);
    }
};
/* One immutable encoded ConstantPool replacement, independent of managed string ownership. */
struct FVM_JitPool
{
    fvm_detail::vector<fvm_detail::string> entries; /* Source-owned terminated byte strings. */
};
/* Host-owned program and executable mapping, intentionally outside graph transport. */
struct FVM_JitCode
{
    fvm_detail::vector<unsigned char>
        source; /* Owned source snapshot for root diagnostic replay. */
    fvm_detail::vector<FVM_JitConstant> constants; /* No managed references. */
    fvm_detail::vector<FVM_JitPool> pools;         /* Immutable complete pool replacements. */
    size_t pool_entries; /* Bounded aggregate pool entry count, independent of literal records. */
    fvm_detail::vector<FVM_JitAction> actions;     /* Source map and compile-time call plan. */
    void *memory;                                  /* RX-only native instruction mapping. */
    size_t mapped;                                 /* Complete OS mapping length. */
    size_t bytes;                                  /* Actual emitted instruction byte count. */
    unsigned int active; /* Reentrant same-thread invocation count; prevents active release. */
    /* No executable mapping is owned until the compiler publishes one. */
    FVM_JitCode() : pool_entries(0), memory(0), mapped(0), bytes(0), active(0)
    {
    }
    /* Release an inactive mapping and then the immutable compile-time data. */
    ~FVM_JitCode()
    {
        fvm_port_code_release(memory, mapped);
    }
};
/* Stack lives in an execution root frame throughout native calls and polling. */
struct FVM_JitInvocation
{
    FVM_Context *context;                 /* Owning invocation context. */
    const FVM_ScriptFrame *frame;         /* Borrowed prepared activation ABI. */
    FVM_JitCode *code;                    /* Immutable native program, protected by active count. */
    fvm_detail::vector<FVM_Value> *stack; /* [GC:] Rooted dynamic operands. */
    const fvm_detail::vector<fvm_detail::string> *pool; /* Activation snapshot or selected code pool. */
    FVM_Value result;    /* Returned value copied before stack roots leave scope. */
    unsigned long steps; /* Invocation-local checkpoint polling cadence. */
    bool condition;      /* Latest popped branch truth value. */
};
/* Runtime helper families; all exceptions are contained before returning to machine code. */
FVM_Result fvm_jit_checkpoint(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_push(FVM_JitInvocation *, unsigned int);
/* Select an immutable pool only when the encoded ConstantPool action actually executes. */
FVM_Result fvm_jit_set_pool(FVM_JitInvocation *, unsigned int);
/* Adopt a selected/captured pool entry before pushing it into managed state. */
FVM_Result fvm_jit_push_pool(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_numeric(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_compare(FVM_JitInvocation *, unsigned int);
/* Convert or transform a rooted top operand through the shared unary C services. */
FVM_Result fvm_jit_unary(FVM_JitInvocation *, unsigned int);
/* Push a prepared local/global register, retaining the bank's versioned bounds policy. */
FVM_Result fvm_jit_load_register(FVM_JitInvocation *, unsigned int);
/* Store the top operand without popping it; global banks own copied strings. */
FVM_Result fvm_jit_store_register(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_get_variable(FVM_JitInvocation *, unsigned int);
/* Assign a scoped variable using the interpreter's target and script-this rules. */
FVM_Result fvm_jit_set_variable(FVM_JitInvocation *, unsigned int);
/* Declare an initialized/uninitialized binding in the prepared activation. */
FVM_Result fvm_jit_declare_variable(FVM_JitInvocation *, unsigned int);
/* Resolve or assign an owner/key pair through shared member semantics. */
FVM_Result fvm_jit_member(FVM_JitInvocation *, unsigned int);
/* Invoke or construct a method through the rooted shared AVM1 service. */
FVM_Result fvm_jit_call_method(FVM_JitInvocation *, unsigned int);
/* Invoke a named function or constructor through shared lexical/target lookup. */
FVM_Result fvm_jit_call_named(FVM_JitInvocation *, unsigned int);
/* Select a timeline target from a rooted stack value. */
FVM_Result fvm_jit_target_value(FVM_JitInvocation *, unsigned int);
/* Select a timeline target from an immutable code-owned path literal. */
FVM_Result fvm_jit_target_literal(FVM_JitInvocation *, unsigned int);
/* Delete an explicit owner/key pair and replace operands with its Boolean. */
FVM_Result fvm_jit_delete_member(FVM_JitInvocation *, unsigned int);
/* Delete a scoped or qualified variable reference. */
FVM_Result fvm_jit_delete_reference(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_return(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_condition(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_truth(FVM_JitInvocation *, unsigned int);
FVM_Result fvm_jit_stack(FVM_JitInvocation *, unsigned int);
/* Contain C++ unwinding in compiled helpers: executable pages have no unwind metadata. */
template <FVM_JitStep Step> FVM_Result fvm_jit_safe(FVM_JitInvocation *state, unsigned int operand)
{
    try
    {
        return Step(state, operand);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
/* Validate/decode before any executable mapping is allocated. */
FVM_Result fvm_jit_decode(const unsigned char *, size_t, FVM_JitCode &);
/* Emit relocatable System V AMD64 blocks and seal their mapping. */
FVM_Result fvm_jit_emit(FVM_JitCode &);
#endif
