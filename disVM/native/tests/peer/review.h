/* Copyright (c) 2026 bhgv
 * EN: Native test-owner worker and readiness notification; no guest state.
 * RU: Native worker владельца теста и fd уведомления; без guest state.
 */
#ifndef PEER_REVIEW_H
#define PEER_REVIEW_H
#include "export.h"

/* EN/RU: Borrow owner until joined stop; retain chosen provider;0/-1.
 * Заимствовать owner до joined stop, удержать chosen provider;0/-1. */
int reviewstart(DVM_Publication *owner, DVM_Provider *chosen);
/* EN/RU: Native publication notification; signal queued work, never run a guest.
 * Native notification публикации; сигнализировать работу без guest execution. */
void reviewnotify(void *context);
/* EN/RU: Explicit test-owner approval, wake worker; thread-safe.
 * Явное разрешение тестового owner, разбудить worker; thread-safe. */
void reviewpermit(void);
/* EN/RU: Borrow readiness read fd, or -1 when inactive; owner thread only.
 * Заимствовать read fd готовности либо -1, только owner thread. */
int reviewfd(void);
/* EN/RU: Drain readiness bytes without waiting;1 notified,0 empty; owner only.
 * Вычитать readiness bytes без ожидания;1 уведомление,0 пусто, owner only. */
int reviewdrain(void);
/* EN/RU: Signal native socket owner; safe while worker exists, before end.
 * Сигнализировать native socket-owner до end. */
void reviewwake(void);
/* EN/RU: Stop/join worker before publication close; idempotent, native host only.
 * Остановить/join worker до publication close; idempotent, только native host. */
void reviewstop(void);
/* EN/RU: Consume pipe/provider after worker stop and socket owner end.
 * Поглотить pipe/provider после worker stop и socket owner end. */
void reviewend(void);
#endif
