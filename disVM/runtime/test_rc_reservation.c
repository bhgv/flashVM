/* Copyright (c) 2026 bhgv
 * EN: Audit mandatory GC publication under injected failure at the real root-allocation boundary.
 * RU: Проверка обязательной передачи в GC при внедрённом отказе на настоящей границе создания корня.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_rc.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>

static _Thread_local int active; /* Only this producer's defer call, not collector work. */
static _Thread_local int injected; /* All root allocations fail in this defer scope. */
static _Thread_local Heap *source; /* Borrowed live owner until defer returns or fatal exit. */
static int selected; /* Atomic one-shot producer fault selection. */

int __real_dvm_runtime_rc_defer(void *);
int __real_dvm_runtime_root_add(Heap *);
void __real_abort(void);


/* EN: Stop with a precise semantic failure; borrowed message contains no VM data.
 * RU: Завершить с точной семантической ошибкой; message не содержит данных VM. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL mandatory GC publication: %s\n", message);
        _Exit(10);
    }
}


/* EN: Refuse actual root storage consistently within one selected producer call.
 * heap remains its strong source owner. Optional lanes may roll back, but the
 * mandatory publication attempt must then terminate on the same allocation OOM.
 * RU: Последовательно отказывать корням одного выбранного вызова производителя.
 * heap остаётся сильной исходной ссылкой. Необязательный путь может откатиться,
 * но обязательная передача должна завершить VM при том же отказе выделения. */
int __wrap_dvm_runtime_root_add(Heap *heap)
{
    if (CONTROL && active && (injected || !__atomic_exchange_n(&selected, 1, __ATOMIC_ACQ_REL)))
    {
        require(heap == source && dvm_refcount_load_u(&heap->ref) > 0,
            "publication still owns the live source before failed root allocation");
        if (!injected)
        {
            fprintf(stderr, "INJECT mandatory GC root allocation failure\n");
        }
        injected = 1;
        return 0;
    }
    return __real_dvm_runtime_root_add(heap);
}


/* EN: Forward a real producer release; an injected OOM must not return for inline free.
 * value is borrowed until the real defer transfers it. Never read it after success.
 * RU: Передать настоящее освобождение; внедрённый OOM не должен вернуться в inline free.
 * value заимствован до передачи владения; не читать его после успешного defer. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    active = 1;
    injected = 0;
    source = D2H(value);
    int result = __real_dvm_runtime_rc_defer(value);
    active = 0;
    require(!injected, "root OOM returned to the mutator instead of terminating publication");
    return result;
}


/* EN: Turn only the expected fatal OOM abort into status 73, without a core dump.
 * Require its production diagnostic in the Python driver. Other aborts stay real.
 * RU: Заменить только ожидаемый fatal OOM abort кодом 73 без core dump.
 * Python проверяет настоящую диагностику. Остальные abort остаются настоящими. */
void __wrap_abort(void)
{
    if (active && injected)
    {
        fprintf(stderr, "PASS mandatory GC root OOM terminated publication\n");
        _Exit(73);
    }
    __real_abort();
    _Exit(74);
}
