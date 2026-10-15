/* FlashVM: execution selection and interpreter diagnostic bridges. */
#ifndef FVM_EXECUTION_MODE_INTERNAL_HPP
#define FVM_EXECUTION_MODE_INTERNAL_HPP /* Internal runtime policy interface guard. */
#include "../core/runtime.hpp"
/* Diagnostic replay overrides the requested policy until all retry calls return. */
FVM_ExecutionMode fvm_execution_effective_mode(FVM_Context *);
/* Find/compile an automatic body; null code means safe interpreter fallback. */
FVM_Result fvm_execution_select_code(FVM_Context *, const void *, size_t, FVM_JitCode **);
/* Try a validated root body, preserving normal interpreter fallback and root activation rules. */
FVM_Result fvm_execution_try_root(FVM_Context *, const void *, size_t, FVM_Object *, bool *handled);
/* Emit a step event only during diagnostic replay; the ordinary native path remains quiet. */
void fvm_execution_trace_step(FVM_Context *, unsigned long);
/* Retry a whole script with fresh activation, then restore the original exception and budget. */
void fvm_execution_replay_function(FVM_Context *, const fvm_detail::string &,
                                   const FVM_ScriptFunction &, unsigned int, const FVM_Value *,
                                   FVM_Object *, FVM_Object *, FVM_Object *, FVM_Object *,
                                   unsigned long);
#endif
