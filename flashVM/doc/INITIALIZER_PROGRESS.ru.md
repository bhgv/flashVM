# Прогресс и бюджет инициализации — v0.632

Сохраняется отложенная политика корневого DoInitAction SWF6–8 из
[SWF_INIT_ACTION.md](SWF_INIT_ACTION.md): [`fvm_movieclip_load_swf`](../include/flashvm/movieclip.h#L43) проверяет/сохраняет
код без исполнения, а [`fvm_movieclip_advance`](../include/flashvm/movieclip.h#L66) запускает инициализаторы библиотеки
в исходном корне перед сценой/кадром, в том числе при первом advance общего потомка.
ActionCall по-прежнему исполняет только выбранные действия кадра и не запускает
инициализацию. Полная Player preload/sprite-ID семантика остаётся открытой.

Раньше каждый последующий tick копировал и оплачивал всю таблицу initializers даже
при завершённом cursor. Поэтому пустой stopped/однокадровый клип мог не уложиться
в обычные две единицы. Внутренний [`run_initializers`](../src/standard/movieclip.cpp#L2483) теперь готовит только оставшийся
суффикс; завершённая таблица не требует единиц копирования/входа и памяти для снимка кода.
Обычные расходы node/optional-handler и следующая работа сцены/кадра сохраняются.

Вся таблица ограничена отдельным [`FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS`](../src/standard/movieclip_constants.h#L25) (65536).
Библиотека, таблица и исходный корень должны принадлежать контексту; эти edges
проверяются и при завершённом cursor. Для зафиксированного размера N и cursor P < N
до первого исполнения копируются ordinal P..N-1. Дырка в оставшемся диапазоне даёт
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) до новых действий. Порядок добавления не влияет на запуск.
Завершённый префикс не копируется и не перепроверяется. Cursor на/после конца текущей
таблицы означает завершение; правка private metadata автоматически его не откатывает.

[`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9) списывает единицу за каждый копируемый pending ordinal,
затем единицу перед попыткой блока, плюс прежние checkpoints AVM1 (включая End).
Cursor попытки фиксируется до декодирования hex/исполнения, сохраняя прежнюю политику
at-most-once при ошибке. Неверный код, [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), исключение или лимит могут
привести к пропуску частично/ещё не исполненного attempted блока в следующий раз:
эффекты нельзя откатить. После очистки исключения host может вызвать advance для
непопробованного суффикса. Ошибка копирования или списания следующего entry оставляет
его непопробованным. Ошибка останавливает текущий tick до следующей сцены/кадра;
эффекты уже завершённой работы batch сохраняются.

Для K оставшихся блоков новая подготовка/вход требуют 2K native units и O(K) памяти,
плюс само исполнение. После завершения K=0. Единицы не являются миллисекундами;
нулевой instruction_limit отключает числовой предел, сохраняя polling отмены.
Checkpoint AVM1 может показать limit+1 для отклонённой попытки инструкции, тогда как
native work отказывает до увеличения на пределе. Общий побайтовый бюджет для hex
и всех операций с метаданными не добавляется.

Callback не меняет захваченный batch. Замена раннего/current слота не откатывает cursor;
добавленные ordinal после снимка ждут следующей проверки initializers. Её может
выполнить следующий tick или общий потомок в том же внешнем host-вызове: это
локальный снимок, а не замораживание всего вызова. Короткая/заменённая таблица
продолжает использовать монотонный cursor, а не становится новой библиотекой.
Флаг reentry подавляет повторную инициализацию во вложенном advance, но вложенный
вызов может делать обычный tick: это не общий timeline reentry lock. Флаг восстанавливается
при возврате, ошибке и OOM. Cursor/флаг — приватное состояние runtime и не сериализуются:
получающий контекст запускает переданную библиотеку заново.

[GC:] Внешний tick [`FVM_NativeCallGuard`](../src/core/native_call_guard.hpp#L9) сохраняет захваченный код и исходный root
при изменении таблицы. Явная сборка внутри native tick callback возвращает
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45); вне вызова реальный GC разрешён. Идентичности не переживают
вызов, указатели callbacks не передаются в графе; host регистрирует их на стороне
получателя. Concurrent-доступ, публичное редактирование cursor и новый C/wire ABI
не добавляются.

Шесть новых регрессий проверяют root/child и interpreter/native JIT, границы бюджета,
суффиксы, мутации/reentry/продолжение ошибок, owner/дырки/cap, уничтожение исходного
контекста после транспорта, traced GC и отказы выделения при частичной инициализации.
Прежние swf_init_action и swf_init_action_allocation сохраняют проверки parser/error/graph.
[C89-пример](../examples/embedding/movieclip/README.ru.md) загружает один End-only
initializer с бюджетом пять; два последующих ticks в одном batch занимают четыре.
[English contract and commands](INITIALIZER_PROGRESS.md).

Все девять стадий открыты. Это регрессии FlashVM, а не сертификация original Player,
полного lifecycle scheduler, устройств или других ОС.

## Validation / Проверки v0.632

204 selected Linux Release CTests passed with AGG/bundled static dependencies;
147 minimal whole-core ASan/UBSan CTests passed serially, with LeakSanitizer disabled
and no raster backend. Six new initializer regressions and the existing init/action,
frame/graph/input/timer/native-JIT tests passed. The tutorial compiled under strict
C89/pedantic-errors, linked to the instrumented core and printed six expected lines.
Source-link, deterministic documentation and debugger-example checks passed.

A source-linked control using exact v0.631 movieclip.cpp fails an empty tick at
limit 2 after three initializers have already completed. The new code succeeds
at exactly two units, with the same three one-shot observations. This compares
FlashVM implementations; it is not original-Player or foreign/device acceptance.

Проверены Linux Release и минимальное ASan/UBSan-ядро; отключённый LeakSanitizer,
отсутствующий в нём raster backend и неисполненные SDL/foreign/device прогоны не
выдаются за дополнительную приёмку. Контроль старого кода воспроизводит расход
на завершённую таблицу; новый сохраняет три однократных эффекта и завершает tick
ровно за две единицы. Все девять стадий остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_load_swf` | function | [include/flashvm/movieclip.h:43](../include/flashvm/movieclip.h#L43) |
| `FVM_MOVIECLIP_MAX_INITIALIZER_BLOCKS` | macro / define | [src/standard/movieclip_constants.h:25](../src/standard/movieclip_constants.h#L25) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
