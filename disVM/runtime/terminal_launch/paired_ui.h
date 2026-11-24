/* Copyright (c) 2026 bhgv
 * EN: Mounted job's live Unibus channel pump on the runtime controller.
 * RU: Обслуживание живых каналов Unibus смонтированного задания у контроллера.
 */
#ifndef DVM_PAIRED_UI_H
#define DVM_PAIRED_UI_H
struct StyxClient;
struct UnibusBinding;

/* EN: Begin the single mounted runtime session and return its owned launch binding.
 * RU: Начать сессию смонтированного runtime и вернуть собственную привязку запуска. */
struct UnibusBinding *dvm_paired_ui_begin(struct StyxClient *client);

/* EN: Unregister after the managed controller has finished. Consume the launch binding.
 * RU: Снять регистрацию после завершения контроллера. Освободить привязку запуска. */
void dvm_paired_ui_end(struct UnibusBinding *binding);

/* EN: Report the mounted job's attached port, including deferred detach work.
 * RU: Сообщить, подключён ли порт задания, включая отложенное отсоединение. */
int dvm_paired_ui_active(void);

/* EN: Advance at most one incoming and one outgoing frame. Controller only.
 * RU: Продвинуть не более одного кадра в каждом направлении. Только контроллер. */
int dvm_paired_ui_poll(void);

/* EN: Close retained channel roots before the controller leaves the managed runtime.
 * RU: Освободить удерживаемые корни каналов до выхода контроллера из managed runtime. */
void dvm_paired_ui_finish(void);

#endif
