# Root DoInitAction / Корневой DoInitAction

v0.223 accepts root DoInitAction in SWF6–8 for a previously defined sprite ID,
validates the bounded bytecode and retains owned blocks in source order. The
source library retains its root clip. On first advance, blocks run on that source
root before display placement and ordinary frame actions, including when a child
clip is advanced first. This is deferred initialization after host setup, rather
than execution during parsing/preloading. Nested/imported-movie chains and full
streaming/preload timing remain open.

Корневой DoInitAction SWF6–8 для ранее определённого спрайта проверяется и
сохраняется в библиотеке. При первом advance блоки запускаются в исходном
корневом клипе перед сценой и действиями кадра, в том числе при первом advance
дочернего клипа. Запуск отложен до advance после настройки host; исполнение
при разборе/preload, цепочки импортированных фильмов и streaming остаются открытыми.

Progress and the reentry guard are private per-library object state, initialized
to zero/false in new objects and not serialized. Shared descendants initialize
once; transferred library graphs initialize afresh in the receiving context.
A failed block is attempted at most once; its side effects cannot be rolled back.
After clearing an exception, a later advance can process remaining blocks. This
is an explicit error policy, not a claim of complete Flash Player retry behavior.
The guard is restored on exception/OOM; callbacks can reenter without recursively
running the same initializer. Existing host clips and older graphs without init
metadata keep their previous advancement path.

Прогресс и защита повторного входа — приватное состояние объекта библиотеки,
которое не сериализуется. Общая библиотека инициализируется один раз; переданный
граф запускает инициализацию заново в другом контексте. Блок с ошибкой повторно
не выполняется: побочные эффекты нельзя откатить. После очистки исключения
следующий advance продолжает оставшиеся блоки. Это явная политика ошибок,
а не подтверждение полной семантики Player. Host-клипы и старые графы без
initializer-метаданных сохраняют прежний путь advance.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/display_object/movie_clip.rs do_init_action and
core/src/avm1/runtime.rs run_stack_frame_for_init_action: initialization runs with
the importer clip as target/this, separately from normal frame actions.
Ruffle itself notes incomplete once/sprite-ID semantics; this implementation's
policies above are not presented as independently certified Player behavior.
The existing SWF4/5 and nested-sprite tag policies are unchanged.

swf_init_action checks source order with 20 blocks, frame observation, repeated
advance, transfer+GC, exception continuation, native reentry, source-root scope
when advancing a child first, unknown sprite IDs and truncated tag payloads.
swf_init_action_allocation injects allocation failure through first advance and
checks initializer guards/progress, native/script depth, this/callee/root frames,
then safe subsequent advance and collection. Library/root cycles use existing
graph serialization; public ABI and the wire schema are unchanged. Full AS2
class compatibility and completion of any roadmap stage remain open.

Тесты покрывают порядок 20 блоков, кадры, повторный запуск, перенос/GC, throw,
повторный вход, область исходного корня и отказы выделения памяти. Полная
совместимость AS2-классов и завершение стадии не заявляются.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 382 tests passed
(after eleven execution-permission retries), Cairo Release 381 passed without
retries. Nine selected timeline/sprite/transfer tests passed in NO_OS_PORTS and
heap/global ASan + UBSan; stack/leak checks remain disabled. All 17 unchanged
selected original SWF<=8 probes retained byte-exact traces and clean exits.
A separate staged probe confirms original AS2 v8 load_memory, standard install
and movieclip_load now succeed; advance still returns UNSUPPORTED before its
expected trace. It remains a failing reference test, as do the other this/AS2
probes; no full class compatibility or non-Linux runtime certification is claimed.

Subsequent script-prototype inheritance permits original AS2 v6/v8 execution
to finish; trace differences remain. See [AVM1_FUNCTION_OBJECT_PROTOTYPE.md](AVM1_FUNCTION_OBJECT_PROTOTYPE.md).
Последующая правка наследования позволяет закончить исполнение AS2 v6/v8;
отличия трасс от эталона ещё остаются.
