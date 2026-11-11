/* Copyright (c) 2026 bhgv
 * EN: RC readiness, close and post-join accounting.
 * RU: Готовность RC, закрытие и учёт после join.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Read native queue occupancy only for an ACTIVE caller in this world. Return zero for
 * native/idle/foreign/collector roles. Queue storage lives through producer and collector joins.
 * RU: Прочитать нативную занятость очередей только для ACTIVE этого мира. Для native/idle/чужого мира/сборщика
 * вернуть ноль. Очереди живут до join производителей и сборщиков. */
unsigned int dvm_runtime_rc_pending(void)
{
    DVM_RetirementStats stats = {0};
    if (admitted(0))
    {
        DVM_RetirementStats frame_stats = {0};
        (void)dvm_retirement_statistics(frames, &frame_stats);
        (void)dvm_retirement_statistics(retirement, &stats);
        return stats.ready + stats.reserved + frame_stats.ready + frame_stats.reserved +
               (dvm_retirement_peek(leaves) != NULL) + (dvm_retirement_peek(difficult) != NULL) +
               (dvm_retirement_peek(record_retries) != NULL) + (dvm_retirement_peek(cycle_refused) != NULL) +
               dvm_runtime_rc_candidates_ready() + dvm_runtime_rc_duke_ready();
    }
    return stats.ready + stats.reserved;
}


/* [GC:] EN: Read committed native entries while queues survive worker join. include_shared_retries includes
 * accepting retry entries; no managed payload is inspected. Return boolean.
 * RU: Прочитать опубликованные нативные записи при живых до join очередях. include_shared_retries учитывает
 * принимающие записи повтора; управляемые данные не читаются. Вернуть bool. */
static int retirement_ready(int include_shared_retries)
{
    DVM_RetirementStats refs = {0};
    DVM_RetirementStats packets = {0};
    DVM_RetirementStats candidates = {0};
    DVM_RetirementStats duke = {0};
    (void)dvm_retirement_statistics(duke_candidates, &duke);
    DVM_RetirementStats retries = {0};
    (void)dvm_retirement_statistics(record_retries, &retries);
    (void)dvm_retirement_statistics(cycle_candidates, &candidates);
    (void)dvm_retirement_statistics(retirement, &refs);
    (void)dvm_retirement_statistics(frames, &packets);
    return __atomic_load_n(&duke_cleanup_fallback, __ATOMIC_ACQUIRE) || refs.ready != 0 || packets.ready != 0 ||
           (duke.ready && !duke.accepting) || dvm_retirement_peek(leaves) != NULL ||
           dvm_retirement_peek(difficult) != NULL ||
           (retries.ready && (include_shared_retries || !retries.accepting)) ||
           dvm_retirement_peek(cycle_refused) != NULL || (candidates.ready && !candidates.accepting);
}


/* [GC:] EN: Report pending Fast metadata including snapshots queued for shared retry. Queue storage must remain
 * live; no admission or payload access occurs.
 * RU: Проверить задания Fast, включая shared-повторы снимков. Очереди должны жить весь вызов; допуска и
 * доступа к данным кучи нет. */
int dvm_runtime_rc_ready(void)
{
    return retirement_ready(1);
}


/* [GC:] EN: Close producers on the active controller, or after sealing with collectors idle. Queued references
 * retain roots until drain; no metadata may be freed before join.
 * RU: Закрыть производителей на активном контроллере либо после sealing при простаивающих сборщиках. Очередные
 * ссылки сохраняют корни до drain; метаданные нельзя освобождать до join. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
    dvm_retirement_close(retirement);
    dvm_retirement_close(cycle_candidates);
    dvm_retirement_close(cycle_refused);
    dvm_retirement_close(duke_candidates);
    dvm_retirement_close(frames);
    dvm_retirement_close(leaves);
    dvm_retirement_close(difficult);
    dvm_retirement_close(record_retries);
}


/* [GC:] EN: After all worker joins, report diagnostics when requested and destroy empty queues. Return false with
 * roots preserved if work remains; release native session identity only on success. Caller keeps mutator
 * TLS until return.
 * RU: После join всех служб вывести diagnostics по запросу и удалить пустые очереди. При оставшейся работе
 * вернуть false, сохранив корни; снять идентичность сеанса только при успехе. TLS исполнителя живёт до
 * возврата. */
