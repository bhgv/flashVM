/* Copyright (c) 2026 bhgv
 * EN: Resident-owned error/FP state and shared namespace lifetime.
 * RU: Error/FP-состояние resident и lifetime общего namespace.
 */
#ifndef DVM_PROCESS_ENVIRONMENT_H
#define DVM_PROCESS_ENVIRONMENT_H


/* EN: Initialize private runtime process, borrowing parent (NULL at startup).
 * Share its namespace, copy error/FP bytes. Return one/zero; failure publishes
 * nothing. Native allocation only; caller owns parent during preparation.
 * RU: Инициализировать приватный process с parent (NULL при startup).
 * Разделить namespace, скопировать error/FP. Один/ноль; отказ без публикации.
 * Только native allocation; caller владеет parent при подготовке. */
int dvm_process_environment_init(Prog *process, Prog *parent);


/* EN/RU: Release native group on setup failure/final retirement; NULL allowed.
 * Освободить native group при setup failure/retirement; NULL допустим. */
void dvm_process_environment_destroy(Prog *process);


/* EN: Bind this permanent Prog's embedded environment and guest FP state.
 * Save native host FP control/status on the current typed up for release.
 * Return zero for malformed host metadata; no allocation or frame/root changes.
 * The owner retains Prog/context through native suspension and reentry.
 * RU: Привязать environment/FP своего постоянного Prog; сохранить host FP
 * в текущем up. Ноль при неверных metadata; без allocation и изменения roots. */
int dvm_process_environment_bind(Prog *process);


/* EN: Save guest FP and restore this OS owner's stable default environment
 * before retirement or native suspension. Guest error text remains unchanged.
 * RU: Сохранить guest FP и вернуть стабильное host environment до retirement
 * либо native suspension. Guest error text не меняется; без передачи Prog. */
void dvm_process_environment_release(Prog *process);
#endif
