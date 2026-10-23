# Ограниченные обводки SWF 8 LINESTYLE2

[fvm_swf_read_shape](../src/swf/shape.cpp#L470) принимает LINESTYLE2 в DefineShape4 с круглыми концами и
соединениями, обычным масштабированием ширины и необязательным NoClose. Ширина
задана беззнаковым числом твипов; ноль включает однопиксельную hairline-обводку.
Принимаются прямой RGBA-цвет и FillType со сплошной, линейной, радиальной или
фокусной заливкой. Общий декодер градиентов принимает 1–15 упорядоченных RGBA-точек,
pad/reflect/repeat, RGB/linearRGB и знаковый внутренний FIXED8-фокус.

Пять резервных битов и резервный код концов/соединений 3 возвращают
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Другие концы/соединения, NoHScale, NoVScale, PixelHinting
и bitmap-заливки обводок возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Границы фокуса пока не
поддержаны. Усечённые поддержанные записи возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45).
При неподдержанном заголовке остаток записи может не проверяться.

NoClose подавляет автоматическую команду CLOSE при возвращении обводки в
начальную точку: остаются концы с caps. Явные move/style-разрывы сохраняются.
Флаг преобразуется в собственные команды пути и не требует нового поля
управляемого графа. Контуры заливки он не изменяет.

[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) связывает градиент обводки с lineGradient вместо gradient.
Описания заливки и обводки независимы; граф владеет точками и командами. Байты
файла и указатели бэкенда не сохраняются. Нативные барьеры защищают построение;
сериализация, каналы и GC используют обычные рёбра графа. Ошибка декодирования
сохраняет прежний результат; ошибка загрузки не публикует root. Отрисовка всей
сцены не является общей транзакцией.

Ширина использует существующий масштаб оси X и hairline-политику. Полное
поведение Flash при неравномерных/скошенных трансформациях и эталонное округление
остаются открытыми. Это расширение не завершает LINESTYLE2, DefineShape4, SWF8
или этап плана. Приёмка устройств и платформ учитывается отдельно.

Первоисточник: Adobe SWF specification, LINESTYLE2, печатные страницы 124–125:
https://open-flash.github.io/mirrors/swf-spec-19.pdf

`tests/swf_shape4_stroke.cpp` строит байты LINESTYLE2, загружает настоящие
персонажи сцены и сравнивает всю поверхность с публичными C89 API обводок для
24 сочетаний типа и режимов.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:470](../src/swf/shape.cpp#L470) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
