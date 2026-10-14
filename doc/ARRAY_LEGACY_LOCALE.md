# Array locale inheritance / Наследование Array.toLocaleString — v0.207

Загруженный Array.prototype не содержит штатный alias toLocaleString → join:
метод наследуется от Object и преобразует receiver через текущий toString,
игнорируя аргументы locale call. Изменённый toString вызывается один раз;
NUL, type-label fallback и ошибки сохраняются. Host alias join не меняется.

Загрузка после установки подготавливает удаление canonical alias и method cache
до изменения других прототипов. Commit удаляет сохранённые ключи без allocation;
ранее сохранённая Function-ссылка на join остаётся callable. Custom flags/data/
descriptors не удаляются. Native Array results без system prototype получают
locale projection напрямую. Native callback IDs/graph/API форматы сохранены.

## English

Loaded Array.prototype inherits locale conversion from Object instead of owning
a join alias. It uses current receiver toString, ignores locale operands and
preserves counted strings, type-label fallback and callback errors. Host alias
behavior remains. Loading after install stages canonical alias/cache removal
before any mutation and commits using retained keys without allocation. Previously
retained join Function references remain callable; custom shapes are preserved.
Prototype-free native results install receiver locale projection directly.

Tests cover host/SWF4–8/both installation orders, own-property shape, cache
removal, old function references, ignored separators, custom NUL/nonstring/error
projections, GC/channel restored arrays and prototype-free results. Joint OOM
checks retain the old alias on failure and remove it after successful retry.
Reference: pinned Ruffle array.rs PROTO_DECLS and object.rs to_locale_string,
commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Customized native layouts, raw
__proto__, property case/version visibility and all roadmap stages remain open.
