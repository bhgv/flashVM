/* Copyright (c) 2026 bhgv
 * EN: Ordinary mutex for native UTF-8 cache creation and validation.
 * RU: Обычный mutex для создания и проверки нативного кэша UTF-8.
 */
#ifndef DVM_STRING_VIEW_SYNC_H
#define DVM_STRING_VIEW_SYNC_H


/* [GC:] EN: Enter cache metadata exclusion. Caller owns a rooted live String;
 * no managed callback or admission change may run until unlock. Blocks normally.
 * RU: Войти в защиту кэша живой укоренённой String; до unlock нельзя вызывать
 * managed-код или менять допуск GC. Обычное блокирующее ожидание. */
void dvm_string_view_lock(void);


/* [GC:] EN: Release cache exclusion after all view bytes are initialized.
 * RU: Снять защиту кэша после полной инициализации байтов view. */
void dvm_string_view_unlock(void);

#endif
