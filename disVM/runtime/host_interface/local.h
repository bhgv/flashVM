/* Copyright (c) 2026 bhgv
 * EN: Legacy local graphical backend behind the per-launch dispatcher.
 * RU: Существующий локальный графический backend за per-launch диспетчером.
 */
#ifndef DVM_IDE_LOCAL_H
#define DVM_IDE_LOCAL_H


/* EN/RU: Borrow the owner-local ink frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame ink, сохранить прежний result/error contract. */
void dvm_local_Unibus_ink(void *frame);


/* EN/RU: Borrow the owner-local inkadmit frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame inkadmit, сохранить прежний result/error contract. */
void dvm_local_Unibus_inkadmit(void *frame);


/* EN/RU: Borrow the owner-local inkdone frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame inkdone, сохранить прежний result/error contract. */
void dvm_local_Unibus_inkdone(void *frame);


/* EN/RU: Borrow the owner-local linenumbers frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame linenumbers, сохранить прежний result/error contract. */
void dvm_local_Unibus_linenumbers(void *frame);


/* EN/RU: Borrow the owner-local rows frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame rows, сохранить прежний result/error contract. */
void dvm_local_Unibus_rows(void *frame);


/* EN/RU: Borrow the owner-local attach frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame attach, сохранить прежний result/error contract. */
void dvm_local_Unibus_attach(void *frame);


/* EN/RU: Borrow the owner-local attachbulk frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame attachbulk, сохранить прежний result/error contract. */
void dvm_local_Unibus_attachbulk(void *frame);


/* EN/RU: Borrow the owner-local detach frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame detach, сохранить прежний result/error contract. */
void dvm_local_Unibus_detach(void *frame);


/* EN/RU: Borrow the owner-local fresh frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame fresh, сохранить прежний result/error contract. */
void dvm_local_Unibus_fresh(void *frame);


/* EN/RU: Borrow the owner-local path frame; preserve its existing result/error contract.
 * Заимствовать owner-local frame path, сохранить прежний result/error contract. */
void dvm_local_Unibus_path(void *frame);

#endif
