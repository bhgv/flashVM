# SWF scene controls / Команды SWF-сцены

Development checkpoint v0.28; roadmap stage 3 remains open.

The MovieClip importer decodes PlaceObject, PlaceObject2, RemoveObject and
RemoveObject2 for previously defined sprite resources. Optional matrix, color,
ratio and name fields retain their presence flags. A move updates supplied display fields; names are assigned only on initial
placement and ignored by subsequent Modify/Replace controls. A Place creates an instance; Replace on an existing MovieClip updates
its pose while retaining its script identity and original sprite timeline.
Replace at an empty depth does nothing. Static-art replacement is not implemented
by this sprite-only checkpoint.

Raw SWF depth 1 maps to ActionScript depth -16383. Placement controls run before
frame action blocks; new children are advanced after the parent's action blocks.
Seeking composes the target display state without executing skipped actions or
instantiating intermediate children. Matching placement identities are reused,
including a surviving instance when the timeline loops. A one-frame parent runs
its frame actions once while its children and onEnterFrame continue to advance.
Replacing its current frame with the host API schedules that frame once again.

Matrices retain skew, reflection and zero scale. The new C89 functions
[`fvm_movieclip_get_matrix`](../include/flashvm/movieclip.h#L105) and [`fvm_movieclip_set_matrix`](../include/flashvm/movieclip.h#L107) access the local transform
in pixels. Setting a finite representable transform publishes all components
atomically, including under allocation failure. SWF translations convert twips
to pixels; rotation/skew fields use signed 16.16 values. Color transforms compose
through the parent hierarchy and affect the existing solid-vector renderer in
both bundled AGG and Cairo. Public `_alpha` supplies the local alpha multiplier.

Controls, definitions, effective color and affine state are managed graph edges
or scalars. They survive queue/file transport and graph snapshots, including the
current scene frame and child identities. Native barriers protect temporary
descriptors during constructors. Loading runs no scripts and leaves `out` null
on failure. Scene advancement is not a transaction over user callbacks or a
whole frame: an allocation error can leave already published preceding controls.
The caller must handle the returned error; this checkpoint does not promise
rollback of script effects.

## Проверенное поведение

Импортёр читает размещение и удаление ранее объявленных DefineSprite. Флаги
присутствия матрицы, цвета, имени и ratio сохраняются: перемещение не сбрасывает
пропущенные поля. Имя задаётся при создании; последующие команды его не меняют. Place создаёт клип; Replace существующего MovieClip меняет
параметры отображения, сохраняя экземпляр и его исходный таймлайн. На пустой
глубине Replace ничего не создаёт. Замена статической графики пока не реализована.

Переход по кадрам собирает конечную сцену без исполнения пропущенных сценариев
и создания промежуточных экземпляров. Совпавший экземпляр сохраняется, в том
числе при зацикливании. Однокадровый родитель исполняет кадровые действия один
раз; его дети и onEnterFrame продолжают работать. Обновление текущего кадра через
API хоста назначает однократное исполнение нового сценария.

Матрицы сохраняют перекос, отражение и нулевой масштаб; C89 API читает и атомарно
задаёт локальную матрицу. Цвет преобразуется с учётом родителей при отрисовке
AGG/Cairo. Команды, ресурсы и состояние продолжают работать после переноса через
очередь, файл и снимок, включая сборку мусора. Отказ памяти при загрузке оставляет
нулевой результат; обновление целой сцены не является атомарной транзакцией и
может сохранить уже выполненные команды. Откат побочных эффектов не обещается.

## Tests and limits / Тесты и ограничения

`tests/swf_scene.cpp` constructs real FWS records and checks moves, sprite Replace,
removal, seeks, loop retention, one-frame parents, nested transforms, RGBA pixels,
queue/file/snapshot restoration and GC. It covers signed 32-bit bit fields,
truncated matrices/strings, invalid depths/characters, trailing bytes and rejected
unsupported fields. `tests/swf_scene_allocation.cpp` injects failure at every
allocation boundary until load/advance succeeds, checks reclamation after each
case, and verifies atomic local-matrix updates.

Shape/bitmap/text/button definitions, ClipDepth masks, ClipActions, PlaceObject3,
filters, blend modes, audio/video, loading and full input remain open. These
unimplemented tags/fields return UNSUPPORTED rather than silently disappearing.
Instance-name generation, callback scheduling, reentrant gotos, detailed rewind
rules, resource budgets and Flash version differences still need reference-player
fixtures. Synthetic tests and comparison with an independent implementation do
not certify complete Flash compatibility or non-Linux platform operation.

Не реализованы остальные типы ресурсов, маски ClipDepth, ClipActions,
PlaceObject3, фильтры, режимы смешивания, звук/видео и полная модель загрузки/ввода.
Полная сверка переходов, имён, порядка обратных вызовов, лимитов и различий версий
ещё требуется. Этапы 3 и 4 не завершены.

Format reference: [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).
Independent implementation cross-check:
[Ruffle MovieClip](https://github.com/ruffle-rs/ruffle/blob/master/core/src/display_object/movie_clip.rs)
and [display-object replacement](https://github.com/ruffle-rs/ruffle/blob/master/core/src/display_object.rs).
Upstream code was not copied into FlashVM.

## Validation v0.28 / Проверки v0.28

Strict C89/C++98 Linux profiles: headless 177/177, bundled AGG 187/187,
bundled Cairo 187/187, portable/no-OS 168/168. The headless and portable final
runs needed executable-permission repairs and successful reruns of the affected
programs; the original failures and rerun logs remain separate validation files.
The sanitizer driver completed 174 tests/examples after the final semantic fixes.
Heap/global ASan and UBSan were enabled; all ASan stack checks were explicitly
disabled for the workspace unwind environment, LeakSanitizer was disabled because
/proc is unavailable, and container exception tests use UBSan only.
The first-party comment audit covers 236 files with no missing function/member
or tutorial-line comments; the direct-GC audit found no missing tagged comments.
This evidence does not close any roadmap stage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_get_matrix` | function | [include/flashvm/movieclip.h:105](../include/flashvm/movieclip.h#L105) |
| `fvm_movieclip_set_matrix` | function | [include/flashvm/movieclip.h:107](../include/flashvm/movieclip.h#L107) |
<!-- source-links:end -->
