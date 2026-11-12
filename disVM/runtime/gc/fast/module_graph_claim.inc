/* Copyright (c) 2026 bhgv
 * EN: Atomically claim managed graphs and their aggregated native Module owners.
 * RU: Атомарно захватить managed-графы и объединённых native-владельцев Module.
 */


/* [GC:] EN: Borrowed state through one bounded registry claim callback.
 * RU: Заимствованное состояние одного ограниченного callback захвата реестра. */
typedef struct ModuleGraphClaim
{
    const RecordGroup *group; /* Rooted snapshot with reserved field cells. */
    Module **modules;         /* Unique Modules represented by dead Modlinks. */
    unsigned int *owners;     /* Expected native count for each unique Module. */
    unsigned count;           /* Number of unique native Modules in this transaction. */
    DVM_NativeWorld *world;   /* Native reader epoch domain, alive through return. */
    unsigned long *epoch;     /* Optional final cutoff destination. */
} ModuleGraphClaim;


/* [GC:] EN: Under the Module registry mutex when count is nonzero, claim heap
 * counts FIRST, preventing valid new direct Module retains through these links.
 * Then reserve the aggregated native owners and create the final reader cutoff. Roll back
 * every managed/native count on refusal. No allocation, guest callback or registry
 * reentry; world metadata follows the registry mutex in this bounded native call.
 * argument borrows the snapshot, unique module vector and optional epoch output.
 * RU: Под mutex реестра при ненулевом count СНАЧАЛА захватить managed-счётчики,
 * исключив новые корректные прямые retain Module через эти link. Затем захватить
 * native-владельцев и создать последнюю эпоху. При отказе откатить все счётчики.
 * Без выделений, гостевых callback и повторного входа; порядок mutex: реестр,
 * затем метаданные мира. argument заимствует снимок, вектор Module и выход эпохи. */
static int claim_module_graph_counts(void *argument)
{
    ModuleGraphClaim *claim = argument;
    if (!claim_record_group(claim->group))
    {
        return 0;
    }
    unsigned reserved = 0;
    while (reserved < claim->count &&
           dvm_refcount_compare_i(&claim->modules[reserved]->ref, (int)claim->owners[reserved], 0))
    {
        reserved++;
    }
    if (reserved == claim->count && (!claim->epoch || (*claim->epoch = dvm_world_retire_epoch(claim->world))))
    {
        return 1;
    }
    while (reserved)
    {
        reserved--;
        if (!dvm_refcount_compare_i(&claim->modules[reserved]->ref, 0, (int)claim->owners[reserved]))
        {
            abort();
        }
    }
    restore_claimed_counts(claim->group, claim->group->dead_count);
    return 0;
}


/* [GC:] EN: Collect distinct native owners attributed to dead Modlinks in group.
 * Allocate large native vectors before taking the registry mutex. Claim returns
 * false unchanged on storage failure, outside ownership or atomic count failure.
 * Success unlinks reserved Modules before lookup resumes, but preserves their
 * payload and original MP fields until the final world epoch. epoch is optional
 * only for groups without native owners; world stays live through all reclamation.
 * RU: Собрать разные native-владельцы мёртвых Modlink из group. Большой вектор
 * выделить до mutex реестра. При отказе памяти/внешнем владельце/счётчиках вернуть
 * ноль без изменения. Успех удаляет Module до продолжения lookup, но сохраняет
 * payload и origmp до последней эпохи world. Для native-владельцев epoch обязателен. */
static int claim_graph_with_modules(const RecordGroup *group, DVM_NativeWorld *world, unsigned long *epoch)
{
    Module *local[RECORD_NODE_CAPACITY];
    unsigned int local_owners[RECORD_NODE_CAPACITY];
    Module **modules = local;
    unsigned int *owners = local_owners;
    if (group->nodes_used > RECORD_NODE_CAPACITY)
    {
        modules = malloc(group->nodes_used * (sizeof(*modules) + sizeof(*owners)));
        if (!modules)
        {
            return 0;
        }
        owners = (unsigned int *)(modules + group->nodes_used);
    }
    unsigned count = 0;
    for (unsigned position = 0; group->native_modules && position < group->dead_count; position++)
    {
        unsigned index = dead_node_at(group, position);
        Module *module = group->native_modules[index];
        if (module && group->native_owners[index])
        {
            owners[count] = group->native_owners[index];
            modules[count++] = module;
        }
    }
    ModuleGraphClaim claim = {group, modules, owners, count, world, epoch};
    int result =
        count ? (epoch && dvm_module_registry_claim_counts(modules, owners, count, claim_module_graph_counts, &claim))
              : claim_module_graph_counts(&claim);
    if (modules != local)
    {
        free(modules);
    }
    return result;
}
