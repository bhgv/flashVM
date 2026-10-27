# Interactive editor on native Dis

[Live watch/watchtable/callref/setref commands](../../LIVE-PROJECTIONS.md)

Run `python3 build_menu.py`, choose `all`, accept the defaults, then select **5**.
The terminal editor starts with an empty document. It is written in Limbo and
runs on the packaged Dis interpreter/JIT. The separate idecore owner receives
real blocking channel messages for edits, history, searches and saved revisions.
The independently installed launcher is `out/disvm-runtime/run-editor.sh`.

Every source line is commented. The helper functions explain request ownership,
UTF-8 validation, input bounds and I/O. No document/history logic is implemented
in native C by this frontend. Menu item **6** retains the noninteractive workspace
service demonstration; automated installed checks use that demonstration.

| Command | Effect |
| --- | --- |
| `help`, `status`, `show` | Show commands, revision/dirty metadata, or complete source |
| `set TEXT` | Replace the whole document |
| `append TEXT` | Append text and one newline |
| `insert OFFSET TEXT` | Insert at a Unicode character offset |
| `delete START END` | Delete the half-open character interval |
| `find TEXT` | Report the first occurrence and line/column |
| `replaceall OLD\|NEW` | Replace nonoverlapping literal occurrences |
| `undo`, `redo` | Use actual owner history |
| `complete as PREFIX`, `complete limbo PREFIX` | Query the real declaration owner |
| `hint LANGUAGE NAME ARGUMENT` | Show exact overload and active-parameter help |
| `load PATH`, `load! PATH` | Read UTF-8 source; `load` refuses dirty-buffer replacement |
| `save PATH` | Replace that file, then acknowledge the exact written revision |
| `quit`, `quit!` | Quit cleanly, or explicitly discard unsaved text |

Text arguments accept `\n`, `\t`, `\r`, `\s` (space) and `\\` escapes. Paths
are literal and can contain spaces. Offsets count Unicode characters, including
non-BMP characters as one character; they do not count UTF-8 bytes. `STATUS 0`
means the owner accepted the operation. Other status values are the documented
Idecore result enum; rejected requests preserve the returned text/revision.

Try these commands in the launched editor:

```text
set alpha\n
append beta
show
undo
redo
find beta
replaceall beta|gamma
save example.b
quit
```

The save command writes the actual file. Its destination is replaced, not appended.
A failed save leaves the document dirty. Loading invalid UTF-8 or an oversized
file preserves the current document. Dirty EOF produces a visible failure after
the document owner acknowledges shutdown; the editor never silently autosaves.
Source import permits at most four UTF-8 bytes per allowed document character;
commands are limited to 4096 bytes and oversized lines are consumed completely.

The initial Sys host supports read-only opens, `create(OWRITE, 0666)` subject to
platform umask, read/readn, file writes and borrowed standard descriptors 0/1/2.
Other modes/permissions remain unsupported. Save is checked and flushed but not
a transactional filesystem replacement or a power-loss durability guarantee.
Standard input can block this single native owner; asynchronous I/O and graphical
FlashVM widgets remain the next integration boundary. The full graphical editor
is not claimed by this terminal client.


For example, `complete as MovieClip.goto` lists both frame-navigation methods;
`hint as MovieClip.gotoAndPlay 0` shows the frame argument's name, accepted types
and short help. `complete limbo Sys.pr` and `hint limbo Sys.open 1` use an
independent Limbo catalog. These are initial declaration catalogs, not contextual
local-variable inference or proof that every advertised host service is available.
The two index owners are explicitly shut down on exit. Relative file paths use
the caller's working directory; module imports resolve beside the entry module.

`show` renders C0 controls other than newline/tab as Unicode control pictures;
for example a NUL becomes `␀` and ESC becomes `␛`. Display conversion uses a copied
string, so saved bytes and document history remain unchanged. The busy-work
budget renews only after actual standard-input progress, allowing a long terminal
session without weakening the guard against a busy loop or repeated EOF reads.


### Read-only binary resources / Чтение бинарных ресурсов

`inspect PATH` asks the independent [Idefiles channel service](../../bridge/idefiles.README.md)
for the file extent and its first sixteen bytes. Paths retain interior spaces.
For example, `inspect editor-console.dis` shows compiled bytecode without trying
to decode it as source. The result contains `RESOURCE size=… offset=0 bytes=…`
and hexadecimal bytes. The existing source, history and saved revision stay intact.
Missing paths report `ERROR inspect`; no file is created or changed.

`inspect PATH` показывает размер и первые 16 байт `.dis`, `.swf` либо другого
ресурса. Чтение выполняет отдельный процесс Dis по типизированному каналу.
Путь может содержать пробелы. Команда не заменяет открытый текст и не изменяет
историю Undo/Redo; файловый дескриптор закрывается перед ответом. Это просмотр
байтов, а не декодер форматов SWF/Dis и не подключённая графическая панель.

## Directory browser

`dir PATH` opens a read-only stream and displays one batch of at most 32 entries.
Use `dirnext` to advance, `dirclose` to retire it, and `stat PATH` for copied
metadata. A failed replacement keeps the previous cursor. These commands leave
document/history state unchanged. Shutdown also closes an active cursor.
Names are escaped and emitted in bounded chunks.
[Full EN contract](../../DIRECTORIES.md) / [RU](../../DIRECTORIES.ru.md) ·
[Executable tutorial](../directory-browser/README.md).
