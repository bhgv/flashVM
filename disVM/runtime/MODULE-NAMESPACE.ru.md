# Загрузка модулей через namespace гостя

Теперь lookup/open/parse удерживает directory resource. Absolute имя ниже —
display provenance; после rename оно может устареть и не переоткрывается для I/O.
[Текущий контракт каталогов](DIRECTORY-ANCHORS.ru.md).

Файловый import теперь разрешает namespace группы **один раз**, до поиска cache
и native release. Cache key, open и parser получают одну копию absolute host path.
Конкурентный bind не перенаправляет начатое чтение. Loader держит открытый FD до
metadata/read/close; metadata берётся с объекта даже после замены pathname.
Глобальная exclusive-секция, перенос потока, восстановление REG и внешний сервис
для этого не нужны.

## Начальное окружение и наследование

Startup controller создаёт native namespace до загрузки entry image. Первый
guest удерживает группу; spawn разделяет либо отделяет её по прежним правилам.
Ссылка controller снимается после join всех native owners, в том числе при ошибке
startup. Обычный Sys path сначала использует host cwd. Для совместимости с
packaged programs начальный relative-import directory группы — каталог entry
package. Оба каталога явно хранятся в группе.

`FORKNS` согласованно копирует import directory и bindings. Успешный `chdir`
одновременно меняет cwd и relative-import directory под одним mutex. `NEWNS`
берёт resolved cwd за native root и `/` за оба virtual directory; bundled fallback
из старой группы не наследуется.

Прежние девять имён `/dis/lib/` теперь обычные начальные file bindings к siblings
entry package. `Sys.open/stat` и `load` видят одну привязку; MREPL может её заменить.
Специальная подмена в `kopen` удалена. Список имён прежний; listing их virtual
parents не синтезируется. Builtins с `$` сохраняют global registry; `$self`
связывает code/MP этого экземпляра. Embedded NUL запрещён до классификации
builtin/self/file; nil-H и пустое имя дают обычный отказ загрузки.

## Cache provenance и сохранённые экземпляры

Старый hosted `kdirfstat` возвращал length, но нулевые identity/time; прежний
Module мог приниматься после замены файла по тому же пути. Теперь реальные
inode/file identity, device, modification seconds и length копируются через
`fvm_port_file_info` под gate открытого FD и участвуют в проверке cache.
Length хранится в private loader Qid version; Sys.Dir/wire formats не меняются.
Старшие биты device помещаются в retained type; слишком широкая identity
запрещает reuse. Private Qid marker не позволяет narrowed старому Module совпасть
с новым представимым файлом. Отказ cache не запрещает свежую загрузку.

Module.path теперь содержит frozen absolute host path. Debugger-тесты требуют
точного provenance, не ослабляют сравнение до basename. Разные spellings одного
host object могут иметь отдельные cache entries; одно имя может иметь несколько
живых версий после atomic replacement. Modlink сохраняет private MP и прежний
code; bind/replacement влияет на будущий load, не меняя живой экземпляр и его
function quarantine.

Публиковать compiled images следует как immutable files с atomic replacement.
В имеющемся port timestamps имеют секундную точность: in-place edit с прежними
identity/length/modification seconds не гарантирует cache coherence. Конкурентная
запись на месте не является whole-image transaction. Полные provider revision/
content identity остаются работой следующего слоя вместе с Chan/directory handles.
Symlinks и lexical cwd сохраняют [границы path routing](VFS-NAMESPACE.ru.md).

## Проверки и границы

Public example запускается вне entry directory. Восемь detached native owners
загружают две реализации по одному virtual name:192 load, private MP при native
sleep, `$self`, retirement. Также проверены bundled-file rebind, сохранность
старого code, chdir/NEWNS, missing/nil/NUL names.

Test-linked probe задерживает **настоящее успешное открытие**. Другой guest до
release выполняет rebind/load; host атомарно заменяет исходный pathname другим
inode с сохранением length/timestamp. Настоящий registry lookup подтверждает cold
и warm ownership. Удержанный source получает старый code, следующий load — новый;
старый экземпляр жив. Оба cold/warm проверены в четырёх interpreter/JIT/poll
сочетаниях. Omission возвращает zero metadata и обязан дать конкретное stale value
со штатным retirement, не crash. Отдельно слишком широкая identity требует
безопасного отказа reuse cache.

Проверки выполняются Linux x64 обычно и под TSan. Обязательный native-набор
теперь66 с `namespace-load`; новая ARM64/RV64 CI — отдельная проверка. Общее
namespace/cache ядро использует имеющиеся portable FS/sync APIs. Исполняемые
Windows/macOS/Android adapters всё ещё предстоят. Vendor source и serialized ABI
не менялись. [Пример](../examples/namespace-load/README.ru.md),
[English](MODULE-NAMESPACE.md).
