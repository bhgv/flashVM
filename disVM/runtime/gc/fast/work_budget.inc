/* Copyright (c) 2026 bhgv
 * [GC:] EN: Fair visit allowances for independent Fast ownership queues.
 * RU: Справедливые квоты обхода независимых очередей владельцев Fast.
 */


/* [GC:] EN: Independent Fast work classes; only the sole Fast consumer schedules them.
 * RU: Независимые классы работы Fast; их планирует единственный потребитель Fast. */
enum
{
    FastOwners,    /* Mandatory strong owners. */
    FastRetries,   /* Leaf/graph claims waiting for retry. */
    FastLeaves,    /* Newly published leaf/graph owners. */
    FastFrames,    /* Published frame children. */
    FastQueueCount /* Number of independent allowances. */
};

static unsigned int fast_queue_turn; /* Sole Fast consumer's next small-budget preference. */


/* [GC:] EN: Divide budget among nonempty native queues into caller-owned shares.
 * Borrow queue metadata only; caller holds sole Fast shared admission. Rotate
 * remainder units so even budget one reaches each nonempty class. Empty classes
 * receive zero. No roots, counts, cursors or managed fields are changed here.
 * RU: Разделить budget между непустыми native-очередями в личный shares.
 * Вызывающий — единственный shared Fast; читаются лишь метаданные. Остаток
 * распределяется по кругу, поэтому бюджет один достигает каждого класса.
 * Пустым классам — ноль. Корни, ссылки, курсоры и поля объектов не меняются. */
static void fast_work_budget(size_t budget, size_t shares[FastQueueCount])
{
    DVM_RetirementQueue *queues[FastQueueCount] = {retirement, record_retries, leaves, frames};
    unsigned int ready[FastQueueCount];
    unsigned int count = 0;
    for (unsigned int index = 0; index < FastQueueCount; index++)
    {
        ready[index] = dvm_retirement_peek(queues[index]) != NULL;
        count += ready[index];
        shares[index] = 0;
    }
    if (!count || !budget)
    {
        return;
    }
    size_t remainder = budget % count;
    unsigned int next_turn = (fast_queue_turn + 1) % FastQueueCount;
    for (unsigned int offset = 0; offset < FastQueueCount; offset++)
    {
        unsigned int index = (fast_queue_turn + offset) % FastQueueCount;
        if (ready[index])
        {
            shares[index] = budget / count;
            if (remainder)
            {
                shares[index]++;
                remainder--;
                next_turn = (index + 1) % FastQueueCount;
            }
        }
    }
    fast_queue_turn = next_turn;
}
