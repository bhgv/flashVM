# MovieClip duplication / Дублирование MovieClip

Development checkpoint v0.26; roadmap stage 3 remains open.

## English

`clip.duplicateMovieClip(name, depth[, initObject])` creates a sibling under the
same parent. A root or detached clip has no destination parent and returns
undefined. Depths use the existing dynamic range 0–1048575; negative/bias rules
and complete SWF-version gating remain open. Occupied depths use the same
replacement operation as attachMovie and createEmptyMovieClip.

The duplicate starts playing at frame 1, regardless of the source frame or pause
state. It owns a new frame table while sharing immutable private frame records.
Host-defined frame replacements on either instance therefore do not mutate the
other table. Existing dynamic properties, script event-handler properties and
dynamically created children are not copied. Static display-list children and
clip-event handlers are not implemented by this action-timeline importer yet.

The currently supported display properties are copied: position, scale,
rotation, alpha, visibility, enabled and useHandCursor. Vector paths, paint style,
pen position and the active-path alias are cloned together. Later drawing or
clearing the original cannot change the duplicate, and drawing on the duplicate
continues its own active path. This uses the existing graph snapshot codec and
inherits its 4096-object/65536-entry/64-MiB limits; very large host drawings can
therefore be rejected. It does not implement bitmap fills, masks or filters.

Imported linkage names are stored as context-owned strings in private clip state.
The duplicate retains the resource library and chooses the current registered
class using the same context/transported-registry policy as attachMovie.
Enumerable initObject properties are applied after copying supported display
state and before the constructor runs. Primitive initObject values are ignored.
Constructors bind the new clip, observe its parent, and cannot replace the native
instance with a returned object. Errors and exceptions propagate; already
published initialization/constructor side effects are not rolled back.

[GC:] Native dispatch prevents collection of the unpublished instance and
temporary drawing snapshot. The new clip owns cloned drawing edges and its own
timeline table, while retaining the shared resource library. Constructor call
metadata uses a rooted scope that unwinds after exceptions. A snapshot preserves
the shared identity between the active path and its entry in the path table.
Temporary serialized bytes are released on both successful and failed restore.
The method callback was appended to the stable table; earlier IDs are unchanged.
Wire version 7 and ABI 3 remain unchanged, without forward-reader guarantees.

## Русский

Метод создаёт новый клип у того же родителя. Корневой или отсоединённый клип
возвращает undefined. Копия начинает проигрываться с кадра 1, даже если оригинал
остановлен на другом кадре. Таблица кадров у копии собственная; определения
кадров могут разделяться. Динамические переменные, свойства обработчиков событий
и динамически созданные дочерние клипы не копируются.

Копируются поддержанные параметры отображения и векторный рисунок. Рисунок
копируется глубоко, вместе со стилями и связью текущего пути с таблицей путей.
Продолжение рисования и clear у одного экземпляра не меняют другой. Для копии
геометрии используется существующий формат графов с его ограничениями размера;
это не обещание работы с произвольно большим рисунком.

У импортированных клипов сохраняются библиотека и linkage name. Реестр классов
выбирается так же, как при attachMovie. Начальные свойства применяются после
копирования параметров отображения, до вызова конструктора. Необъектный initObject
игнорируется. Конструктор получает новый клип как this и видит _parent; исключения
передаются вызывающему коду без отката уже опубликованных изменений.

[GC:] Барьер нативного вызова удерживает ещё не опубликованные объекты и снимок
рисунка. Управляемые связи клипа сохраняют собственную геометрию и общую библиотеку.
После очереди или файлового канала восстановленный клип снова можно дублировать
в контексте без установки стандартных глобалов.

## Tests / Тесты

- `tests/movieclip_duplicate.c`: independent drawing/active paths, equal AGG/Cairo
  output before mutation, display-property copying and initObject overrides,
  omitted dynamic properties/children, root handling, depth replacement,
  GC and queue/file restoration followed by another duplication.
- `tests/swf_sprite_resources.cpp`: duplication from frame 2 resets to frame 1;
  registered-class initialization and duplication after graph restoration.
- `tests/swf_sprite_allocation.cpp`: allocation failures through loading,
  attachment and duplication with a nonempty drawing; unpublished children and
  collector recovery are checked before the successful operation.

The commented AS tutorial is in `examples/actionscript/sprite_resources.as`.
It requires an external AS-to-SWF compiler; FlashVM has no AS text parser here.
The [published AS2 MovieClip reference](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html)
provides the documented method behavior. Current tests do not establish a full
reference-player match. Global duplicateMovieClip, AVM1 CloneSprite, all negative
depths/version rules, static display lists/events, loading, drag/input, masks,
text fields and the remaining scene functionality are still open.

## Validation v0.26 / Проверки v0.26

Strict C89/C++98 Linux CTest runs: headless 174/174, bundled AGG 184/184,
bundled Cairo 184/184, portable/no-OS 165/165; all passed without repeats.
Sanitized programs plus the independent TCP fixture passed 167 checks. The driver
then stopped when the workspace removed execute permission from the Error process
binary. After restoring permissions, the four remaining Error/object/file/network/
timezone fixtures completed successfully: 171 combined checks. The original driver
did not produce a final summary; the repair/resume record is in the separate
validation archive. This was a process-launch failure, not a sanitizer finding.
Heap/global ASan and UBSan were enabled; ASan stack checks were explicitly disabled,
LeakSanitizer was disabled because /proc is unavailable, and container exception
checks use UBSan only. Target OSes beyond Linux remain unverified.
