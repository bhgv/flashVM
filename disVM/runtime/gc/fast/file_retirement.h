/* Copyright (c) 2026 bhgv
 * EN: Shared Fast retirement for the known pointer-free Sys.FD owner only.
 * RU: Параллельное освобождение Fast только известного беспоинтерного Sys.FD.
 */
#ifndef DVM_GC_FAST_FILE_RETIREMENT_H
#define DVM_GC_FAST_FILE_RETIREMENT_H


/* [GC:] EN: Return whether borrowed heap is the startup-published Sys.FD type.
 * No allocation/mutation; other custom finalizers are never admitted here.
 * RU: Проверить тип заимствованного heap на опубликованный при старте Sys.FD.
 * Без выделения/изменения; другие пользовательские финализаторы не допускаются. */
int dvm_gc_fast_file_eligible(const Heap *heap);


/* [GC:] EN: Run the known FD finalizer after Fast claims its last reference.
 * heap has ref=0 and no other owners/borrowers. Detach its native slot and queue
 * the preallocated close job; do not free the heap/type (the caller does that).
 * The native close worker remains active through GC join. No managed callbacks,
 * allocation or blocking filesystem I/O occurs here.
 * RU: Выполнить известный финализатор FD после захвата последней ссылки Fast.
 * У heap ref=0 и нет других владельцев/заёмщиков. Отделить слот и поставить
 * готовый close job. Нативный closer работает до join GC. Кучу/тип освобождает
 * вызывающий. Нет управляемых вызовов,
 * выделения памяти или блокирующего файлового I/O. */
void dvm_gc_fast_file_finalize(Heap *heap);
#endif
