# Совместимость и предварительная проверка входа проигрывателя

[English](PLAYER_PREFLIGHT.md).

Текущая политика v0.758: [один root PCM stream воспроизводится](PLAYER_PCM_STREAMS.ru.md); остальные кодеки, смесь и несколько потоков отклоняются до native setup. Ниже сохранён исходный контракт muted import.

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

Root streaming sound проверяется до создания окна/устройства. Изначально без --mute
все root streams отклонялись; v0.758 разрешает один PCM stream по политике выше. С --mute
проигрыватель использует новый [fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L151) и
[FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../include/flashvm/movieclip.h#L141): проверяет и пропускает только корневые потоковые
управления. Прежний [fvm_movieclip_load_swf](../include/flashvm/movieclip.h#L102) остаётся строгим.

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
## Числовые опции без зависимости от locale (v0.665)

ASCII-парсер --fps/--speed больше не вызывает strtod и не меняет locale процесса.
C-хост может вызвать его после установки десятичной запятой: `--fps 12.5`
продолжит работать без влияния на locale других потоков. Поддержаны точка,
экспонента e/E и завершающий % только для speed. Знак разрешён только у экспоненты.
Предел текста — 128 байтов, модуля экспоненты — 400; неверный синтаксис и прежние
границы FPS/процента проверяются. Пересчёт использует конечную double-арифметику
представления, не точную десятичную арифметику и не ActionScript ToNumber.
Очень длинные аргументы не принимаются. Лимит кадров остаётся проверяемым unsigned.

Автономный тест подменяет strtod/setlocale ошибочными реализациями и проверяет,
что они не вызываются; затем проверяет формы чисел, запятую/мусор, границы и %.
Это детерминированный тест зависимостей/грамматики, а не новый запуск установленной
чужой locale или каждого возможного floating-point процессора.
## Подробности C-интерфейса и пример (v0.666)

[fvm_movieclip_load_swf_ex](../include/flashvm/movieclip.h#L151) получает контекст владельца, заимствованные полные
FWS/CWS-байты и точную длину, имя клипа, флаги и обязательный указатель результата.
Сначала установите стандартную модель объектов. Вход/имя нужны только до возврата;
импортированное хранилище принадлежит контексту. Объект — заимствованная управляемая
память, хост обязан укоренить его до сборки. Вызов синхронный в потоке владельца,
вне GC/teardown; скриптовых/device callback нет, канонический SWF не меняется.
Нулевые флаги сохраняют строгий режим прежнего API.

[FVM_OK](../include/flashvm/flashvm.h#L42) выдаёт клип; неверный вход, другой канонический источник или неизвестные
флаги дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); неподдержанные scene features, включая sprite
streams, — [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46); отказ памяти — [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44). Если out=null,
очищать нечего; иначе любой отказ оставляет *out=null. Сжатые блоки семантически
не декодируются и не сертифицируются. Владение event-sound callback и timing прежние.

[Исполняемый C89/GC и SWF8 пример](../examples/player/muted_stream/README.ru.md)
показывает setup, точный источник, освобождение входа, pin/GC/advance и cleanup.
Ожидается `streams=1 advanced=1 muted=1`. C-хост собирается без SDL, отдельная
команда player показывает сцену с visible/dummy video. Генератор автономен.

Проверки используют Linux x86-64/GCC 13.3 и внешний SDL2 2.30.12. Запускаются
затронутые проверки player и выбранные sound/scene/stream тесты, не полный core
или sanitizer suite. Полный AGG/JIT player с отдельными целевыми SDL SDK слинкован
для Linux ARM64, Linux RISC-V64 и Windows x64 прежней документированной оболочкой
Zig. Это cross-сборки, не запуск на чужих ОС/устройствах. Android/macOS, слышимый
потоковый звук, A/V sync, физические устройства, длительная нагрузка и все девять
стадий ещё открыты. [Сборка](../../BUILDING.ru.md),
[происхождение SDK](PLAYER_RECOVERY.ru.md#проверки-серии).
Пройдены 59 выбранных нативных тестов: 44 frontend/player и 15 sound/scene/stream.
Новый импорт проверяется на каждом отказе new/new[]; collector/release/teardown
отвергают вход до временных выделений декодера.
Автономный C89-хост muted import также слинкован для всех трёх целей; проверены
ELF/PE machine headers. Исполнялся он только на нативном Linux.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:102](../include/flashvm/movieclip.h#L102) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:151](../include/flashvm/movieclip.h#L151) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:50](../include/flashvm/mouse.h#L50) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:141](../include/flashvm/movieclip.h#L141) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
