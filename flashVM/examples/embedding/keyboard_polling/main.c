/* Demonstrate context-local keyboard publication and Key polling without an OS device. */
#include "flashvm/keyboard.h" /* Declare the C89 snapshot ABI and virtual-key constants. */
#include "flashvm/standard.h" /* Install the ordinary standard object library first. */
#include "flashvm/value.h"    /* Read Key and call its native polling methods. */
#include <stdio.h>            /* Print results and ordinary error messages. */
#include <string.h>           /* Initialize the complete native snapshot before publication. */


/* Display one polling result; the context and Key object remain borrowed. */
static FVM_Result show(FVM_Context *context, FVM_Object *key, const char *method, int argument) /* Borrow the namespace only during this call. */
{                                                                                               /* Start the synchronous method call helper. */
    FVM_Value code, result;                                                                     /* Keep scalar arguments/results on this native stack. */
    FVM_Result status;                                                                          /* Propagate errors to the caller instead of asserting. */
    fvm_value_number(&code, argument);                                                          /* Convert the selected virtual key into a VM number. */
    status = fvm_object_call(context, key, method, argument < 0 ? 0 : 1, &code, &result);       /* Poll without dispatching an event. */
    if (status != FVM_OK)                                                                       /* Preserve failures such as an unavailable method. */
    {                                                                                           /* Begin conditional branch: Preserve failures such as an unavailable method. */
        return status;                                                                          /* Let main perform cleanup and report the status. */
    } /* End conditional branch: Preserve failures such as an unavailable method. */
    if (result.type == FVM_VALUE_BOOLEAN)                                   /* Held/toggled queries return a Boolean. */
    {                                                                       /* Begin conditional branch: Held/toggled queries return a Boolean. */
        printf("Key.%s = %s\n", method, result.boolean ? "true" : "false"); /* Show the polled state. */
    } /* End conditional branch: Held/toggled queries return a Boolean. */
    else if (result.type == FVM_VALUE_NUMBER)             /* Latest code queries return a Number. */
    {                                                     /* Begin conditional branch: Latest code queries return a Number. */
        printf("Key.%s = %.0f\n", method, result.number); /* Print the accepted host code. */
    } /* End conditional branch: Latest code queries return a Number. */
    return FVM_OK; /* The result owns no independent memory. */
} /* End the polling helper. */


