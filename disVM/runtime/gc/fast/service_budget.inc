/* Copyright (c) 2026 bhgv
 * [GC:] EN: Independent visit budgets for the three shared Fast services.
 * RU: Независимые квоты обхода для трёх параллельных служб Fast.
 * Private fragment of retirement_pass.c; only the sole Fast thread schedules it.
 */


/* [GC:] EN: Service classes own distinct queues and independent reader cutoffs.
 * RU: Классы служб владеют разными очередями и независимыми эпохами читателей. */
enum
{
    FastTickets,     /* Managed tickets retaining their original roots. */
    FastReserves,    /* Emergency strings with their own retirement epochs. */
    FastRC,          /* RC owners, leaves, graph retries and frame packets. */
    FastServiceCount /* Number of independently scheduled services. */
};

static unsigned int fast_service_turn; /* Sole Fast consumer rotates remainder allowances. */


/* [GC:] EN: Split budget across nonempty service metadata into caller-owned
 * shares. Rotate remainder units even when budget is one; a reader retry may
 * consume only its own allowance. Ready samples are hints: a later publication
 * notifies Fast and is seen on the next pass. No ownership/admission changes,
 * managed payload reads or allocation. Caller is the sole Fast consumer.
 * RU: Разделить budget по непустым метаданным служб в личный shares. Остаток
 * выдаётся по кругу даже при бюджете один; ожидание читателя расходует только
 * свою квоту. Готовность — подсказка: новая публикация будит Fast для следующего
 * прохода. Без смены владения/допуска, чтения данных кучи и выделения памяти.
 * Вызывает единственный потребитель Fast. */
static void fast_service_budget(size_t budget, size_t shares[FastServiceCount])
{
    unsigned int ready[FastServiceCount] = {
        dvm_managed_owner_pending() != 0, dvm_runtime_exception_retirement_ready() != 0, dvm_runtime_rc_ready() != 0};
    unsigned int count = 0;
    for (unsigned int index = 0; index < FastServiceCount; index++)
    {
        count += ready[index];
        shares[index] = 0;
    }
    if (!count || !budget)
    {
        return;
    }
    size_t remainder = budget % count;
    unsigned int next = (fast_service_turn + 1) % FastServiceCount;
    for (unsigned int offset = 0; offset < FastServiceCount; offset++)
    {
        unsigned int index = (fast_service_turn + offset) % FastServiceCount;
        if (ready[index])
        {
            shares[index] = budget / count;
            if (remainder)
            {
                shares[index]++;
                remainder--;
                next = (index + 1) % FastServiceCount;
            }
        }
    }
    fast_service_turn = next;
}
