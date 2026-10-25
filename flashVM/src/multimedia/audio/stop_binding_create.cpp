/* OS: all native mixers. Register an always-called context StopSounds control. */
#include "channel.hpp" /* Import native ownership, lifecycle checks and callback guards. */
#include <new>         /* Contain native allocation failure before publishing ownership. */


/* This registration stops prepared voices, not just channels with successful PCM writes. */
FVM_Result fvm_audio_stop_binding_create(FVM_Context *context, FVM_AudioStopCallback callback, /* Borrow native callback identity. */
                                         void *userdata, FVM_AudioStopBinding **out)           /* Keep host userdata ownership unchanged. */
{                                                                                              /* Begin transactional native registration. */
    if (!out)                                                                                  /* Condition for this action: Require storage for deterministic publication. */
    {                                                                                          /* Begin conditional branch: Condition for this action: Require storage for deterministic publication. */
        return FVM_INVALID_ARGUMENT;                                                           /* Require storage for deterministic publication. */
    } /* End conditional branch: Condition for this action: Require storage for deterministic publication. */
    *out = 0;                                           /* Never expose partial construction. */
    if (!fvm_audio_context_valid(context) || !callback) /* Condition for this action: Reject callback mutation and invalid phases. */
    {                                                   /* Begin conditional branch: Condition for this action: Reject callback mutation and invalid phases. */
        return FVM_INVALID_ARGUMENT;                    /* Reject callback mutation and invalid phases. */
    } /* End conditional branch: Condition for this action: Reject callback mutation and invalid phases. */
    if (context->audio_stop_bindings.size() >= FVM_AUDIO_STOP_MAX_BINDINGS) /* Condition for this action: Bound native controls per context. */
    {                                                                       /* Begin conditional branch: Condition for this action: Bound native controls per context. */
        return FVM_OUT_OF_MEMORY;                                           /* Bound native controls per context. */
    } /* End conditional branch: Condition for this action: Bound native controls per context. */
    FVM_AudioStopBinding *binding = 0;                   /* Own unpublished allocation until vector insertion succeeds. */
    try                                                  /* Keep node and registry allocation failures atomic. */
    {                                                    /* Prepare one independent host registration. */
        binding = new FVM_AudioStopBinding;              /* Allocate native state outside GC. */
        binding->context = context;                      /* Copy only borrowed identities. */
        binding->stop = callback;                        /* Copy only borrowed identities. */
        binding->userdata = userdata;                    /* Copy only borrowed identities. */
        context->audio_stop_bindings.push_back(binding); /* Commit after the last fallible allocation. */
        *out = binding;                                  /* Publish the context-owned handle without entering host code. */
        return FVM_OK;                                   /* Publish the context-owned handle without entering host code. */
    } /* End successful publication. */
    catch (const std::bad_alloc &) /* Leave no registry entry or caller handle. */
    {                              /* Begin exception boundary: Leave no registry entry or caller handle. */
        delete binding;            /* Leave no registry entry or caller handle. */
        return FVM_OUT_OF_MEMORY;  /* Leave no registry entry or caller handle. */
    } /* Leave no registry entry or caller handle. */
    catch (...)           /* Preserve the C ABI on unexpected native failure. */
    {                     /* Begin exception boundary: Preserve the C ABI on unexpected native failure. */
        delete binding;   /* Preserve the C ABI on unexpected native failure. */
        return FVM_ERROR; /* Preserve the C ABI on unexpected native failure. */
    } /* Preserve the C ABI on unexpected native failure. */
} /* End context stop registration. */
