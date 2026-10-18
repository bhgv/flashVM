# Sprite resources and attachMovie / Ресурсы спрайтов и attachMovie

Development checkpoint v0.25; roadmap stage 3 remains open.

## English

The bounded MovieClip SWF importer now accepts root DefineSprite (SWF 3+) and
ExportAssets (SWF 5+) in addition to its earlier action-timeline tags. Each sprite
has a character ID, a declared frame count and its own validated control stream.
Every nested tag boundary, End and ShowFrame count is checked. The currently
supported sprite controls are ShowFrame, DoAction, FrameLabel and End. Other
controls and resource kinds are still unsupported; this is not full SWF playback.

Character IDs must be nonzero and unique. Exports must refer to sprites already
defined in the root stream and contain nonempty terminated names. A later export
of the same ID replaces its previous linkage name; a later binding of the same
name resolves to the newer ID. Invalid nested definitions are rejected. As before,
loading runs no actions and failures clear the returned clip pointer.

`clip.attachMovie(linkage, instanceName, depth[, initObject])` creates an exported
sprite with its own frame table, current frame, playback flags, drawing state and
children. Immutable private frame records can be shared by instances. Replacing
a frame through the host API affects only that instance's frame table. Creating
an instance runs no frame actions; it enters frame 1 on its next advance.

The new clip is linked to its parent before initialization, so constructors can
read `_parent`. Replacing an occupied depth detaches and stops the previous clip.
Depths retain the existing dynamic range 0–1048575; negative timeline depths are
not implemented here. Name collisions at different depths retain the existing
host binding policy and still need reference-player comparison. Unknown linkage
names return undefined. Counted names with embedded NUL are rejected explicitly.

Enumerable initObject properties, including enumerable inherited properties,
are copied before the registered constructor runs. Enumeration snapshots keys;
reads/writes invoke actual accessors. Nonobject initObject values are ignored, as specified by the AS2 reference. `Object.registerClass` selects a constructor and its
prototype; the function runs with the attached MovieClip as `this`. It should
inherit MovieClip.prototype to retain its ordinary methods. Constructor return
values do not replace the native clip. Exceptions propagate, and side effects
on an already published child are not rolled back.

The imported library retains the live registerClass registry. An interpreter
with its own installed Object class uses its current registry. A restored clip
in a context without standard globals falls back to the transported registry.
Removing a registration affects future attachments, not existing instances.
Empty descendants created through createEmptyMovieClip retain their parent's
library and the library's builtin MovieClip prototype.

[GC:] Library definitions, linkage names, class functions and builtin prototypes
are ordinary managed edges reachable from a clip. Instances share the library,
and parent/child edges may form cycles. Native-call barriers protect unpublished
instances and initialization key snapshots. Registered constructor dispatch uses
the rooted call scope introduced for super and restores it after exceptions.
Serialization keeps the library, shared definitions and class registry in wire
version 7; no new public ABI is introduced. The attachMovie callback was appended
to the stable callback table, preserving prior IDs. Older readers may reject this
new callback ID; this is not a forward-compatibility promise.

## Русский

Импортер теперь читает DefineSprite и ExportAssets. Спрайт получает собственные
кадры и метки; вложенные границы тегов, End и число ShowFrame проверяются до
создания объектов VM. Экспорт разрешён только для уже объявленных спрайтов.
Повторный экспорт ID меняет его linkage name; повторное имя связывается с новым
ID. Остальные ресурсы и управление display list ещё не реализованы.

`attachMovie` принимает linkage ID, имя экземпляра, глубину и необязательный
initObject. У каждого экземпляра независимое воспроизведение, таблица кадров,
рисование и дети. Неизменяемые определения кадров и библиотека разделяются.
Изменение кадра одного экземпляра через API хоста не изменяет другой экземпляр.
Новый клип входит в первый кадр при следующем advance.

Сначала клип публикуется у родителя, затем копируются перечисляемые свойства
initObject, после чего вызывается зарегистрированный через Object.registerClass
конструктор. Он получает новый клип как this и может читать _parent и начальные
свойства. Исключения передаются вызывающему коду; уже выполненные изменения не
откатываются. Возвращённый конструктором объект не заменяет клип. Неизвестный
linkage ID возвращает undefined. Имена со встроенным NUL явно отклоняются.

Занятая глубина заменяется: старый клип отсоединяется и останавливается.
Поддерживается прежний динамический диапазон 0–1048575; отрицательные глубины
таймлайна и полная сверка коллизий имён остаются открытыми. Пользовательский
prototype должен наследовать MovieClip.prototype для доступа к обычным методам.

[GC:] Словарь, функции классов и определения удерживаются графом клипа и
переносятся вместе с ним. Контекст со стандартным Object использует собственный
реестр классов; контекст без стандартных глобалов использует перенесённый реестр.
Барьер нативного вызова защищает создание и вызовы аксессоров, а кадр вызова
конструктора сохраняет корни и восстанавливается после исключений.

## Tests / Тесты

`tests/swf_sprite_resources.cpp` checks linkage renaming, independent playback,
per-instance frame replacement, parent/root/depth replacement, empty descendants,
initObject enumeration, ignored primitive inputs and constructor order, live registration/removal, constructor
throws and recovery, pinned GC, queue/snapshot/file restoration and new attachments
in a receiver context without standard globals. It rejects truncated sprite tags,
frame mismatches, duplicate IDs, unknown export IDs and nested definitions.
File transport is tested with OS ports; the portable configuration checks its
explicit unsupported result. These are same-process multi-context file tests,
not independent-process or network sprite-attachment certification.

`tests/swf_sprite_allocation.cpp` injects failures at every new/new[] boundary
before successful loading and attachment, checks failure results, unpublished
outputs/children and collector recovery. It does not inject malloc or external
library allocator failures. The commented AS source in
`examples/actionscript/sprite_resources.as` explains arguments and independent
playback; it is a tutorial for an external AS compiler, not an executed text parser.

SWF tag layouts follow the published
[SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
DefineSprite and ExportAssets sections. Optional initialization behavior follows
the published [AS2 MovieClip reference](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html).
Exact accessor enumeration order and all SWF-version gates still need differential tests. This checkpoint does not establish full
Flash compatibility, display-list scheduling, shapes, clip events, audio/video,
cross-platform behavior or completion of any roadmap stage.

## Validation v0.25 / Проверки v0.25

Strict C89/C++98 Linux runs: headless 173/173, bundled AGG 183/183,
bundled Cairo 183/183, portable/no-OS 164/164. A previous headless run
needed one executable-permission repair; final complete runs passed without
repeats. The sanitizer driver passed 170 tests/examples after the initObject
correction. Heap/global ASan and UBSan are enabled; ASan stack checks are
explicitly disabled for the workspace exception/unwind environment, LeakSanitizer
is disabled because /proc is unavailable, and container exception tests use
UBSan only. This does not certify other platforms or complete Flash semantics.
