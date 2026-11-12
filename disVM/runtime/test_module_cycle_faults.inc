/* Copyright (c) 2026 bhgv
 * EN: Real native-count/epoch claim refusal and exact rollback observations.
 * RU: Отказы native-счётчика/эпохи при настоящем захвате и наблюдение отката.
 */


/* [GC:] EN: Refuse one native Module CAS unchanged after managed counts became
 * zero. In two-Module mode the first native count is already reserved. Other
 * counter/expected/replacement operations forward to the actual primitive.
 * RU: Отказать одному CAS Module без изменения после обнуления managed-ссылок.
 * Для двух Module первый native-счётчик уже захвачен. Остальные операции
 * counter/expected/replacement передаются настоящему примитиву. */
int __wrap_dvm_refcount_compare_i(int *counter, int expected, int replacement)
{
    Module *module = __atomic_load_n(SCENARIO == 16 ? &second_module : &target_module, __ATOMIC_ACQUIRE);
    if (confirming && (SCENARIO == 14 || SCENARIO == 16 || SCENARIO == 21 || SCENARIO == 32) && !native_refused &&
        module && counter == &module->ref &&
        expected == (SCENARIO == 32   ? 3
                     : SCENARIO == 21 ? 2
                                      : 1) &&
        replacement == 0)
    {
        require(dvm_refcount_load_u(&nodes[0]->ref) == 0 && dvm_refcount_load_u(&nodes[1]->ref) == 0,
                "managed graph is claimed before native Module CAS");
        if (SCENARIO == 16)
        {
            require(dvm_refcount_load_i(&target_module->ref) == 0, "first native Module claimed before second refusal");
        }
        native_refused = 1;
        return 0;
    }
    return __real_dvm_refcount_compare_i(counter, expected, replacement);
}


/* [GC:] EN: Refuse one final epoch only after real managed/native counts are
 * reserved. world and every other retirement call pass through unchanged.
 * RU: Отказать одной последней эпохе только после настоящего захвата managed/
 * native-счётчиков. world и остальные вызовы эпохи проходят без изменения. */
unsigned long __wrap_dvm_world_retire_epoch(DVM_NativeWorld *world)
{
    if (confirming && (SCENARIO == 15 || SCENARIO == 22) && !epoch_refused)
    {
        require(dvm_refcount_load_i(&target_module->ref) == 0 && dvm_refcount_load_u(&nodes[0]->ref) == 0,
                "final epoch follows native and managed claims");
        epoch_refused = 1;
        return 0;
    }
    return __real_dvm_world_retire_epoch(world);
}


/* [GC:] EN: Before packet replay can consume references, verify the real refusal
 * restored every count in rooted group. selected/result describe the actual
 * confirmation; no count or field is modified by this observer.
 * RU: До снятия ссылок повтором пакета проверить восстановление всех счётчиков
 * настоящего group после отказа. selected/result описывают подтверждение;
 * наблюдатель не меняет поля или счётчики. */
static void observe_claim_rollback(const RecordGroup *group, int selected, DVM_CellResult result)
{
    if (!selected)
    {
        return;
    }
    pthread_mutex_lock(&sync);
    if ((SCENARIO == 13 || SCENARIO == 24) && racing_lookup && !lookup_refused)
    {
        require(result == DVM_CELL_REFUSED && dvm_refcount_load_i(&target_module->ref) == (SCENARIO == 24 ? 3 : 2),
                "outside Module lookup defeats actual graph claim");
        lookup_refused = 1;
    }
    if ((native_refused || epoch_refused) && !rollback_seen)
    {
        require(result == DVM_CELL_REFUSED &&
                    dvm_refcount_load_i(&target_module->ref) == ((SCENARIO == 21 || SCENARIO == 22) ? 2
                                                                 : SCENARIO == 32                   ? 3
                                                                                                    : 1),
                "native Module count restored after claim refusal");
        if (SCENARIO == 16)
        {
            require(dvm_refcount_load_i(&second_module->ref) == 1, "all native Module counts restored");
        }
        for (unsigned index = 0; index < group->nodes_used; index++)
        {
            require(dvm_refcount_load_u(&D2H(group->nodes[index])->ref) == group->incoming[index] + (index == 0),
                    "all managed graph counts restored before replay");
        }
        rollback_seen = 1;
    }
    pthread_mutex_unlock(&sync);
}


/* [GC:] EN: Control 28 restores the old one-owner registry preflight without
 * changing any count. Forward entries/count/claim/argument; otherwise use the
 * actual aggregated owners vector. This must not collect a two-owner Module.
 * RU: Контроль 28 возвращает старую проверку одной ссылки, не меняя счётчики.
 * Передать entries/count/claim/argument; иначе использовать настоящий owners.
 * Старое условие не должно собрать Module с двумя native-владельцами. */
int __wrap_dvm_module_registry_claim_counts(
    Module **entries, const unsigned int *owners, unsigned count, int (*claim)(void *), void *argument)
{
    return __real_dvm_module_registry_claim_counts(entries, CONTROL == 28 ? NULL : owners, count, claim, argument);
}
