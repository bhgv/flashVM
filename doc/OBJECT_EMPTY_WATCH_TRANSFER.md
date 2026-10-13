# Empty watcher transport / Передача empty watcher — v0.171

Graph decoder принимает пустое имя watcher, которое loaded watch уже разрешает.
Проверки типа descriptor, уникальности ключа, ссылок и handler сохраняются.
Format/ABI не меняются. Изменение согласует существующий encoder и decoder;
оно не меняет правило игнорирования assignment пустого имени в loaded Object.

object_empty_watch_transfer проверяет SWF4..8: сохранённое до load пустое
свойство 9, watch с AVM1 handler и userData=self, traced GC, channel transfer,
повторный traced GC, alias receiver, игнорирование assignment 81 без callback,
unwatch true/false и восстановленные roots/watch-running. Callback marker called
не создаётся. Точная версия masks, остальные watcher/transport cases и все
стадии остаются открытыми.

## English

Graph decoding accepts empty watcher keys already supported by loaded watch.
Descriptor type, duplicate-key, reference and handler checks remain. Wire format
and ABI are unchanged; this aligns the existing encoder/decoder. Empty-name
assignment continues to be ignored in loaded Objects.

SWF4..8 tests cover a pre-load empty property, AVM1 handler, self userData,
traced GC/channel transfer/GC, preserved aliases, ignored empty assignment without
handler effects, successful/repeated unwatch and restored roots/watch state.
Attribute version masks, other watcher/transport cases and all stages remain open.
