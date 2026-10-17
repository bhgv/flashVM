/* FlashVM: shared script activation setup and private interpreter fallback bridge. */
#ifndef FVM_SCRIPT_CALL_HPP
#define FVM_SCRIPT_CALL_HPP /* Private activation guard. */
#include "../execution_state.hpp"
/* Prepare lexical bindings, arguments, super and registers, then execute the selected body. */
FVM_Result fvm_avm1_call_script(FVM_Context *, const fvm_detail::string &,
                                const FVM_ScriptFunction &, unsigned int, const FVM_Value *,
                                FVM_Object *, FVM_Value *, FVM_Object *closure = 0,
                                FVM_Object *callee = 0, FVM_Object *base_clip = 0);
/* Enter bytecode only after the shared activation builder chooses interpreter fallback. */
FVM_Result fvm_avm1_interpret_function(FVM_Context *, const FVM_ScriptFunction &, FVM_Value *,
                                       fvm_detail::map<fvm_detail::string, FVM_Value> *,
                                       FVM_Object *, Execution *, const FVM_Value *);
#endif
