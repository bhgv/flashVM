# Совместимость и предварительная проверка входа проигрывателя

[English](PLAYER_PREFLIGHT.md).

## Namespace курсора SWF5 (v0.662)

Проигрыватель теперь устанавливает Mouse для SWF5–8 по прежнему контракту
[fvm_standard_mouse_install](../include/flashvm/mouse.h#L50). Mouse.hide/show возвращают предыдущий запрос
видимости. addListener/removeListener для загруженного SWF5 остаются неподдержанными:
наличие namespace не обходит версионный запрет. SWF1–4 идут прежним путём.
Политика SDL основного потока и восстановление при закрытии не меняются.

flashvm_player_legacy_cursor_interpreter/jit исполняют настоящие SWF5–8,
проверяя hide/hide/show/show → 1/0/0/1. Заведомо неверные значения ошибочны;
отдельный listener-вызов должен отказать в SWF5 и пройти в SWF6–8. Dummy video
проверяет скриптовую семантику, не физический курсор и не полный Flash API.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../include/flashvm/mouse.h#L50) |
<!-- source-links:end -->
