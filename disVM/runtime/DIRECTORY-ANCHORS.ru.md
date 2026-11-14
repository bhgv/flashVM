# Удерживаемые ресурсы каталогов

Namespace group теперь владеет native handles корня, cwd, import base и directory
bindings. FORKNS согласованно копирует ссылки; NEWNS удерживает выбранный cwd как
новый корень. Относительная операция внутри cwd/import base продолжает работать
с тем же каталогом после host rename и bind поверх его virtual имени. Более
глубокие descendant bindings по-прежнему выбираются по virtual components.
Обычный spawn разделяет группу родителя.

Resource plan под mutex захватывает имена, revision и собственную ссылку на
каталог. Blocking open/stat/acquisition выполняются с этим handle после снятия
mutex и выхода из managed admission. Во время native I/O нет доступа к указателю
группы. Commit проверяет revision; конфликты повторяются не более восьми раз.
Нет global stop/exclusive, миграции worker или восстановления live REG из снимка.

Загрузчик удерживает один selection до конца cache/open/parser. Module.path —
абсолютное **display имя на момент выбора**, после rename оно может устареть.
Anchored I/O не переоткрывает это имя. Metadata берётся с реально открытого image;
правила atomic replacement и границы cache описаны в
[namespace загрузчика](MODULE-NAMESPACE.ru.md).

Plan расположен по стабильному адресу в C stack своего владельца. Native cleanup
снимает ссылки до unwind frame; копировать живой cleanup record нельзя.
Group/binding/plan держат независимые ссылки: unmount, уход родителя и peer rebind
не закрывают чужой handle. Последнее закрытие вне namespace mutex. Managed roots
и постоянный каталог статистики объектов не добавляются.

## Точные границы

Удерживается identity **каталога**. Binding обычного файла держит parent directory
и basename: atomic replacement виден следующим open. Собственный rename/unlink
этого файла не отслеживается. Уже открытые файлы и cursors сохраняют свои handles.

Virtual `..` остаётся lexical: выход за subtree удержанного cwd/import base идёт
через virtual root lookup. Полного Inferno Chan/mount-parent walk ещё нет.
Symlinks обрабатывает host; sandbox не заявляется. Children могут измениться
между metadata и open. Лимит пути4096 bytes с NUL. POSIX anchor требует читаемого
каталога, а не только разрешения search.

Исполняемые проверки проведены на Linux x64. POSIX реализация openat/fstatat и
независимых fdopendir выбирается также для macOS/Android, но физической приёмки
этих платформ здесь нет. Windows явно возвращает UNSUPPORTED для новых операций;
обычный file API сохранён. Исполняемые non-Linux adapters, union/mount transports,
providers, FD/env/credentials и dynamic C/Limbo providers остаются работой.

## Проверки

[Пример](../examples/directory-anchors/README.ru.md) запускает четыре native owner.
Драйвер действительно переименовывает каталоги и ставит подмены по старым именам.
Чтение, create, stat, независимые cursors и imports должны идти к исходным objects;
явное обращение по старому пути — к подмене. Bind поверх cwd меняет absolute route,
но сохраняет удержанный cwd. Source-filter unmount сравнивает полные device/inode.
Shutdown сверяет число успешных acquisitions/closes и требует live=0.
Контрольное исключение handle делает настоящий old-path read, обнаруживает
подмену и штатно завершает потоки. Проверяются interpreter/JIT и оба GC polls.

Отдельный port-тест: восемь OS threads,1024 чтения, ровно один exclusive creator,
независимые cursors и поздний отказ fdopen. Отказ обязан закрыть настоящий FD и
сохранить содержимое существующего файла. Если создавался новый файл, поздний
отказ может оставить пустой файл; опасного pathname unlink rollback нет.

[Portable API](../../flashVM/doc/DIRECTORY_ANCHORS.ru.md),
[контракты](vfs/resource.h), [English](DIRECTORY-ANCHORS.md).

Приёмка блока:107/107 обычных CTest-групп. После последней правки error cleanup
повторены шесть обычных и семь TSan-групп; ранее прошли одиннадцать целевых
TSan-групп и весь anchor driver. Real-port прошёл ASan/UBSan с отключённым leak
detection: LSan в этом окружении недоступен. Это не whole-runtime sanitizer/leak
coverage. Public header компилируется strict C89 и C++98. В архитектурном списке67
нагрузок есть directory-anchors; исполнение ARM64/RV64 требует CI и не выводится
из этого x64-прогона.
