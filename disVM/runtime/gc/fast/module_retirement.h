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
 * to a rooted packet and its reader epoch elapsed. Caller keeps shared admission and
 * RC acceptance until return, so unload queues origmp rather than recursively
 * destroying it. Registry unlinks LAST before freeing metadata outside its lock.
 * Caller frees the link heap and static Type owner; no link/Module alias escapes.
 * RU: Завершить обычный link с нулевым счётчиком после передачи MP в пакет
 * с корнем и завершения эпохи. Вызывающий сохраняет shared-допуск и приём RC
 * до возврата: unload ставит origmp в очередь вместо рекурсивного разрушения.
 * Реестр удаляет LAST до освобождения метаданных вне mutex. Кучу link и ссылку
 * статического Type снимает вызывающий; алиасы link/Module не выходят наружу. */
void dvm_gc_fast_module_finalize(Heap *heap);
#endif
