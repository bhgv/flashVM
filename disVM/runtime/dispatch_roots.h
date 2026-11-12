/* Copyright (c) 2026 bhgv
 * [GC:] EN: Owner-only register publication around dedicated channel waits.
 * RU: Публикация регистров владельцем вокруг ожиданий выделенных каналов.
 */
#ifndef DVM_DISPATCH_ROOTS_H
#define DVM_DISPATCH_ROOTS_H
struct Prog;


/* [GC:] EN: Borrow the current dispatch's process and live TLS registers.
 * Publish only while ready in dedicated mode; a parked channel's immutable
 * image was committed before its waiter became visible. Acquire readiness
 * permits publication after the completion's foreign readers finish. Reference
 * mode keeps its ordinary publication. No register reload, allocation or wait.
 * RU: Заимствовать process своего назначения и живые TLS-регистры. В dedicated
 * публиковать лишь при готовности; снимок канального ожидания уже опубликован
 * до видимости waiter. Acquire готовности разрешает запись после завершения
 * чтения партнёром. Эталон сохраняет обычную публикацию. Без восстановления
 * регистров, выделения памяти и ожидания. */
void dvm_runtime_publish_dispatch_roots(struct Prog *process);
#endif
