# Подчёркивание по метрикам шрифта

[`fvm_font_underline_metrics`](../include/flashvm/font.h#L63) копирует центр и полную толщину подчёркивания
масштабируемого шрифта в пикселях без хинтинга; положительная y направлена вниз
от базовой линии. FreeType нормализует метрики исходного формата:
[руководство](https://freetype.org/freetype2/docs/tutorial/step2.html) описывает
центр подчёркивания и масштабирование единиц шрифта. Толщина не подбирается.

Аргументы: заимствованный шрифт и обязательный выход [`FVM_FontUnderlineMetrics`](../include/flashvm/font.h#L32).
[FVM_OK](../include/flashvm/flashvm.h#L42) публикует числа, которые переживают шрифт. При ошибке выход обнуляется:
отсутствующий выход/шрифт с поддержкой шрифтов даёт INVALID_ARGUMENT;
отсутствующие или неположительные масштабированные метрики — UNSUPPORTED.
NO_FONTS отклоняет отсутствующий выход, иначе обнуляет его и даёт UNSUPPORTED.
Запрос не выделяет память и не обращается к VM/GC. Общий изменяемый шрифт
требует синхронизации вызывающего; независимые шрифты и скопированные числа
могут использоваться независимыми потоками.

`font_underline.c` использует CC0-шрифт с 1000 единицами/em и явной верхней
границей TrueType post -75 при толщине 50. Нормализованный центр FreeType
равен -100: при размере 40 пикселей центр y-down равен 4, толщина 2.
Три размера проверяются по фиксированным аналитическим значениям. Контуры,
advances и подстановки глифов не изменены. Это нативная проверка метрик,
а не полная приёмка Flash-текста или устройств/платформ.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_font_underline_metrics` | function | [include/flashvm/font.h:63](../include/flashvm/font.h#L63) |
| `FVM_FontUnderlineMetrics` | struct | [include/flashvm/font.h:32](../include/flashvm/font.h#L32) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
