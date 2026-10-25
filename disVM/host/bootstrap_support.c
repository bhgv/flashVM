/* Small POSIX support for the legacy native bootstrap compiler only.
 * No scheduler, graphics, security service or deployed VM is implemented here. */
#include "lib9.h"
#include <fenv.h>
#include <stdio.h>


/* The compiler requests return addresses for optional allocation diagnostics. */
ulong getcallerpc(void *unused)
{
    USED(unused);
#if defined(__GNUC__) || defined(__clang__)
    return (ulong)__builtin_return_address(0);
#else
    return 0;
#endif
}


/* Abort a failed upstream invariant with a visible diagnostic. */
void _assert(char *message)
{
    fprintf(stderr, "bootstrap assertion: %s\n", message);
    abort();
}


/* Translate the upstream abstract rounding bits to the host C floating model.
 * These functions deliberately do not enable hardware exception traps. */
void setfcr(ulong control)
{
    int mode = FE_TONEAREST;
    switch (control & FPRMASK)
    {
    case FPRZ:
        mode = FE_TOWARDZERO;
        break;
    case FPRPINF:
        mode = FE_UPWARD;
        break;
    case FPRNINF:
        mode = FE_DOWNWARD;
        break;
    }
    fesetround(mode);
}


/* Report the host rounding direction using the upstream's abstract constants. */
ulong getfcr(void)
{
    switch (fegetround())
    {
    case FE_TOWARDZERO:
        return FPRZ;
    case FE_UPWARD:
        return FPRPINF;
    case FE_DOWNWARD:
        return FPRNINF;
    default:
        return FPRNR;
    }
}


/* Return exception status without changing the caller's floating environment. */
ulong getfsr(void)
{
    int status = fetestexcept(FE_ALL_EXCEPT);
    ulong result = 0;
    if (status & FE_INEXACT)
    {
        result |= FPAINEX;
    }
    if (status & FE_OVERFLOW)
    {
        result |= FPAOVFL;
    }
    if (status & FE_UNDERFLOW)
    {
        result |= FPAUNFL;
    }
    if (status & FE_DIVBYZERO)
    {
        result |= FPAZDIV;
    }
    if (status & FE_INVALID)
    {
        result |= FPAINVAL;
    }
    return result;
}


/* Clearing exception status is the bootstrap compiler's required operation. */
void setfsr(ulong status)
{
    USED(status);
    feclearexcept(FE_ALL_EXCEPT);
}
