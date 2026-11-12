/* Copyright (c) 2026 bhgv
 * [GC:] EN: Fresh outside-owner classification over complete guarded captures.
 * RU: Повторная классификация внешних владельцев полного защищённого снимка.
 * Private fragment of graph_capture.c; no independent ownership or admission.
 */


/* [GC:] EN: Classify complete group, borrowing its live allocations and fields.
 * shared permits an unclosed seed to be treated as live. Reinitialize dense work
 * arrays on every call; growing captures recompute derived roots and live marks.
 * Return zero for invalid managed/native counts; discovery index is overwritten.
 * RU: Классифицировать полный group, заимствуя живые узлы и поля. shared позволяет
 * считать незамкнутый seed живым. Каждый вызов сбрасывает рабочие массивы;
 * растущий снимок заново проверяет корни и вычисленные live-метки. Ноль — неверные
 * managed/native-счётчики; индекс обхода перезаписывается. */
static int classify_record_group(RecordGroup *group, int shared)
{
    group->live_count = 0;
    if (group->growing)
    {
        for (unsigned int index = 0; index < group->nodes_used; index++)
        {
            group->live[index] &= RECORD_INTRINSIC_LIVE;
            unsigned long expected = index == 0 ? 1 : index <= group->pinned_nodes;
            group->live[index] |= dvm_runtime_root_count(D2H(group->nodes[index])) != expected;
        }
    }
    if (!dvm_rc_classify_modules(group))
    {
        return 0;
    }
    if (!group->incoming[0])
    {
        if (!shared)
        {
            return 0;
        }
        group->live[0] = 1;
    }
    /* Discovery is complete; reuse its index as two dense work arrays. */
    unsigned int *ready = group->index;
    unsigned int *starts = ready + group->nodes_used;
    unsigned int edge = 0;
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        Heap *heap = D2H(group->nodes[index]);
        unsigned long expected = group->incoming[index] + (index == 0 || index <= group->pinned_nodes);
        unsigned long actual = dvm_refcount_load_u(&heap->ref);
        if (actual < expected)
        {
            return 0;
        }
        group->live[index] |= actual > expected;
        if (group->live[index])
        {
            ready[group->live_count++] = index;
        }
        starts[index] = edge;
        while (edge < group->edges_used && group->sources[edge] == index)
        {
            edge++;
        }
    }
    starts[group->nodes_used] = edge;

    /* Edges are captured in source order. Visit each live node's outgoing
     * range once, even when reachability runs against discovery order. */
    for (unsigned int head = 0; head < group->live_count && !group->live[0]; head++)
    {
        unsigned int source = ready[head];
        dvm_rc_mark_module_live(group, source, starts, ready);
        for (edge = starts[source]; edge < starts[source + 1]; edge++)
        {
            unsigned int target = group->targets[edge];
            if (target != UINT_MAX && !group->live[target])
            {
                group->live[target] = 1;
                ready[group->live_count++] = target;
            }
        }
    }
    return 1;
}


/* [GC:] EN: Reclassify complete Duke group after pins were released. Caller
 * holds all captured field guards and excludes candidate-owner removal through
 * seed consumption. Reset derived live state; return zero on invalid counts.
 * RU: Повторить классификацию полного Duke group после снятия pin. Вызывающий
 * держит guards всех полей и исключает снятие кандидатов до потребления seed.
 * Сбросить вычисленные live-метки; вернуть ноль при неверных счётчиках. */
int dvm_rc_reclassify_duke_group(RecordGroup *group)
{
    return classify_record_group(group, 1);
}
