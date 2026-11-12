/* Copyright (c) 2026 bhgv
 * EN: Known ordinary Modlink finalization after MP transfer and reader grace.
 * RU: Известная финализация обычного Modlink после передачи MP и эпохи читателей.
 */
#ifndef DVM_GC_FAST_MODULE_RETIREMENT_H
#define DVM_GC_FAST_MODULE_RETIREMENT_H


/* [GC:] EN: Inspect independently owned heap for an ordinary hosted Modlink.
 * Reject dynamic code/data modules and malformed/absent MP or Module identities.
 * No ownership, root or admission changes; the module reference belongs to link.
 * RU: Проверить собственный heap как обычный hosted Modlink. Отвергнуть
 * динамические код/данные и отсутствующие MP/Module. Ссылки, корни и допуск
 * не меняются; ссылкой Module владеет link. */
int dvm_gc_fast_module_eligible(const Heap *heap);


/* [GC:] EN: Finalize a claimed zero-count ordinary link after its MP was moved
 * to a rooted packet or consumed by a closed graph claim, after its reader epoch.
 * Caller keeps shared admission and
 * RC acceptance until return, so unload queues origmp rather than recursively
 * destroying it. Registry unlinks LAST before freeing metadata outside its lock.
 * Claimed graphs may also finalize under stopped admission after RC closes.
 * Caller frees the link heap and static Type owner; no link/Module alias escapes.
 * RU: Завершить обычный link с нулевым счётчиком после передачи MP в пакет
 * с корнем либо снятия ребра захватом графа, после завершения эпохи.
 * Захваченный граф допускает остановленную финализацию при закрытом RC.
 * Иначе вызывающий сохраняет shared-допуск и приём RC
 * до возврата: unload ставит origmp в очередь вместо рекурсивного разрушения.
 * Реестр удаляет LAST до освобождения метаданных вне mutex. Кучу link и ссылку
 * статического Type снимает вызывающий; алиасы link/Module не выходят наружу. */
void dvm_gc_fast_module_finalize(Heap *heap);


/* [GC:] EN: Borrow module while independently owned; retain an extra original MP
 * owner when a nonfinal native release may unblock a cycle. Return owned MP or H.
 * Caller passes the extra owner to destroy after registry release; no VM safepoint
 * occurs between this retain and publication. No original field is changed.
 * RU: При независимом владении module удержать дополнительный origmp, если
 * непоследний native-release может открыть цикл. Вернуть владение MP либо H.
 * После release вызывающий передаёт его destroy без промежуточной точки VM. */
void *dvm_gc_module_retry_mp(Module *module);


/* [GC:] EN: After the final reader epoch, finalize the graph-owned zero-count
 * native module referenced by claimed heap. Both managed MP edges were consumed
 * by the graph claim. Caller clears the link MP and frees heap afterwards.
 * RU: После последней эпохи завершить native Module с нулём ссылок из heap.
 * Оба managed-ребра MP уже сняты захватом графа. Вызывающий очищает MP link и
 * затем освобождает heap. */
void dvm_gc_fast_module_graph_finalize(Heap *heap, Module *module);


/* [GC:] EN: Native loader metadata cleanup after its registry owner was consumed.
 * Caller owns module and has either stopped admission or a valid final epoch.
 * origmp must be H when its strong owner was consumed by a graph claim.
 * RU: Очистить native-метаданные loader после снятия ссылки реестра. module
 * принадлежит вызывающему под остановкой либо после последней эпохи. origmp
 * равен H, если его ссылку снял захват графа. */
void dvm_runtime_module_finish(Module *module);
#endif
