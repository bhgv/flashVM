/* Copyright (c) 2026 bhgv
 * EN: Short synchronized Module membership and reference transactions.
 * RU: Короткие синхронные операции состава реестра Module и ссылок.
 */
#ifndef DVM_MODULE_REGISTRY_H
#define DVM_MODULE_REGISTRY_H

struct Module; /* Native metadata; the full type is supplied by interp.h. */


/* EN: Publish count fully initialized, privately owned entries atomically.
 * entries are borrowed; ownership counts do not change. No allocation/callback.
 * RU: Атомарно опубликовать count готовых личных entries. Массив заимствован;
 * счётчики владения не меняются. Без выделений/callback. */
void dvm_module_registry_publish_set(struct Module **entries, unsigned count);


/* EN: Publish borrowed module's fully initialized metadata, preserving its owner.
 * RU: Опубликовать готовые данные заимствованного module, сохранив владельца. */
void dvm_module_registry_publish(struct Module *module);


/* EN: Find borrowed path and retain its Module before releasing the registry lock.
 * Return an owned reference or NULL. No I/O, allocation or guest callback.
 * RU: Найти path и удержать Module до снятия mutex реестра. Вернуть владеющую
 * ссылку либо NULL. Без I/O, выделений и гостевых callback. */
struct Module *dvm_module_registry_lookup(const char *path);


/* [GC:] EN: Consume module's reference and unlink its last owner atomically with
 * lookup. Return 1 only for caller-owned final destruction outside the mutex;
 * return 0 after a nonfinal release. Private unpublished modules are permitted.
 * RU: Снять ссылку module и удалить последнего владельца атомарно с поиском.
 * Вернуть 1 для финализации вызывающим вне mutex, 0 при оставшихся ссылках.
 * Допускаются личные ещё не опубликованные модули. */
int dvm_module_registry_release(struct Module *module);


/* [GC:] EN: Under the registry mutex, validate count unique owned modules have
 * one native owner, run claim(argument), then unlink only on success. claim must
 * atomically consume managed counts and native counts to zero, restoring all on
 * refusal. It may use bounded native metadata locks but never allocate, call VM,
 * reenter registry or run destructors. Entries remain independently live until
 * caller's post-epoch finalization. Return zero unchanged on outside ownership.
 * RU: Под mutex проверить count уникальных modules с одной native-ссылкой,
 * выполнить claim(argument), при успехе удалить из списка. claim захватывает
 * managed/native счётчики в ноль и полностью откатывает отказ. Допустимы только
 * ограниченные native-операции без выделений, VM, повторного входа и деструкторов.
 * Вызывающий хранит entries до финализации после эпохи. Отказ сохраняет владение. */
int dvm_module_registry_claim(struct Module **entries, unsigned count, int (*claim)(void *), void *argument);
#endif
