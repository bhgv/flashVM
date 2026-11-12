/* Copyright (c) 2026 bhgv
 * [GC:] EN: Strict native RC rejects work requiring legacy collector exclusion.
 * RU: Строгий нативный RC отвергает работу, требующую прежнего эксклюзива сборщика.
 */


/* [GC:] EN: Select immutable strict policy before producers start.
 * RU: До запуска производителей выбрать неизменную строгую политику. */
void dvm_runtime_rc_set_concurrent(void)
{
    concurrent_only = 1;
}


/* [GC:] EN: A declined transfer must not reach destroy's legacy callback in
 * a live strict session. admitted_role distinguishes closed queues from invalid
 * phase/world. Reject before touching the value, its root or reference count.
 * The reference and fully joined post-session cleanup keep their old return path.
 * RU: При живом строгом сеансе отказ передачи не должен вести к старому callback
 * destroy. admitted_role различает закрытую очередь и неверную фазу/мир.
 * Отказать до доступа к объекту, корню и счётчику. Эталон и очистка уже завершённого
 * сеанса сохраняют прежний возврат. */
static void concurrent_retirement_refused(int admitted_role)
{
    if (concurrent_only && world)
    {
        fprintf(stderr,
                "NATIVE_CONCURRENCY_ERROR component=RC reason=%s\n",
                admitted_role ? "retirement-admission-closed" : "retirement-not-admitted");
        fflush(stderr);
        _Exit(70);
    }
}


/* [GC:] EN: Inspect only synchronized native queue metadata; runtime storage
 * survives all worker joins. Forbidden work terminates with explicit reason;
 * queue owners and roots remain intact until process exit.
 * RU: Читать только синхронизированные native-метаданные очередей; память живёт
 * до join всех служб. Запрещённая работа завершает процесс с явной причиной;
 * владельцы и корни остаются целыми до выхода процесса. */
void dvm_runtime_rc_check_concurrent(void)
{
    if (!concurrent_only)
    {
        return;
    }
    const char *reason = dvm_retirement_peek(difficult)                              ? "difficult-finalization"
                         : dvm_retirement_peek(cycle_refused)                        ? "refused-cycle-graph"
                         : __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE) ? "duke-pin-fallback"
                                                                                     : NULL;
    if (reason)
    {
        fprintf(stderr, "NATIVE_CONCURRENCY_ERROR component=RC reason=%s\n", reason);
        fflush(stderr);
        _Exit(70);
    }
}
