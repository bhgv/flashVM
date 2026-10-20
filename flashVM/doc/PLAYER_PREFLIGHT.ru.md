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
## Явное отключение корневого потока (v0.663)

Root streaming sound проверяется до создания окна/устройства. Без --mute фильм
с корневым потоком явно сообщает о неподдержанном воспроизведении. С --mute
проигрыватель использует новый [fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L90) и
[FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../include/flashvm/movieclip.h#L82): проверяет и пропускает только корневые потоковые
управления. Прежний [fvm_movieclip_load_swf](../include/flashvm/movieclip.h#L43) остаётся строгим.

Декодированный вход opt-in должен побайтно совпасть с текущим источником
[fvm_load_memory](../include/flashvm/flashvm.h#L72). Поэтому порядок/версия заголовков, кадры, дубликаты и PCM alignment
проверяются для точно импортируемого root. Неизвестные флаги ошибочны; при отказе
выход null. Нет действий скрипта и устройств. Хост обязан укоренить клип/библиотеку;
ограничения потока владельца/GC сохраняются. Сжатый звук остаётся непрозрачным:
MP3-декодер и A/V sync не добавлены. Sprite streams ещё не поддержаны. Event sound
сохраняет отдельный callback хоста; --mute ставит прежний muted callback.

Тесты: FWS/CWS PCM, непрозрачный MP3, смесь event/stream, неверные заголовки,
orphan/duplicate/odd PCM, строгий и opt-in импорт, другой источник, неизвестные
флаги, отказ вложенному потоку и укоренённый клип через GC/advance.
## Полный вход измеренного размера (v0.664)

После чтения измеренного размера хост проверяет EOF. Рост файла ошибочен вместо
проигрывания префикса; короткое чтение, ошибка EOF-probe и закрытия также отменяют
загрузку. Перед probe очищается старая посторонняя ошибка SDL. Каждый поток
закрывается один раз; отказ сохраняет выходной размер и освобождает буфер.
Прежний предел 64 МиБ и требование положительного/seekable входа сохраняются.
Изменение длины обнаруживается, перезапись той же длины — не гарантированно:
файл должен быть стабилен. UTF-8-доступ к файлам остаётся в SDL.

Фикстура подменяет открытие настоящим memory stream SDL и управляет сообщаемым
размером/ошибками. Проверяются EOF, старая ошибка, рост, усечение, неверные размеры,
отказ открытия/probe/close и cleanup без создания окна или устройства.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../include/flashvm/movieclip.h#L43) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:90](../include/flashvm/movieclip.h#L90) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../include/flashvm/mouse.h#L50) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:82](../include/flashvm/movieclip.h#L82) |
<!-- source-links:end -->