int dvm_runtime_rc_finish(int diagnostics)
{
    DVM_RetirementStats stats = {0};
    int result;
    (void)dvm_retirement_statistics(retirement, &stats);
    if (diagnostics)
    {
        printf(
            "NATIVERC enqueued=%lu released=%lu pending=%u saturated=%lu bytes=%lu byte_limited=%lu helper_enqueued=%lu helper_saturated=%lu helper_byte_limited=%lu\n",
            enqueued,
            released,
            stats.ready + stats.reserved,
            saturated,
            stats.bytes,
            byte_limited,
            helper_enqueued,
            helper_saturated,
            helper_byte_limited);
    }
    DVM_RetirementStats frame_stats = {0};
    (void)dvm_retirement_statistics(frames, &frame_stats);
    if (diagnostics)
    {
        DVM_RetirementStats candidates = {0};
        (void)dvm_retirement_statistics(cycle_candidates, &candidates);
        printf("NATIVECYCLEQUEUE ready=%u reserved=%u capacity=%u peak_capacity=%u\n",
               candidates.ready,
               candidates.reserved,
               candidates.capacity,
               candidates.peak_capacity);
        printf("NATIVEFASTGC ignored_live_fields=%zu reserved_confirmations=%zu\n",
               fast_ignored_live_fields,
               fast_reserved_confirmations);
        printf("NATIVEDUKEGC promoted=%lu peak_fields=%u resumed=%lu adopted_pins=%lu publish_retries=%lu\n",
               duke_promoted,
               duke_peak_fields,
               duke_resumed_slices,
               duke_adopted_pins,
               duke_publish_retries);
        printf(
            "NATIVEFRAMERC drained=%lu pending=%u shared_children=%lu record_groups=%lu shared_record_groups=%lu shared_record_groups_with_peer=%lu shared_record_scans=%lu shared_record_snapshots_with_peer=%lu shared_record_confirmations=%lu cycle_candidates_rotated=%lu cycle_candidates_coalesced=%lu cycle_candidates_rooted=%lu cycle_candidates_oversized=%lu cycle_candidates_full=%lu\n",
            frames_drained,
            frame_stats.ready + frame_stats.reserved,
            shared_children,
            record_groups,
            shared_record_groups,
            shared_record_groups_with_peer,
            shared_record_scans,
            shared_record_snapshots_with_peer,
            shared_record_confirmations,
            cycle_candidates_rotated,
            cycle_candidates_coalesced,
            cycle_candidates_rooted,
            cycle_candidates_oversized,
            cycle_candidates_full);
    }
    if (diagnostics)
    {
        printf(
            "NATIVEMANDATORYCYCLE transferred=%lu retries=%lu\n", mandatory_cycle_transfers, mandatory_cycle_retries);
        printf("NATIVEMANDATORYRC shared=%lu\n", shared_mandatory_owners);
        printf("NATIVELISTRC shared=%lu\n", shared_lists);
        printf("NATIVERECORDRC shared=%lu\n", shared_plain_records);
        printf("NATIVECHANNELRC shared=%lu\n", shared_channels);
        printf("NATIVETABLERC tables=%lu keys=%lu with_peer=%lu\n",
               shared_tables,
               shared_keys,
               shared_table_owners_with_peer);
        printf("NATIVEFDRC shared=%lu with_peer=%lu\n", shared_files, shared_files_with_peer);
        printf("NATIVESTRINGRC shared=%lu with_peer=%lu\n", shared_strings, shared_strings_with_peer);
        printf("NATIVEARRAYRC shared=%lu with_peer=%lu graphs=%lu\n",
               shared_arrays,
               shared_arrays_with_peer,
               shared_array_graphs);
    }
    if (diagnostics)
    {
        printf(
            "NATIVERCPASSES idle=%lu promoted=%lu packet_promoted=%lu groups=%lu nonfinal=%lu records=%lu arrays=%lu lists=%lu channels=%lu strings=%lu modules=%lu other=%lu\n",
            fallback.passes,
            fallback.promoted,
            fallback.packet_promoted,
            fallback.groups,
            fallback.nonfinal,
            fallback.records,
            fallback.arrays,
            fallback.lists,
            fallback.channels,
            fallback.strings,
            fallback.modules,
            fallback.other);
        DVM_RetirementStats difficult_stats = {0};
        (void)dvm_retirement_statistics(difficult, &difficult_stats);
        printf("NATIVERCFALLBACK moved=%lu pending=%u\n",
               difficult_moved,
               difficult_stats.ready + difficult_stats.reserved);
    }
    result = !duke_inflight.owned_scratch && !duke_abandoned.pinned_nodes &&
             (duke_candidates == NULL || dvm_retirement_destroy(&duke_candidates)) &&
             (cycle_candidates == NULL || dvm_retirement_destroy(&cycle_candidates)) &&
             (cycle_refused == NULL || dvm_retirement_destroy(&cycle_refused)) &&
             (record_retries == NULL || dvm_retirement_destroy(&record_retries)) &&
             (difficult == NULL || dvm_retirement_destroy(&difficult)) &&
             (leaves == NULL || dvm_retirement_destroy(&leaves)) &&
             (retirement == NULL || dvm_retirement_destroy(&retirement)) &&
             (frames == NULL || dvm_retirement_destroy(&frames));
    if (result)
    {
        if (candidate_transfer)
        {
            fvm_port_sync_destroy(candidate_transfer);
            candidate_transfer = NULL;
        }
        owner = NULL;
        world = NULL;
        notify_ready = NULL;
    }
    return result;
}
