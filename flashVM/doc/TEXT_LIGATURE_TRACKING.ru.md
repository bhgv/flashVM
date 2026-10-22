# Необязательные лигатуры и трекинг

v0.861 добавляет [FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES](../include/flashvm/font.h#L10) в [fvm_font_shape_configured](../include/flashvm/font.h#L90).
Флаг отключает OpenType liga и clig во всём входном run. Обязательные rlig,
соединение букв и позиционирование диакритики сохраняют настройки по умолчанию.
[FVM_FONT_SHAPE_NO_KERNING](../include/flashvm/font.h#L9) независим; побитовое OR выбирает обе политики.
Нулевые флаги и [fvm_font_shape](../include/flashvm/font.h#L76) сохраняют прежнюю раскладку. Флаги действуют на
один вызов и не меняют постоянное состояние шрифта. Это управление OpenType,
а не обещание разделить любой кластер письма или любую технологию шрифтов.

API заимствует font/text/language только на время вызова, возвращает принадлежащий
вызывающему [FVM_TextRun](../src/capi/font_c.cpp#L75); освобождение — [fvm_text_run_destroy](../include/flashvm/font.h#L94). Для экспорта через
[fvm_text_run_path](../include/flashvm/font.h#L98) нужен тот же шрифт/размер. Изменяемый font handle требует
сериализации вызывающим; независимые handles допускают разные потоки. VM, GC,
callbacks и устройства не используются. Неизвестные флаги, неверные UTF-8,
direction/input дают INVALID_ARGUMENT и очищают допустимый выходной указатель.
Ошибка выделения памяти — OUT_OF_MEMORY. NO_FONTS проверяет представление флагов
и direction, затем возвращает UNSUPPORTED для допустимых флагов. Прежние границы
раскладки и координат сохраняются.

Исходный CC0-шрифт даёт при 40px fi=28px с лигатурой и f+i=24px без неё,
AV=44.8px с кернингом и 48px без него. Проверяются четыре сочетания флагов
и сохранение лигатур в последующих вызовах прежнего API.

[Документация HarfBuzz](https://harfbuzz.github.io/shaping-opentype-features.html)
описывает управление features во всём буфере. Проверки эталонного Flash и реальных
платформ/устройств учитываются отдельно; все стадии roadmap открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_shape` | function | [include/flashvm/font.h:76](../include/flashvm/font.h#L76) |
| `fvm_font_shape_configured` | function | [include/flashvm/font.h:90](../include/flashvm/font.h#L90) |
| `FVM_FONT_SHAPE_NO_KERNING` | macro / define | [include/flashvm/font.h:9](../include/flashvm/font.h#L9) |
| `FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES` | macro / define | [include/flashvm/font.h:10](../include/flashvm/font.h#L10) |
| `fvm_text_run_destroy` | function | [include/flashvm/font.h:94](../include/flashvm/font.h#L94) |
| `fvm_text_run_path` | function | [include/flashvm/font.h:98](../include/flashvm/font.h#L98) |
| `FVM_TextRun` | struct | [src/capi/font_c.cpp:75](../src/capi/font_c.cpp#L75) |
<!-- source-links:end -->
