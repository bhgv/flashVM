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
#endif
