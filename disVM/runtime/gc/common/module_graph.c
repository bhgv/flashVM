/* Copyright (c) 2026 bhgv
 * EN: Shared Module identity indexing, native-owner aggregation and reachability.
 * RU: Индекс Module, объединение native-владельцев и поиск достижимости.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "graph_snapshot.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>


/* [GC:] EN: Find module's open-addressed slot in the borrowed discovery group.
 * Storage and every recorded Module stay independently live through this call.
 * RU: Найти слот module в хеш-индексе заимствованного group. Буфер и все
 * записанные Module независимо живут до возврата. */
static unsigned int module_slot(const RecordGroup *group, const void *module)
{
    uintptr_t value = (uintptr_t)module / sizeof(void *);
    unsigned int slots = group->field_limit * 4;
    unsigned int slot = (value ^ (value >> 7)) % slots;
    while (group->native_index[slot] && group->native_modules[group->native_index[slot] - 1] != module)
    {
        slot = (slot + 1) % slots;
    }
    return slot;
}


/* [GC:] EN: Associate source with module and return its first captured source.
 * group owns native index storage; no managed/native owner is acquired here.
 * RU: Связать source с module и вернуть первый узел. Индекс принадлежит group;
 * новые managed/native-ссылки не создаются. */
unsigned int dvm_rc_native_module_source(RecordGroup *group, unsigned int source, void *module)
{
    group->native_modules[source] = module;
    unsigned int slot = module_slot(group, module);
    if (!group->native_index[slot])
    {
        group->native_index[slot] = source + 1;
    }
    return group->native_index[slot] - 1;
}


/* [GC:] EN: Find module's first source in group without dereferencing module.
 * Return UINT_MAX for absent native discovery metadata or an unknown identity.
 * RU: Найти первый узел module в group без чтения module. При отсутствии
 * native-индекса либо идентичности вернуть UINT_MAX. */
unsigned int dvm_rc_native_module_first(const RecordGroup *group, const void *module)
{
    if (!group->native_index || !module)
    {
        return UINT_MAX;
    }
    unsigned int entry = group->native_index[module_slot(group, module)];
    return entry ? entry - 1 : UINT_MAX;
}


/* [GC:] EN: Count captured Modlinks per Module, and preserve original data when
 * additional native owners remain. A smaller native count invalidates capture.
 * group has complete guarded discovery; its caller later propagates live fields.
 * RU: Посчитать Modlink каждого Module и сохранить исходные данные при внешних
 * native-владельцах. Меньший native-счётчик отвергает снимок. group полностью
 * захвачен; вызывающий затем распространяет живость по полям. */
int dvm_rc_classify_modules(RecordGroup *group)
{
    if (!group->native_modules || !group->native_owners)
    {
        return 1;
    }
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        group->native_owners[index] = 0;
    }
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        void *module = group->native_modules[index];
        if (module)
        {
            group->native_owners[dvm_rc_native_module_first(group, module)]++;
        }
    }
    for (unsigned int index = 0; index < group->nodes_used; index++)
    {
        if (!group->native_owners[index])
        {
            continue;
        }
        Module *module = group->native_modules[index];
        int actual = dvm_refcount_load_i(&module->ref);
        if (actual < 0 || (unsigned int)actual < group->native_owners[index])
        {
            group->busy = 1;
            return 0;
        }
        if ((unsigned int)actual > group->native_owners[index])
        {
            group->native_owners[index] = 0;
        }
    }
    for (unsigned int edge = 0; edge < group->edges_used; edge++)
    {
        unsigned int source = group->sources[edge];
        Module *module = group->native_modules[source];
        unsigned int target = group->targets[edge];
        if (module && group->cells[edge] == &module->origmp && !group->native_owners[source] && target != UINT_MAX)
        {
            group->live[target] = 1;
        }
    }
    return 1;
}


/* [GC:] EN: A live source's Module keeps its original MP live even when another
 * Modlink supplied the single physical native edge. starts locates that first
 * source's original-MP field; ready has capacity for every newly live node.
 * RU: Module живого source сохраняет origmp, даже если единственное физическое
 * native-ребро записано от другого Modlink. starts указывает первое поле того
 * узла; ready вмещает каждый новый живой узел. */
void dvm_rc_mark_module_live(RecordGroup *group, unsigned int source, const unsigned int *starts, unsigned int *ready)
{
    Module *module = group->native_modules ? group->native_modules[source] : NULL;
    if (!module)
    {
        return;
    }
    unsigned int first = dvm_rc_native_module_first(group, module);
    group->native_owners[first] = 0;
    unsigned int edge = starts[first];
    if (edge >= group->edges_used || group->cells[edge] != &module->origmp)
    {
        abort();
    }
    unsigned int target = group->targets[edge];
    if (target != UINT_MAX && !group->live[target])
    {
        group->live[target] = 1;
        ready[group->live_count++] = target;
    }
}


/* [GC:] EN: Original-MP edge ownership moves only when all native owners die.
 * group/edge borrow complete discovery and do not change references or payload.
 * RU: Владение ребром origmp передаётся только при снятии всех native-владельцев.
 * group/edge заимствуют полный снимок без изменения ссылок или полей. */
int dvm_rc_keep_record_edge(const RecordGroup *group, unsigned int edge)
{
    unsigned int source = group->sources[edge];
    Module *module = group->native_modules ? group->native_modules[source] : NULL;
    return !module || group->cells[edge] != &module->origmp ||
           group->native_owners[dvm_rc_native_module_first(group, module)] != 0;
}
