/* Copyright (c) 2026 bhgv
 *
 * EN: Thread-local error buffers for native collector callbacks.
 * RU: Буферы ошибок в TLS для нативных проходов сборщика.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "../collector_passes.h"


/* EN: Set up the bound collector's error buffers on first use.
 * RU: Инициализировать буферы ошибок текущего сборщика при первом обращении. */
void dvm_gc_collector_environment(void)
{
    if (!up->env)
    {
        up->env = &up->defenv;
        up->env->errstr = up->env->errbuf1;
        up->env->syserrstr = up->env->errbuf0;
    }
}
