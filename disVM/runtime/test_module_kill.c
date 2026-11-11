/* Copyright (c) 2026 bhgv
 * EN: Observe accepted real killwait before a host releases the provider's input.
 * RU: Наблюдать настоящий принятый killwait до выдачи host-входа провайдеру.
 */
#include "lib9.h"
#include "interp.h"
#include "thread_port.h"
#include <stdio.h>

int __real_dvm_runtime_prepare_killwait(Prog *, int);


/* EN: Forward authorized caller/pid unchanged and announce only real acceptance.
 * CONTROL models parent descheduling before its request; no reply is fabricated.
 * Return the actual prepared-call result without touching registers or ownership.
 * RU: Передать caller/pid без изменений и сообщить только реальное принятие.
 * CONTROL моделирует вытеснение родителя до запроса; ответ не создаётся.
 * Вернуть настоящий результат, не меняя регистры или владение. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    if (CONTROL)
    {
        fvm_port_thread_sleep_us(250000);
    }
    int result = __real_dvm_runtime_prepare_killwait(caller, pid);
    if (!result)
    {
        fprintf(stderr, "PROVIDER_KILL_ACCEPTED pid=%d\n", pid);
    }
    return result;
}
