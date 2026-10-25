/* OS: all native mixers. Detach control ownership without requiring a functioning device. */
#include "channel.hpp" /* Import native registry and lifecycle rules. */


/* Hosts explicitly stop first when status matters; detach never calls host code. */
FVM_Result fvm_audio_stop_binding_close(FVM_AudioStopBinding *binding) /* Null is idempotent partial-setup cleanup. */
{                                                                      /* Begin unconditional detach outside callbacks and teardown. */
    if (!binding)                                                      /* Condition for this action: No context or userdata was acquired. */
    {                                                                  /* Begin conditional branch: Condition for this action: No context or userdata was acquired. */
        return FVM_OK;                                                 /* No context or userdata was acquired. */
    } /* End conditional branch: Condition for this action: No context or userdata was acquired. */
    if (!fvm_audio_context_valid(binding->context)) /* Condition for this action: Keep callback iteration stable. */
    {                                               /* Begin conditional branch: Condition for this action: Keep callback iteration stable. */
        return FVM_INVALID_ARGUMENT;                /* Keep callback iteration stable. */
    } /* End conditional branch: Condition for this action: Keep callback iteration stable. */
    fvm_detail::vector<FVM_AudioStopBinding *> &bindings = binding->context->audio_stop_bindings; /* Borrow the native registry. */
    for (size_t i = 0; i < bindings.size(); ++i)                                                  /* Find the exact context-owned handle. */
    {                                                                                             /* Examine one stable registration slot. */
        if (bindings[i] == binding)                                                               /* Never detach by shared userdata identity. */
        {                                                                                         /* Release only this registration. */
            bindings.erase(bindings.begin() + i);                                                 /* No callback or allocation is required. */
            delete binding;                                                                       /* No callback or allocation is required. */
            return FVM_OK;                                                                        /* No callback or allocation is required. */
        } /* Finish exact-match detach. */
    } /* Finish registry lookup. */
    return FVM_INVALID_ARGUMENT; /* Do not free an inconsistent private handle. */
} /* End detached host ownership. */
