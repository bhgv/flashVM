Текущая политика плеера: [воспроизведение одного root PCM](../../../doc/PLAYER_PCM_STREAMS.ru.md). Отдельный C89-хост ниже по-прежнему явно отключает звук.

# Явно отключённый корневой звуковой поток

Готовый [SWF8](muted-stream.swf) содержит сине-серый кадр 320×200 и три mono PCM
16-bit отсчёта в SoundStreamHead2/SoundStreamBlock. Это маленький пример формата
и импорта, не демонстрация слышимого звука или синхронизации. Автономный
[Python-генератор](make_movie.py) воспроизводит байты без компиляторов/тестов.
[C89-хост](main.c), [границы](bounds.h),
[контракт импорта](../../../doc/PLAYER_PREFLIGHT.ru.md#явное-отключение-корневого-потока-v0663).

Из корня репозитория с собранным SDL player:

```sh
python3 flashVM/examples/player/muted_stream/make_movie.py
build/player/frontend/flashvm_player --interpreter --mute --frames 3 \
  flashVM/examples/player/muted_stream/muted-stream.swf
```

--jit допустим при доступном backend. С --mute сцена играет без звука; без него
с v0.758 этот одиночный PCM stream также воспроизводится. Три отсчёта слишком
коротки для слышимой демонстрации: используйте [длинный пример](../pcm_stream/README.ru.md). Для headless
задайте SDL_VIDEODRIVER=dummy и --fps 1000; это не физическая видео/аудиоприёмка.

C89-хост не требует SDL и не открывает устройство. Его stdio-чтение использует
кодировку имён файлов C runtime платформы; SDL player сохраняет UTF-8 адаптер:

```sh
cmake -S flashVM -B build/vm -DFVM_BUILD_EXAMPLES=ON
cmake --build build/vm --target flashvm_examples_player_muted_stream_main
build/vm/flashvm_examples_player_muted_stream_main \
  flashVM/examples/player/muted_stream/muted-stream.swf
```

Ожидается `streams=1 advanced=1 muted=1`. Хост читает стабильный seekable-файл,
держит вход до обоих load-вызовов, затем освобождает. Клип/библиотека укореняются,
выполняются GC-step и один advance; далее unpin, context destroy, runtime destroy.
Ошибки освобождают те же ресурсы. Код 0 — успех, 1 — I/O/API, 2 — использование.
Рисования здесь нет: показ обеспечивает команда player выше.
[Обычная/cross-сборка](../../../../BUILDING.ru.md): SDK обязан соответствовать
целевой ОС/CPU, исполняемый хост запускают на этой цели.

| API | Назначение и владение |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L70) | Инициализирует конфигурацию хоста без выделения runtime. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L74) | Создаёт принадлежащий хосту runtime из конфигурации; уничтожается последним. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L82) | Создаёт контекст потока владельца, уничтожаемый раньше runtime. |
| [fvm_load_memory](../../../include/flashvm/flashvm.h#L90) | Копирует канонический источник в контекст; вход остаётся у вызывающего. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L22) | Готовит стандартную модель MovieClip, без аудиоустройства. |
| [fvm_swf_stream_sound_count](../../../include/flashvm/swf_stream_sound.h#L61) | Проверяет root streams и копирует числовой счётчик. |
| [fvm_movieclip_load_swf_ex](../../../include/flashvm/movieclip.h#L194) | Явный [FVM_SWF_IMPORT_MUTE_ROOT_STREAMS](../../../include/flashvm/movieclip.h#L182); требуется совпадающий источник, выход при отказе null. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L60) / [fvm_gc_unpin](../../../include/flashvm/gc.h#L70) | Сохраняют/освобождают корень клипа/библиотеки через сборку; владение памятью не передают. |
| [fvm_gc_step](../../../include/flashvm/gc.h#L116) | Сборка в безопасной точке потока владельца, не внутри callback. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L160) | Исполняет поддержанный root-кадр с отключёнными stream controls. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L86) / [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L78) | Освобождают владение в порядке context, затем runtime. |

Строгий [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L129) по-прежнему отвергает stream controls. Неизвестные
флаги, другой источник, неверные записи root и sprite streams не игнорируются.
Сжатые блоки остаются непрозрачными: MP3 decode, stream mixing, A/V sync,
видеокодеки и полная семантика Flash sound не добавлены. Event sound сохраняет
отдельный callback хоста; в этой фикстуре его нет.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../../../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../../../include/flashvm/flashvm.h#L86) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:116](../../../include/flashvm/gc.h#L116) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:90](../../../include/flashvm/flashvm.h#L90) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:160](../../../include/flashvm/movieclip.h#L160) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:129](../../../include/flashvm/movieclip.h#L129) |
| `fvm_movieclip_load_swf_ex` | function | [include/flashvm/movieclip.h:194](../../../include/flashvm/movieclip.h#L194) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../../../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../../../include/flashvm/flashvm.h#L78) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:22](../../../include/flashvm/standard.h#L22) |
| `FVM_SWF_IMPORT_MUTE_ROOT_STREAMS` | macro / define | [include/flashvm/movieclip.h:182](../../../include/flashvm/movieclip.h#L182) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:61](../../../include/flashvm/swf_stream_sound.h#L61) |
<!-- source-links:end -->
