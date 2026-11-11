/* Copyright (c) 2026 bhgv
 * EN: Native builder mutex and release/acquire publication of immutable JIT code.
 * RU: Mutex native-строителя и release/acquire-публикация неизменного JIT-кода.
 */
#ifndef DVM_JIT_PUBLICATION_H
#define DVM_JIT_PUBLICATION_H
struct Module;
struct Jit;


/* [GC:] EN: Serialize native builders; caller retains Module and catches builder
 * errors before unlock. No guest execution or GC wait inside the protected work.
 * RU: Сериализовать native-строителей; Module удерживается, ошибки перехватываются
 * до unlock. Внутри нет исполнения гостя или ожидания GC. */
void dvm_jit_build_lock(void);


/* [GC:] EN: Leave builder exclusion after publication or complete native cleanup.
 * RU: Снять защиту строителя после публикации либо полной native-очистки. */
void dvm_jit_build_unlock(void);


/* [GC:] EN: Borrow module's immutable published code, or NULL; acquire no owner.
 * Caller independently retains module until finished using the returned table.
 * RU: Заимствовать опубликованный код module либо NULL; владение не передаётся.
 * Вызывающий отдельно удерживает module всё время использования таблицы. */
struct Jit *dvm_jit_load(struct Module *module);


/* [GC:] EN: Publish fully initialized code under builder exclusion. Transfer code
 * ownership to retained module; native instructions/cache permissions are ready.
 * RU: Под защитой строителя передать готовый code удерживаемому module;
 * native-инструкции и права памяти уже подготовлены. */
void dvm_jit_publish(struct Module *module, struct Jit *code);

#endif
