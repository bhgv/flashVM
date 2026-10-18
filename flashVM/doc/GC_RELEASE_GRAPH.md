# Поздние корни нативной финализации v0.73 / Native release graph roots

До v0.73 native resource-release callback мог добавить managed object в global,
stack, exception или property после resurrection remark. Затем sweep удалял его,
оставляя reachable dangling pointers. Regression gc_release_resurrection сначала
воспроизводит удаление опубликованных объектов, затем проверяет исправление.

После запуска resource-release callbacks добавлен traversal текущих roots и всех
ранее surviving объектов. Их предыдущие marks не сбрасываются: callback мог
удалить последний root native объекта, чей resource ещё не освобождён. Такой
объект безопасно сохраняется до следующей collection, когда нормальные finalizer/
release фазы обработают его. Force traversal ранее marked объектов замечает новые
edges, которые простой mark уже помеченного global/prototype пропускал бы.

Поздний проход сохраняет managed shell и его properties/метатаблицы/циклы.
**Уже освобождённый native payload не восстанавливается**: release pointer/resource
сбрасываются до callback, повторного освобождения нет; userdata_get такой оболочки
возвращает NULL payload. Другие native классы сохраняют правила доступа к уже
закрытым/освобождённым ресурсам. AS __gc resurrection до native release сохраняет
ресурс по прежнему контракту. Внешний C pointer сам по себе не root.

Object и traced string roots обходятся совместно. Поздний pass только добавляет
marks, поэтому удалённые callback корни могут удержать объект/строку ещё один
цикл; это консервативное сохранение. Если ни один native callback не запускался,
дополнительного traversal/allocation нет.

Late pending queue может потребовать память. При OOM sweep не запускается,
collecting flag снимается и managed nodes/string buffers остаются выделенными.
Уже выполненные resource cleanup/finalizer side effects не откатываются.
Следующая collection безопасна и не повторяет освобождённые native payloads.
GC не транзакционен относительно внешних эффектов callback.

Проверки: обе string policies, republished global/stack/exception objects,
новые edges в старом survivor, циклы, удаление root native survivor с отложенным
release, одноразовые __gc/native cleanup. gc_release_graph_allocation перебирает
отказы всех new в collection, включая late queue growth после cleanup, проверяет
50 child strings/objects, повторную collection, indexes и allocation balance.
Реальные SWF, полная extension root приёмка и distributed debugger всё ещё открыты.

## English

Native release callbacks could publish managed objects after resurrection remark,
leaving global/stack/exception/property roots pointing into swept memory. A late
root traversal now force-rescans previous survivors and follows newly published
edges before object/string sweep. Previous marks remain intact so a callback that
detaches an unreleased native survivor does not cause deletion without cleanup;
that survivor can be reclaimed normally on the next collection.

Republishing retains managed shells, properties, metatables and cycles. It cannot
restore an already-released native payload: resource/release pointers are cleared
before callback entry, cleanup is not repeated and userdata_get reports NULL.
Other native classes keep their closed/disposed resource contracts. Earlier AS
__gc resurrection still preserves resources normally. Raw host pointers are not
roots. The late pass adds marks conservatively, potentially retaining detached
nodes/strings for one additional cycle, and is skipped when no release callback ran.

Pending-queue OOM aborts sweep, preserves managed allocations/string buffers and
resets collection state. Already-completed cleanup/finalizer effects are not
rolled back; retry is safe and does not repeat released payloads. This is not
transactional GC for external callback effects.

Regressions cover both string policies, late global/stack/exception roots, changed
survivor edges, cycles, delayed cleanup of detached native survivors and exactly-once
finalization. Exhaustive new-failure injection includes late traversal after cleanup,
50 child objects/strings, retry, ownership-index consistency and teardown balance.
Full real-SWF/native extension/distributed debugger acceptance remains open.

Проверки v0.73 / verification: full headless 255/255, AGG 271/271, Cairo
271/271, portable 246/246; targeted ASan/UBSan 46/46. Heap/global ASan
and UBSan active; leak detection/stack ASan disabled. Live fonts execute in
AGG/Cairo, live TCP excluded from portable. Linux only; full stage 7 open.
