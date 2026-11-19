/* Copyright (c) 2026 bhgv
 * EN: Owner-thread view contracts; borrowed/copied values cross these boundaries.
 * RU: Контракты view владельца; границы используют заимствованные/копии значений.
 */
/* Single-owner FlashVM renderer and copied event source for the Dis editor. */
#ifndef DVM_EDITOR_VIEW_H
#define DVM_EDITOR_VIEW_H
#include "disvm/ui_bridge.h"


/* Configure a borrowed CLI value before initialization; returns one for a known option. */
int dvm_editor_option(const char *name, const char *value);


/* Select the packaged default font beside the entry module; explicit --font wins.
 * directory is borrowed; returns one after copying a bounded native resource path. */
int dvm_editor_default_font(const char *directory);


/* Return actual visible logical-line capacity for the current font size, bounded 1..128. */
int dvm_editor_rows(void);


/* Return the copied initial line-number preference, 0/1; no UI state is mutated. */
int dvm_editor_line_numbers(void);


/* Create the owner-local FlashVM field/window. Returns one or zero with a diagnostic. */
int dvm_editor_create(void);


/* Read the configured immutable document path; borrowed until process exit. */
const char *dvm_editor_path(void);


/* Return the explicit new-document flag; no file is created until Dis saves it. */
int dvm_editor_fresh(void);


/* Accept one copied Dis presentation frame. Returns zero on protocol/render errors. */
int dvm_editor_present(const DVM_UIMessage *message);


/* Poll one native/scripted event without waiting; 1=value, 0=idle, -1=error.
 * The caller retains rejected/backpressured records; no document is mutated here. */
int dvm_editor_event(DVM_UIMessage *message);


/* Release native/FlashVM resources after all Dis channel callbacks stop. */
void dvm_editor_destroy(void);
/* EN: Reset wake state on UI main before create/after sender joins; no SDK call.
 * RU: Сбросить wake в UI main до create/после join отправителей; без SDK. */
void dvm_editor_idle_reset(void);


/* EN: Register native wake on UI main after SDL init; return 0 on exhaustion.
 * RU: Зарегистрировать native wake в UI main после SDL init; 0 при исчерпании. */
int dvm_editor_idle_initialize(void);


/* EN: Query next useful deadline in ms on UI main, without callbacks or redraw.
 * RU: Получить дедлайн в мс в UI main, без callbacks и перерисовки. */
unsigned long dvm_editor_idle_delay(void);


/* EN: UI-main wait up to delay ms; 1=SDL used, 0=caller must wait its condition.
 * RU: Ожидание UI main до delay мс; 1=SDL, 0=вызывающий ждёт свою condition. */
int dvm_editor_idle_wait(unsigned long delay);


/* EN: Wake native wait; caller pins session lifetime with its mutex. No drawing.
 * RU: Прервать native wait; вызывающий удерживает жизнь сессии замком. Без рисования. */
void dvm_editor_idle_notify(void);

#endif
