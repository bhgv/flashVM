/* Copyright (c) 2026 bhgv
 * EN: Rooted RC publication and shared/retained collector interfaces.
 * RU: Публикация RC с корнями и интерфейсы shared/резервного сборщика.
 */
#ifndef DVM_RUNTIME_RC_H
#define DVM_RUNTIME_RC_H
#include <stddef.h>

struct Type;
struct DVM_NativeContext;


/* [GC:] EN: Start empty queues on the bound controller before producers publish.
 * Mandatory allocation failure or an unfinished previous session aborts.
 * RU: Создать пустые очереди на контроллере до публикаций производителей.
 * Отказ обязательного выделения или незавершённый прошлый сеанс вызывает abort. */
void dvm_runtime_rc_start(void);


/* [GC:] EN: Enable shared-only RC before producer startup; no ownership moves.
 * RU: До запуска производителей включить RC без эксклюзива; владение не меняется. */
void dvm_runtime_rc_set_concurrent(void);


/* [GC:] EN: Validate native fallback metadata under strict policy; forbidden
 * queued work prints its reason and exits 70 with owners/roots still retained.
 * RU: Проверить резервные native-очереди строгого режима; запрещённая работа
 * печатает причину и завершает процесс кодом 70 при сохранённых корнях/владельцах. */
void dvm_runtime_rc_check_concurrent(void);


/* [GC:] EN: Borrow optional notify until join; configure before producer startup.
 * RU: Заимствовать необязательный notify до join; настроить до запуска производителей. */
void dvm_runtime_rc_set_notifier(void (*notify)(void));


/* [GC:] EN: Transfer mapped frame/type owners and optional extra under shared
 * admission. Return 1 on success, -1 on retryable pressure, 0 otherwise. Refusal
 * preserves the frame; success requires unpublishing it before polling.
 * RU: Передать размеченные ссылки frame/type и необязательную extra под shared.
 * Вернуть 1 при успехе, -1 при временном заполнении, иначе 0. Отказ сохраняет
 * кадр; при успехе снять его публикацию до polling. */
int dvm_runtime_rc_transfer_frame(const void *frame, const struct Type *type, void *extra);


/* [GC:] EN: Consume one value reference from ACTIVE/SHARED admission in this
 * world. Supported final owners enter epoch-gated Fast; optional refusal uses
 * the mandatory rooted FIFO. Return 0 for invalid/closed admission unchanged.
 * Mandatory publication failure terminates rather than losing ownership.
 * RU: Снять ссылку value из ACTIVE/SHARED этого мира. Поддержанные последние
 * ссылки идут в Fast после эпохи; при необязательном отказе — в FIFO с корнем.
 * При неверном/закрытом допуске вернуть 0 без изменения. Обязательный отказ
 * завершает процесс, не теряя владение. */
int dvm_runtime_rc_defer(void *value);


/* [GC:] EN: Consume at most budget top-level owners on a stopped collector.
 * Recursive legacy destruction can exceed budget. Return remaining work.
 * RU: Снять до budget верхних владельцев на остановленном сборщике. Рекурсивное
 * прежнее разрушение может превысить бюджет. Вернуть оставшуюся работу. */
unsigned int dvm_runtime_rc_drain(unsigned int budget);


/* [GC:] EN: Query native occupancy in ACTIVE admission; other roles return 0.
 * Queue mutexes may briefly block, but no managed admission is acquired.
 * RU: Проверить нативную занятость из ACTIVE; иные роли получают 0. Мьютексы
 * очередей могут ненадолго блокировать, но управляемый допуск не захватывается. */
unsigned int dvm_runtime_rc_pending(void);


/* [GC:] EN: Query committed Fast metadata without admission; queues stay live.
 * RU: Проверить опубликованные метаданные Fast без допуска; очереди должны жить. */
int dvm_runtime_rc_ready(void);


/* [GC:] EN: Close on the active controller or after sealing with idle collectors.
 * Queued owners keep their roots; storage must survive all joins.
 * RU: Закрыть на активном контроллере либо после sealing простаивающих сборщиков.
 * Ссылки сохраняют корни; память очередей должна пережить все join. */
void dvm_runtime_rc_close(void);


/* [GC:] EN: After all joins, optionally print diagnostics and free empty queues.
 * Return 0 with owners preserved if work remains; keep mutator TLS through return.
 * RU: После всех join вывести diagnostics по запросу и удалить пустые очереди.
 * При оставшейся работе вернуть 0, сохранив ссылки; TLS должен жить до возврата. */
int dvm_runtime_rc_finish(int diagnostics);


/* [GC:] EN: Query committed Cycles metadata without payload access or admission.
 * RU: Проверить метаданные Cycles без доступа к данным кучи или допуска. */
int dvm_runtime_rc_candidates_ready(void);


/* [GC:] EN: Query committed Duke metadata without payload access or admission.
 * RU: Проверить метаданные ДюкGC без доступа к данным кучи или допуска. */
int dvm_runtime_rc_duke_ready(void);


/* [GC:] EN: Run one Duke slice on bound context within budget; return work/retry.
 * Queues/world remain runtime-owned. Refusal preserves published owners.
 * RU: Выполнить порцию ДюкGC на context в пределах budget; вернуть работу/повтор.
 * Очереди/мир принадлежат runtime. Отказ сохраняет опубликованные ссылки. */
unsigned int dvm_runtime_rc_shared_duke(struct DVM_NativeContext *context, size_t budget);


/* [GC:] EN: Run shared Cycles on bound context within budget; return work/retry.
 * Roots protect candidates; no retained tracing is called by this interface.
 * RU: Выполнить shared Cycles на context с budget; вернуть работу/повтор. Корни
 * защищают кандидатов; этот интерфейс не вызывает резервную трассировку. */
unsigned int dvm_runtime_rc_shared_candidates(struct DVM_NativeContext *context, size_t budget);


/* [GC:] EN: Run shared Fast on bound context within budget; return progress or
 * reader retry. Mandatory owners, leaves and packets retain roots on refusal.
 * RU: Выполнить shared Fast на context с budget; вернуть работу или повтор
 * читателей. Обязательные ссылки, листья и пакеты сохраняют корни при отказе. */
unsigned int dvm_runtime_rc_shared_leaves(struct DVM_NativeContext *context, size_t budget);
#endif
