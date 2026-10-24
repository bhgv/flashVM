/* Host-only diagnostics for the selected Inferno build utility. */
#include "lib9.h"
#include <stdio.h>

/* Return a native caller address for optional lib9 allocation diagnostics. */
uintptr getcallerpc(void *unused)
{
    USED(unused);
#if defined(__GNUC__) || defined(__clang__)
    return (uintptr)__builtin_return_address(0);
#else
    return 0;
#endif
}

/* Report a failed lib9 invariant before terminating this build-tool process. */
void _assert(char *message)
{
    fprintf(stderr, "mk host assertion: %s\n", message);
    abort();
}
