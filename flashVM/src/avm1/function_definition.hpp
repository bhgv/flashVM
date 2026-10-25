/* FlashVM: shared function-record materialization for interpreter and native definitions. */
#ifndef FVM_AVM1_FUNCTION_DEFINITION_HPP
#define FVM_AVM1_FUNCTION_DEFINITION_HPP /* Private definition materialization guard. */
#include "function_record.hpp"
#include "../core/runtime.hpp"


/* Copy validated record metadata/body and the currently selected raw constant pool. */
FVM_Result fvm_avm1_prepare_function(const unsigned char *,
                                     const FVM_FunctionRecord &,
                                     bool,
                                     const fvm_detail::vector<fvm_detail::string> *,
                                     FVM_ScriptFunction &);
#endif
