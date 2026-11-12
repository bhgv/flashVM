/* Copyright (c) 2026 bhgv
 * [GC:] EN: Guard every runtime entry into the retained heap tracer.
 * RU: Защитить каждый вход runtime в прежний общий обход кучи.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../../shared_arithmetic.h"
#include <stdio.h>
#include <stdlib.h>


/* [GC:] EN: Retained implementation is compiled under a private symbol.
 * roots is borrowed under the single-thread reference collector's admission.
 * RU: Прежняя реализация собирается под закрытым именем. roots заимствован
 * под допуском сборщика однопоточного эталона. */
extern void dvm_retained_rungc(Prog *roots);


/* [GC:] EN: Borrow roots for the reference tracer only. Native-spawn execution
 * terminates before any trace mutation with diagnostic status 70.
 * RU: Передать roots лишь эталонному трассировщику. В native-spawn завершиться
 * с диагностикой и кодом 70 до любого изменения трассировки. */
void rungc(Prog *roots)
{
    if (dvm_runtime_guest_unbounded)
    {
        fputs("NATIVE_CONCURRENCY_ERROR component=Cycles reason=retained-trace-forbidden\n", stderr);
        fflush(stderr);
        _Exit(70);
    }
    dvm_retained_rungc(roots);
}
