# Файловые каналы в отладчике / File channel debugger

В v0.63 [`fvm_debug_channel_freeze`](../include/flashvm/debug.h#L114) поддерживает локальные файловые каналы.
Замораживается чтение: неблокирующий receive и raw read возвращают
[`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L47); блокирующий receive ждёт resume или close. Запись разрешена.
Close пробуждает читателей с [`FVM_ERROR`](../include/flashvm/flashvm.h#L43); закрытый файл просмотреть нельзя.
Это состояние конкретного handle, а не блокировка файла в других процессах.

`fvm_debug_channel_message(context, channel, index, out)` читает копию
сериализованной записи с указанным индексом относительно текущей позиции чтения.
Просмотр разрешён при freeze. Позиция восстанавливается под блокировкой канала
на успехе, EOF, повреждённой записи и отказе памяти; декодирование происходит
после освобождения блокировки. Повторный receive получает ту же запись.
Результат просмотра — самостоятельный граф, сохраняющий циклы; для хранения
объекта через последующий GC вызывающая сторона должна удерживать корень.
Ресурсные capabilities остаются общими живыми ресурсами, а не копиями состояния.

Ограничения: raw bytes без framing FVMG не являются сообщениями; индексация
проходит предыдущие записи последовательно и использует ограничение 64 MiB
на запись. Изменение файла внешним процессом не замораживается. В v0.67 remote endpoints поддерживают freeze/state через совместимый broker,
а v0.70 добавляет remote indexed peek (REMOTE_CHANNEL_DEBUG.md). В v0.65 [`fvm_debug_channel_state`](../include/flashvm/debug.h#L118) сканирует framed записи локального файла
под lock и возвращает pending/frozen/closed без изменения read cursor. Pending
считает complete framed records, а не проверенные decoded payloads; capacity=0.
Закрытый file handle имеет pending=0 (дальнейшее чтение через него невозможно).
Read/framing/OOM ошибки не изменяют переданный output. С v0.67 scan ограничен
65536 records/64 MiB; превышение возвращает UNSUPPORTED и сохраняет cursor/output. Write-only file даёт
[FVM_ERROR](../include/flashvm/flashvm.h#L43) при сканировании. Внешние изменения не защищены этим lock.
Эта возможность не закрывает стадии 6 и 7 или полную приёмку отладчика.

Проверки: `tests/file_channel_debug.c` — frozen raw/primitive/graph reads,
indexed peek, циклы/GC, блокирующий worker/resume, EOF, повреждённые записи,
закрытый и write-only файл. Существующие file_channels и
channel_debug_concurrent проверяют совместимость очередей и файлового транспорта.

## English

v0.63 adds debugger read freezing and non-consuming indexed inspection for
local file channels. Nonblocking receives and raw reads return NOT_FOUND while
frozen; blocking receives wait for resume or close. Writers remain active.
Closure wakes receivers with ERROR and prevents subsequent inspection.
Freezing applies to one shared channel handle, not to external file access.

Message indexes are relative to the unread cursor. The channel lock protects
record copying and cursor restoration on every outcome; graph decoding runs
after unlocking. Inspection creates a separate graph with preserved cycles.
Callers must root inspected objects across GC. Embedded capabilities continue
to refer to shared live resources; their state is not frozen by inspection.

Raw unframed bytes and external file mutations are not supported. v0.67 adds
compatible broker freeze/state controls; v0.70 adds remote indexed peek. v0.65 scans framed local file records under the channel lock and
reports pending/frozen/closed without advancing the cursor. Pending counts full
framed records, not decoded/validated payload graphs; capacity is zero. Closed
file handles report zero readable pending records. Read/framing/allocation
failures leave the caller output unchanged. Write-only scanning returns ERROR. Indexed lookup is sequential and bounded by the
existing 64 MiB per-record limit. Stages 6 and 7 remain open.
The regression test covers freezing, indexed inspection, cyclic graphs/GC,
worker resume, EOF, malformed records, closed handles and write-only files.

Проверено: headless 237/237, AGG 253/253, Cairo 253/253, portable 228/228; targeted ASan/UBSan 27/27. Leak detection и stack ASan отключены из-за ограничений среды. Это Linux-профили, а не подтверждение запусков Windows/macOS/Android.

Verified Linux profiles: headless 237/237, AGG 253/253, Cairo 253/253, portable 228/228; targeted ASan/UBSan 27/27. Leak detection and stack ASan are disabled in this environment; no Windows/macOS/Android runtime certification.

Проверки v0.65 / v0.65 verification: headless full suite 239/239; targeted GC/resource/channel tests 28/28 in AGG, Cairo, portable and ASan/UBSan. The previously stated sanitizer and target-platform limits still apply.

## v0.69 — ограниченный поиск / bounded indexed lookup

Просмотр по индексу ограничен 65536 записями (индексы 0–65535) и суммарно
64 MiB заголовков и тел. Индекс вне предела возвращает UNSUPPORTED до чтения;
тело, превышающее оставшийся бюджет, отклоняется до выделения памяти и чтения.
Позиция файла восстанавливается при любом результате. Это ограничение работы
отладчика, а не размера канала. Обычное получение записей не меняется.

Indexed inspection is bounded to 65536 records (indexes 0–65535) and an
aggregate 64 MiB of framing and bodies. Out-of-range indexes return UNSUPPORTED
before reading; a body exceeding the remaining budget is rejected before
allocation or body reading. Every outcome restores the file cursor. These are
debugger work limits, not channel size limits; normal receive is unchanged.

Проверки v0.69 / v0.69 verification: headless 246/246; channel debugger
regressions 4/4 under ASan/UBSan (heap/global instrumentation; leak detection
and stack instrumentation disabled). Source comments and GC tags pass audits.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_debug_channel_freeze` | function | [include/flashvm/debug.h:114](../include/flashvm/debug.h#L114) |
| `fvm_debug_channel_message` | function | [include/flashvm/debug.h:125](../include/flashvm/debug.h#L125) |
| `fvm_debug_channel_state` | function | [include/flashvm/debug.h:118](../include/flashvm/debug.h#L118) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
<!-- source-links:end -->
