# Sys.remove через удержанный namespace

`Sys.remove(path)` удаляет обычный файл, final native symlink либо пустой каталог
через namespace вызывающего. Возвращает0 при успехе, -1 с guest error при lookup/
native/admission failure. Rooted Sys frame захватывается до conversion/allocation.
Embedded NUL и oversized paths не вызывают filesystem effects. Nil String
сохраняет существующую nil-reference ошибку (bootstrap может представлять пустой
literal как nil); allocated empty path отвергается. Debugger builtin-fault пример
теперь вызывает remove(nil), а не unavailable stub, сохраняя real caller frame.

Source под namespace mutex выбирает один retained directory и relative name.
Remove выполняется вне mutex и managed admission, не переоткрывает display path
каталога. Rebind/unmount и host rename parent не перенаправляют выбранную операцию.
Source-local cleanup держит plan при native return, cancellation и unwind.
Guest FD slot и постоянный каталог статистики объектов не выделяются.

Shared-cell guards защищают только scalar publication. При занятой result cell
повторяется publication, не deletion. Cancel до native admission не делает I/O;
после admission удаление уже могло завершиться. Rollback/replay не обещается.
Нет global stop/exclusive, миграции или восстановления live REG из snapshot.

## Поведение и границы

POSIX использует [relative removal](../../flashVM/doc/DIRECTORY_REMOVE.ru.md).
Final symlink удаляется сам, включая directory/dangling links. Непустой каталог
отклоняется без recursion. Intermediate symlinks обрабатывает host. Virtual path
нормализуется lexical до routing, как open; полного Chan walk/sandbox нет.

Selected root/cwd с relative именем dot отвергаются; удаление самого directory
binding этим adapter не реализовано. Children удаляются. File binding удерживает
parent+basename: remove по virtual alias удаляет выбранный source file и сохраняет
физический файл под target. Binding record остаётся до unmount; последующий
доступ отклоняется до появления source либо смены route.

Открытый POSIX FD читается после unlink; загруженный Module сохраняет code/private
MP. Следующий load удалённого image отклоняется даже при живом cached Module.
Namespace revision описывает namespace mutations, не existence/content children;
remove её не увеличивает. Тексты namespace debugger — display provenance, не stat.

Child может смениться между classification и unlinkat. Это parent/name removal,
не inode compare-and-delete. Same-kind replacement может быть удалён, смена kind
может дать отказ. Действуют native permissions. Recursive/secure erasure,
durability и полный provider contract не заявляются. Windows backend UNSUPPORTED;
macOS/Android POSIX ещё требует физической приёмки.

## Исполняемые проверки

[Пример](../examples/namespace-remove/README.ru.md): четыре native owners с copied
namespaces удаляют files/empty dirs/bound-file sources, отклоняют nonempty/root/
dot/NUL paths, сохраняют FD/provider после unlink и проверяют отказ нового load.
За shared name ровно один победитель. Host реально переименовывает parent и
ставит decoys: обычные операции удаляют original children. Omission по display
path обязан реально удалить decoy, оставить original и завершиться штатно.
Probe требует ровно четыре removal calls с shared result (без effect replay)
и zero live anchors при shutdown. Interpreter/JIT и оба GC polls; оба режима
проверяют omission. Real-port дополнительно покрывает file/directory/dangling
symlinks, invalid paths, empty/full dirs, восемь OS owners и FD после unlink.
[Sys entry](vfs/remove.c), [resource contract](vfs/resource.h),
[English](NAMESPACE-REMOVE.md).

Приёмка:18 целевых обычных и19 TSan CTest-групп. После финальных port/symlink
проверок повторены remove, retained debugger и generation replay — четыре группы
в каждой сборке. Standalone real port прошёл ASan/UBSan с leak detection disabled;
whole-runtime sanitizer/LSan не заявляется. Public header strict C89/C++98,
doc links и лимит500 строк проверены. Регистрация69 workloads не доказывает
исполнение non-x64; архитектурная CI остаётся отдельной проверкой.
