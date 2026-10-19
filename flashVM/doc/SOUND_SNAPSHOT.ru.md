# Снимки звуковых событий корня — v0.637

[English](SOUND_SNAPSHOT.md). Дополнение к [доставке звука MovieClip](MOVIECLIP_SOUND.ru.md).
[`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) доставляет root StartSound через
[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21); изменения не открывают аудиоустройство.

Private `sounds` кадра — плотный список от нуля, ограниченный
[`FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS`](../src/standard/movieclip_sound_constants.h#L8) (65536). Снимок скалярных индексов читается
по ordinal, независимо от порядка вставки в hash table. Каждое значение должно быть
целым неотрицательным Number ниже прежней границы, вычисленной из размера source.
Пропуски, неверные типы, NaN/infinity, дробные/выходящие за пределы числа и слишком
большой список дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до первого host callback. Усечённый
плотный префикс структурно допустим в этой версии; принадлежность исходному кадру,
полнота и уникальность индексов этим ещё не доказываются.

Весь native vector готовится до внешних эффектов. Поздняя ошибочная запись больше
не обнаруживается после уже доставленного раннего события. OOM снимка даёт
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) без доставки этого пакета. После начала callbacks прежние
эффекты сохраняются при последующей ошибке. Приоритет сравнения source и политики
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) при отсутствии callback сохраняется. Пустой допустимый список
при зарегистрированном callback ничего не доставляет. Сравнение полного hex-source
по-прежнему линейно по размеру файла.

[GC:] Действующий native barrier tick удерживает managed metadata/source во время
синхронной доставки. Снимок хранит только скопированные числовые индексы и
освобождается при любом выходе. Busy guard блокирует замену callback/source и
повторный advance, восстанавливаясь при ошибках и исключениях. Host callback/userdata
заимствованы и должны жить до отмены регистрации или уничтожения контекста.
Контракт callback по-прежнему запрещает правку clip/frame data и исполнение script.
Тесты намеренно повреждают private metadata для проверки устойчивости; это не
разрешение таких операций в host callback.

Вход root кадра помечается выполненным до подготовки звука. Ошибка capture/source/
callback/OOM останавливает его поздние actions, не откатывая сцену и не повторяя
звук автоматически. Остановленный кадр остаётся выполненным; играющий timeline
может перейти к другому кадру. Для повтора нужен явный host seek/recovery.
Интерпретатор и native JIT используют один путь доставки.

Новые проверки: `sound_snapshot_order` (SWF6–8, порядок вставки, настоящий native
JIT, защитный снимок при изменении полей/списка, эффекты до ошибки),
`sound_snapshot_invalid` (полная локальная структурная/числовая предвалидация),
`sound_snapshot_allocation` (все границы allocations, GC/guards, счётчик выделений).
Прежние sound delivery/validation/transport/allocation tests и
[C89 mixer tutorial](../examples/multimedia/swf_event_mixer/README.md) сохраняют
значение; пример использует публичные API и не изменяет metadata.

Все стадии роудмапа открыты. Это не подтверждает scheduling оригинального Player,
аудиоустройство, другие ОС или полную совместимость SWF8.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS` | macro / define | [src/standard/movieclip_sound_constants.h:8](../src/standard/movieclip_sound_constants.h#L8) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
