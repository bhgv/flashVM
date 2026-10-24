/* Fail-closed thread-local register binding for the experimental interpreter build. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_context.h"

/* Never fall back to a shared global R when a native thread was not initialized. */
REG *dvm_native_registers(void)
{
    REG *registers = (REG *)dvm_native_context_registers(dvm_native_context_current());
    if (!registers || dvm_native_context_register_size(dvm_native_context_current()) != sizeof(REG))
        abort();
    return registers;
}
