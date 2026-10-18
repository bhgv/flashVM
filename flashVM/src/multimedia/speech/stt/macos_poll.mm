/* OS: macOS 10.15+. Query/copy actual Speech-framework recognition text. */
#include "macos.hpp" /* Import the private synchronized transcript owner. */
/* Poll without opening, restarting or waiting for the microphone. */
FVM_Result fvm_macos_stt_poll(void *opaque, char *text, size_t capacity, size_t *required, /* Preserve pending text across size queries. */
                              int *final) /* Preserve pending text across size queries. */
{ /* Begin native text polling. */
    @autoreleasepool /* Bound temporary native string conversions. */
    { /* Bound temporary native string conversions. */
        FVMAppleStt *state = static_cast<FVMAppleStt *>(opaque); /* Borrow the owned recognition service. */
        return [state copyText:text /* Retrieve an actual stable recognized transcript. */
                      capacity:capacity /* Retrieve an actual stable recognized transcript. */
                      required:required /* Retrieve an actual stable recognized transcript. */
                         final:final]; /* Retrieve an actual stable recognized transcript. */
    } /* Finish temporary native ownership scope. */
} /* Finish native text polling. */
