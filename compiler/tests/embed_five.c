/* One C99 process links all five parser namespaces and recovers after alternating errors. */
#include "as1/frontend/frontend.h"
#include "as2/frontend/frontend.h"
#include "as3/frontend/frontend.h"
#include "limbo/frontend/frontend.h"
#include "firsyn.h"
#include <string.h>

/* Input/diagnostics stay caller-owned; no parser process or console adapter is used. */
int main(void)
{
    const char *as = "var value = 1\n";
    const char *limbo = "f(){return;}";
    const char *fir = "FIR 1.0\nmodule @m\nfunc @f(%x:i32)->i32 {^entry:return %x}\n";
    const char *bad_fir = "FIR 1.0\nmodule @m\nfunc @f()->void {^entry:%v:i32=}\n";
    As1Diagnostic a1;
    As2Diagnostic a2;
    As3Diagnostic a3;
    LimboDiagnostic lb;
    FirSyntaxDiagnostic ir;
    unsigned int i;
    for (i = 0; i < 32; ++i)
    {
        if (as1_check("@", 1, &a1) || as2_check("@", 1, &a2) || as3_check("@", 1, &a3) ||
            limbo_check("@", 1, &lb) || firsyn_check(bad_fir, strlen(bad_fir), &ir))
            return 1;
        if (!as1_check(as, strlen(as), &a1) || !as2_check(as, strlen(as), &a2) ||
            !as3_check(as, strlen(as), &a3) || !limbo_check(limbo, strlen(limbo), &lb) ||
            !firsyn_check(fir, strlen(fir), &ir))
            return 2;
    }
    return 0;
}
