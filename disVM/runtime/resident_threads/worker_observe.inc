/* Copyright (c) 2026 bhgv
 * EN: Bounded copies of native resident metadata; no managed heap access.
 * RU: Ограниченные копии native resident metadata; без managed heap access.
 */


/* EN: Borrow output page/count/more during a synchronous metadata copy. after
 * selects stable lifetime IDs; capacity is 1..256. Caller keeps the service and
 * disjoint native output storage alive through return. Return zero unchanged on
 * invalid arguments/unavailable service. Never inspect Prog, REG or owner counters.
 * RU: Borrow output page/count/more на synchronous metadata copy. after выбирает
 * stable lifetime IDs; capacity 1..256. Caller удерживает service и disjoint native
 * output storage до return. Ноль без изменений при invalid/unavailable. Не читать
 * Prog, REG либо owner counters. */
int dvm_resident_observe(unsigned long after, DVM_ResidentCard *cards,
    unsigned int capacity, unsigned int *count, int *more)
{
    ManagedWorker *worker;
    unsigned int used = 0;
    int truncated = 0;
    if (!service.sync || !cards || !count || !more || !capacity || capacity > 256)
    {
        return 0;
    }
    fvm_port_sync_lock(service.sync);
    for (worker = service.helpers; worker; worker = worker->next)
    {
        unsigned int index;
        unsigned int move;
        if (worker->id <= after)
        {
            continue;
        }
        index = 0;
        while (index < used && cards[index].id < worker->id)
        {
            index++;
        }
        if (used == capacity)
        {
            truncated = 1;
            if (index == capacity)
            {
                continue;
            }
        }
        else
        {
            used++;
        }
        for (move = used - 1; move > index; move--)
        {
            cards[move] = cards[move - 1];
        }
        cards[index].id = worker->id;
        cards[index].pid = worker->guest_pid;
        cards[index].done = worker->done;
    }
    fvm_port_sync_unlock(service.sync);
    *count = used;
    *more = truncated;
    return 1;
}
