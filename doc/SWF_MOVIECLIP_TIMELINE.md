# SWF root action timelines / Корневые таймлайны действий SWF

Development checkpoint v0.24; roadmap stage 3 remains open.
The v0.25 extension adds sprite/linkage resources: [RU/EN](SWF_SPRITE_RESOURCES.md).

## English

`fvm_movieclip_load_swf` imports the root timeline into a new managed MovieClip.
It accepts bounded FWS SWF 1–8 and CWS SWF 6–8 using the bundled zlib decoder.
The supported root tags are End, ShowFrame, DoAction and FrameLabel. All other
tags return `FVM_UNSUPPORTED`; this API does not silently omit display objects.
The original `fvm_load_memory` and whole-file action API retain their behavior.

Loading runs no script. Every tag boundary, frame count and action stream is
validated before constructing the clip. The number of ShowFrame tags must match
the declared nonzero frame count. Frame labels require a terminating NUL; an
optional SWF 6+ named-anchor flag is accepted, but anchor navigation is not
implemented. Duplicate labels in one frame are rejected. Actions/labels after
the last ShowFrame are invalid. Failure clears the output pointer.

The first advance enters frame 1. Later playing advances move to the next frame
and wrap at the declared count. Existing gotoAndPlay/gotoAndStop and labels use
the imported frame definitions; actions execute on the next advance after a
seek, following the current host timeline model. Multiple DoAction tags in one
frame remain independent execution blocks with separate operand stacks.
Action failures propagate immediately; later blocks do not execute. There is
no rollback of earlier script changes. Each action block is limited to 65536
bytes. The configured memory budget bounds the decoded file, with the existing
64 MiB default; it is not a complete aggregate allocation budget.

[GC:] A native-call barrier protects the unpublished clip and frame containers
during construction. The clip owns its frame records and block strings through
managed graph edges. Hosts must root the returned object before collection.
Ordinary graph serialization preserves these records without another wire or
ABI change. Queue and file restoration can run the imported timeline in a
context without installing its standard globals first.

`tests/swf_movieclip_timeline.cpp` checks frame ordering, two blocks per frame,
receiver identity through property writes, wraparound, label seeking/stopping,
FWS/CWS, pinned collection, queue/snapshot/file restoration, truncated files,
frame-count mismatches, unsupported tags, malformed/duplicate labels, truncated
actions, actions beyond the last frame and operand-stack isolation. File checks
run with OS ports; the portable profile verifies their explicit unsupported result.

This is not full SWF playback. Resource dictionaries, DefineSprite, placed
shapes, display-list mutations, clip events, imported graphics, streamed media,
reference-player frame scheduling and version-specific semantics remain open.
Only Linux host builds have been run; target-platform acceptance remains open.

## Русский

Новый API `fvm_movieclip_load_swf` создаёт MovieClip из корневых DoAction,
ShowFrame и FrameLabel файлов FWS/CWS. End завершает файл. Загрузка не исполняет
скрипты; остальные теги возвращают `FVM_UNSUPPORTED`. Старые API загрузки файла
и исполнения всех действий сохраняют своё поведение.

Число кадров проверяется до создания клипа. Метки должны завершаться NUL;
повторная метка в одном кадре отклоняется. Дополнительный флаг named anchor
SWF 6+ принимается, но навигация по якорям ещё не реализована. Действия после
последнего ShowFrame недопустимы. При ошибке выходной указатель обнуляется.

Первый advance входит в кадр 1; последующие переходят вперёд с зацикливанием.
Метки работают с существующими gotoAndPlay/gotoAndStop. После seek действия
выполняются на следующем advance — это текущая модель хоста, ещё не подтверждённое
совпадение планировщика с Flash. Несколько DoAction одного кадра имеют отдельные
стеки. Ошибка прекращает оставшиеся блоки, без отката уже выполненного скрипта.
Один блок ограничен 65536 байтами; лимит декодированного SWF не означает полного
учёта всех выделений памяти интерпретатора.

[GC:] Барьер нативного вызова защищает создание ещё не опубликованного клипа.
Кадры и строки блоков принадлежат управляемому графу клипа. Хост обязан удержать
полученный объект перед сборкой. Перенос через очередь, снимок и файловый канал
сохраняет таймлайн; это проверяется в контексте без установки стандартных глобалов.
В минимальной конфигурации файловые операции явно неподдержаны.

Тесты перечислены выше. Полноценная сцена, ресурсы/спрайты, placement, графика из
SWF, события клипов, звук/видео и сверка порядка кадров с эталоном ещё отсутствуют
в этом импортере. Этап 3 и все остальные этапы плана остаются открытыми.

## Validation / Проверки v0.24

Strict C89/C++98 Linux builds: headless 171/171, bundled AGG 181/181,
bundled Cairo 181/181, portable/no-OS 162/162. The portable run required a
repeat of two executables after this workspace removed their execute permissions;
that was a launch failure, not an assertion failure. Headless first needed six
such repeats. The new timeline assertion initially caught an indexing error,
which was corrected before the passing runs.

The sanitizer driver passed 168 tests/examples, including independent TCP and
Error file processes. Heap/global ASan and UBSan were enabled. ASan stack checks
were explicitly disabled because of the workspace's exception/unwind environment;
LeakSanitizer was disabled because /proc is unavailable. Container exception tests
use UBSan only. These results do not certify full SWF compatibility or other OSes.
