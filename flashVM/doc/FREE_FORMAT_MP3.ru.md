# Free-format MPEG Layer III в SWF

[English](FREE_FORMAT_MP3.md).

## Строгая разметка (v0.806)

Приватная [fvm_swf_mp3_probe](../src/swf/sound_mp3.cpp#L52) принимает нулевой индекс битрейта, если три
последовательных согласованных заголовка задают наименьшую подтверждённую длину пакета с позиции
ноль. Мусор перед пакетом не пропускается. Частота, число отсчётов и каналов
должны совпадать; padding добавляет байт отдельно для каждого пакета. Длина
строго меньше 2304 байт согласно закреплённому minimp3. Повреждённые заголовки дают
INVALID_ARGUMENT, недостаток данных — UNSUPPORTED. Ошибки сохраняют выходные
метаданные. Нет выделений, устройств, VM и callbacks.

Оригинальные аналитические сигналы закодированы системным LAME 3.100 в режиме
free-format; независимые S16-эталоны получены libmpg123. Runtime использует
прежний статический скалярный minimp3 без новых библиотечных зависимостей.
Приёмка устройств учитывается отдельно.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:52](../src/swf/sound_mp3.cpp#L52) |
<!-- source-links:end -->