/* Publish a simulated simultaneous press, then demonstrate focus-loss reset. */
int main(void)                                          /* Run the example on a single owning thread. */
{                                                       /* Start the standalone C89 host. */
    FVM_Config config;                                  /* Own configuration storage until runtime creation. */
    FVM_Runtime *runtime = 0;                           /* Own the runtime until final cleanup. */
    FVM_Context *context = 0;                           /* Own one isolated VM input state. */
    FVM_KeyboardState state;                            /* Own a copied native snapshot with no managed references. */
    FVM_Value key;                                      /* Borrow the globally rooted Key namespace while the context is alive. */
    FVM_Result status = fvm_config_init(&config);       /* Begin with documented default configuration. */
    if (status == FVM_OK)                               /* Create no later resource after an earlier failure. */
    {                                                   /* Begin conditional branch: Create no later resource after an earlier failure. */
        status = fvm_runtime_create(&config, &runtime); /* Acquire the runtime. */
    } /* End conditional branch: Create no later resource after an earlier failure. */
    if (status == FVM_OK)                               /* The runtime now owns allocation policy. */
    {                                                   /* Begin conditional branch: The runtime now owns allocation policy. */
        status = fvm_context_create(runtime, &context); /* Acquire the context. */
    } /* End conditional branch: The runtime now owns allocation policy. */
    if (status == FVM_OK)                       /* Install the normal standard namespace before its explicit extension. */
    {                                           /* Begin conditional branch: Install the normal standard namespace before its explicit extension. */
        status = fvm_standard_install(context); /* Preserve conflicts according to its API contract. */
    } /* End conditional branch: Install the normal standard namespace before its explicit extension. */
    if (status == FVM_OK)                                /* Key installation opens no keyboard or OS handle. */
    {                                                    /* Begin conditional branch: Key installation opens no keyboard or OS handle. */
        status = fvm_standard_keyboard_install(context); /* Publish the supported polling namespace. */
    } /* End conditional branch: Key installation opens no keyboard or OS handle. */
    if (status == FVM_OK)                              /* Read the installed object while its global keeps it reachable. */
    {                                                  /* Begin conditional branch: Read the installed object while its global keeps it reachable. */
        status = fvm_get_global(context, "Key", &key); /* Obtain the borrowed namespace object. */
    } /* End conditional branch: Read the installed object while its global keeps it reachable. */
    memset(&state, 0, sizeof(state));                     /* Initialize every held-key slot and toggle. */
    state.struct_size = sizeof(state);                    /* Select the exact current ABI layout. */
    state.down[FVM_KEY_LEFT] = 1;                         /* Simulate holding Left. */
    state.down[FVM_KEY_RIGHT] = 1;                        /* Simulate holding Right at the same time. */
    state.last_code = FVM_KEY_RIGHT;                      /* Record the latest key-down independently of held slots. */
    if (status == FVM_OK)                                 /* Publish all fields together on the owning VM thread. */
    {                                                     /* Begin conditional branch: Publish all fields together on the owning VM thread. */
        status = fvm_input_set_keyboard(context, &state); /* Copy and validate the native snapshot. */
    } /* End conditional branch: Publish all fields together on the owning VM thread. */
    if (status == FVM_OK)                                 /* Read back host state without allocating or invoking listeners. */
    {                                                     /* Begin conditional branch: Read back host state without allocating or invoking listeners. */
        status = fvm_input_get_keyboard(context, &state); /* Caller storage remains independent. */
    } /* End conditional branch: Read back host state without allocating or invoking listeners. */
    if (status == FVM_OK)                                           /* The first query should print true. */
    {                                                               /* Begin conditional branch: The first query should print true. */
        status = show(context, key.object, "isDown", FVM_KEY_LEFT); /* Observe the first held key. */
    } /* End conditional branch: The first query should print true. */
    if (status == FVM_OK)                                            /* The second query should also print true. */
    {                                                                /* Begin conditional branch: The second query should also print true. */
        status = show(context, key.object, "isDown", FVM_KEY_RIGHT); /* Observe simultaneous input. */
    } /* End conditional branch: The second query should also print true. */
    if (status == FVM_OK)                           /* Reset as a host should when its window loses focus. */
    {                                               /* Begin conditional branch: Reset as a host should when its window loses focus. */
        status = fvm_input_reset_keyboard(context); /* Clear held slots, codes and toggles without fake events. */
    } /* End conditional branch: Reset as a host should when its window loses focus. */
    if (status == FVM_OK)                                           /* After reset the first query should print false. */
    {                                                               /* Begin conditional branch: After reset the first query should print false. */
        status = show(context, key.object, "isDown", FVM_KEY_LEFT); /* Demonstrate absence of a stuck key. */
    } /* End conditional branch: After reset the first query should print false. */
    if (status == FVM_OK)                                  /* After reset the latest key code should print zero. */
    {                                                      /* Begin conditional branch: After reset the latest key code should print zero. */
        status = show(context, key.object, "getCode", -1); /* Use the helper's no-argument form. */
    } /* End conditional branch: After reset the latest key code should print zero. */
    if (status != FVM_OK)                                              /* Preserve useful error information for the user. */
    {                                                                  /* Begin conditional branch: Preserve useful error information for the user. */
        fprintf(stderr, "keyboard polling failed: %d\n", (int)status); /* Report an ordinary API failure. */
    } /* End conditional branch: Preserve useful error information for the user. */
    if (context)                      /* Destroy only a successfully acquired context. */
    {                                 /* Begin conditional branch: Destroy only a successfully acquired context. */
        fvm_context_destroy(context); /* Release Key and all other context-owned objects. */
    } /* End conditional branch: Destroy only a successfully acquired context. */
    if (runtime)                      /* Release the runtime after its context. */
    {                                 /* Begin conditional branch: Release the runtime after its context. */
        fvm_runtime_destroy(runtime); /* Finish native ownership cleanup. */
    } /* End conditional branch: Release the runtime after its context. */
    return status == FVM_OK ? 0 : 1; /* Report success or failure to the shell. */
} /* End the standalone example. */
