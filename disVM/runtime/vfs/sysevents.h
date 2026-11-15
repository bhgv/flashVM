/* Copyright (c) 2026 bhgv
 * EN: Automatic per-resident namespace invalidation mailbox and its VFS device.
 * RU: Автоматический per-resident namespace invalidation mailbox и VFS device.
 */
#ifndef DVM_VFS_SYSEVENTS_H
#define DVM_VFS_SYSEVENTS_H
#include "resource.h"
#define DVM_SYSEVENT_LIMIT 32U /* Maximum queued records; overflow requests full resynchronization. */
#define DVM_SYSEVENT_VFS 1U /* A provider currently bound in this namespace changed. */
#define DVM_SYSEVENT_NAMESPACE 2U /* Namespace topology/cwd/revision changed. */
#define DVM_SYSEVENT_RESYNC 3U /* Notification detail was lost; refresh current namespace assumptions. */
#define DVM_SYSEVENT_EXIT 4U /* The owning resident retired; queued records remain drainable. */
#define DVM_SYSEVENT_FRAME_BYTES (2U * DVM_NS_PATH_LIMIT + 192U) /* Hex path plus scalar envelope. */
typedef struct DVM_SysEvents DVM_SysEvents; /* Native refcounted owner inbox, no managed pointers. */


/* EN/RU: Create independent inbox and automatic observer for borrowed group.
 * Retains group; owned result/NULL. Создать inbox, удержать group; свой result/NULL. */
DVM_SysEvents *dvm_sysevents_create(DVM_Namespace *group);


/* EN/RU: Retain live inbox for an opened FD; one/zero success/overflow.
 * Удержать inbox для opened FD; один/ноль. No managed access. */
int dvm_sysevents_retain(DVM_SysEvents *events);


/* EN/RU: Consume ref; final release drains native messages/groups, NULL allowed.
 * Поглотить ref; final release очищает native messages/groups; NULL допустим. */
void dvm_sysevents_release(DVM_SysEvents *events);


/* EN/RU: Owner retirement stops publication; retained descriptors can drain then EOF.
 * Retirement прекращает publication; retained FD читают остаток, затем EOF. */
void dvm_sysevents_retire(DVM_SysEvents *events);


/* EN/RU: Replace observed group on successful pctl; retain before mutation.
 * Purges old-scope notifications. One/zero, no provider I/O or callbacks.
 * Сменить group после pctl; удалить old-scope notifications. Один/ноль, без I/O. */
int dvm_sysevents_group(DVM_SysEvents *events, DVM_Namespace *group);


/* EN/RU: Bind inbox to permanent native owner TLS; NULL unbinds. No polling or ownership change.
 * Привязать inbox к owner TLS; NULL отвязывает. Без передачи владения. */
void dvm_sysevents_bind(DVM_SysEvents *events);


/* EN/RU: Borrow this permanent OS owner's native inbox while its reference lives.
 * Заимствовать inbox своего OS owner при удержанной reference. */
DVM_SysEvents *dvm_sysevents_current(void);


/* EN/RU: Construct boot-installed contextual device; exactly one live instance.
 * Contextual opens capture current owner's inbox, never a caller-supplied PID.
 * Создать boot device; opens захватывают current owner inbox, без PID от guest. */
DVM_Provider *dvm_sysevents_device(void);


/* EN/RU: Recognize the device itself to avoid monitoring its own inert journal.
 * Узнать собственный device, чтобы не наблюдать его inert journal. */
int dvm_sysevents_is_device(DVM_Provider *provider);
#endif
