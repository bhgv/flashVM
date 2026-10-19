# Явная доставка ClipActions в C89

[English](README.md). Host загружает встроенный SWF8 sprite с двумя authored records:
EnterFrame записывает `seen=17`, Load — `seen=42`. Обычный tick размещает и продвигает
sprite, затем host выбирает actor, явно доставляет Load и проверяет marker.
Окно, OS input, компилятор, внешний файл и SDL не нужны.

## Сборка и запуск

Из корня независимого компонента FlashVM:

```sh
cmake -S . -B build-clips -DDEFS=INTERPRETER -DFVM_BUILD_EXAMPLES=ON
cmake --build build-clips --target flashvm_examples_embedding_clip_actions_main
./build-clips/flashvm_examples_embedding_clip_actions_main
```

Ожидаются exit code 0 и:

```text
explicit Load marker: 42
```

[main.c](main.c) — самостоятельный C89 host. [clip_action_movie.h](clip_action_movie.h)
содержит независимо закодированные immutable wire bytes, не текстовую AS-программу
и не borrowed resource handle. Loader копирует records. Host владеет runtime/context
и pin корня, удерживающим actor/library/code. Context destruction освобождает граф
и pin до конца жизни локальных данных. После cleanup указатели не используются.

## Контракты API

[Подробный контракт](../../../doc/CLIP_ACTIONS.ru.md). Все вызовы — в потоке владельца.
Доставка синхронная, ошибки сохраняют уже выполненные эффекты. Native barrier
блокирует сборку во время исполнения; GC выполняется на host safe points. Явный Load
повторяем: host отвечает за once-only lifecycle. Native onLoad этим API не вызывается.

| API | Назначение и владение |
| --- | --- |
| [fvm_config_init](../../../include/flashvm/flashvm.h#L62) | Инициализировать ABI host-конфигурации до создания runtime. |
| [fvm_runtime_create](../../../include/flashvm/flashvm.h#L64) | Скопировать конфигурацию и получить внешнего владельца; без worker/устройства. |
| [fvm_context_create](../../../include/flashvm/flashvm.h#L68) | Создать managed heap; дальнейшие вызовы — в её потоке владельца. |
| [fvm_standard_install](../../../include/flashvm/standard.h#L20) | Установить классы/методы до создания загруженной сцены. |
| [fvm_movieclip_load_swf](../../../include/flashvm/movieclip.h#L43) | Скопировать/проверить SWF и сохранить private handlers; root заимствован, scripts при загрузке не исполняются. |
| [fvm_gc_pin](../../../include/flashvm/gc.h#L48) | Удержать root/actor/library между вызовами; pin примера освобождает context_destroy. |
| [fvm_movieclip_advance](../../../include/flashvm/movieclip.h#L62) | Выполнить tick, разместить actor и доставить EnterFrame; Load автоматически не доставляется. |
| [fvm_object_get](../../../include/flashvm/value.h#L72) | Синхронно прочитать actor и scalar marker; borrowed object остаётся достижимым через root. |
| [fvm_movieclip_dispatch_clip_actions](../../../include/flashvm/clip_actions.h#L37) | Исполнить retained Load bytecode с receiver actor и key_code ноль; без выбора цели, native onX, продвижения кадра или показа. |
| [fvm_context_destroy](../../../include/flashvm/flashvm.h#L70) | Освободить граф/registration/pin после завершения вызовов; managed pointers недействительны. |
| [fvm_runtime_destroy](../../../include/flashvm/flashvm.h#L66) | Освободить внешнего владельца после контекста. |

[FVM_CLIP_EVENT_LOAD](../../../include/flashvm/clip_actions.h#L5) выбирает raw authored event. Остальные flags и KeyPress selectors
описаны в заголовке и руководстве. Проверяется одна выбранная запись, не планирование
оригинального Player, сеть, hit testing, видимый вывод или приёмка чужой платформы.
При ошибке автоматического retry нет. [Каталог](../../README.md#example-embedding-clip_actions).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_EVENT_LOAD` | macro / define | [include/flashvm/clip_actions.h:5](../../../include/flashvm/clip_actions.h#L5) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../../../include/flashvm/gc.h#L48) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:62](../../../include/flashvm/movieclip.h#L62) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:37](../../../include/flashvm/clip_actions.h#L37) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../../../include/flashvm/movieclip.h#L43) |
| `fvm_object_get` | function | [include/flashvm/value.h:72](../../../include/flashvm/value.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../../../include/flashvm/standard.h#L20) |
<!-- source-links:end -->
