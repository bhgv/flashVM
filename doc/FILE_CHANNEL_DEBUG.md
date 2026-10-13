# Файловые каналы в отладчике / File channel debugger

В v0.63 `fvm_debug_channel_freeze` поддерживает локальные файловые каналы.
Замораживается чтение: неблокирующий receive и raw read возвращают
`FVM_NOT_FOUND`; блокирующий receive ждёт resume или close. Запись разрешена.
Close пробуждает читателей с `FVM_ERROR`; закрытый файл просмотреть нельзя.
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
на запись. Изменение файла внешним процессом не замораживается. Удалённые
endpoints не поддерживают freeze/peek. `fvm_debug_channel_state` пока сообщает
состояние только очередей, а для файлов возвращает `FVM_UNSUPPORTED`.
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

Raw unframed bytes, remote endpoints, external file mutations and file-state
snapshots are not supported. Indexed lookup is sequential and bounded by the
existing 64 MiB per-record limit. Stages 6 and 7 remain open.
The regression test covers freezing, indexed inspection, cyclic graphs/GC,
worker resume, EOF, malformed records, closed handles and write-only files.

Проверено: headless 237/237, AGG 253/253, Cairo 253/253, portable 228/228; targeted ASan/UBSan 27/27. Leak detection и stack ASan отключены из-за ограничений среды. Это Linux-профили, а не подтверждение запусков Windows/macOS/Android.

Verified Linux profiles: headless 237/237, AGG 253/253, Cairo 253/253, portable 228/228; targeted ASan/UBSan 27/27. Leak detection and stack ASan are disabled in this environment; no Windows/macOS/Android runtime certification.
