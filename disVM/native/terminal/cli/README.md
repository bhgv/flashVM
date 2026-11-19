# Explicit bidirectional CLI / Явный двунаправленный CLI
[DisVM terminal launch / Запуск DisVM через терминал](../../../runtime/TERMINAL-LAUNCH-CLI.md)

`unibus-bidir` is a native binary stdio relay for the
[negotiated ordered broker](../order/broker/README.md). It launches no DisVM,
SWF, editor or remote computation. The managed desktop/editor launchers still
require their existing explicit native handoff; this executable does not replace
that integration. It requires neither a display context nor UI-host libraries.

`unibus-bidir` — общий native-вход Unibus: клиентские S-записи из stdin отправляются
собеседнику, полученные записи выходят в stdout. Это не специализированный IDE-host
и не запуск удалённого Dis-модуля. Обычный FD/VFS-консольный запуск DisVM остаётся
независимым от наличия контекста FlashVM.

## Selection and authority / Выбор и полномочия

No arguments, or `--profile none`, return successfully without transport or stdio
I/O. There is no automatic connection or fallback from a failed ordered profile.
`--help` prints text before acquiring any carrier. Invalid arguments return2
before protocol I/O. An ordered invocation, after its host delegates descriptor3,
is for example:

```sh
build/inferno-native/terminal/cli/unibus-bidir \
    --profile ordered --phy-fd 3 --lane 71 \
    --tag 0 --tag 7 --tag 9 --root . --rx records --tx records
```

The host supplies a connected, exclusively delegated reliable ordered stream.
This command does not open a network address, authenticate users, listen, or infer
filesystem authority from a URL. Both ends must explicitly select P9MX v1 and
compatible Unibus envelopes. `--channel` selects the Styx client session0 or1;
the unused session is closed. This executable exports **no reverse filesystem**.
Bidirectional application records do not imply that it starts two Styx servers.
The remote endpoint must expose the authorized receive/send paths through Styx.
Original libstyx implements attach/walk/open/read/write; no 9P message is changed.

Native CLI currently builds with the Linux64 native SDK. Its POSIX adapter accepts
stream sockets or character devices with caller-supplied stream semantics;
regular files, directories, datagram sockets and aliases of stdio are rejected.
Other physical carriers remain a RemotePhy adapter responsibility. The CLI
retains its own CLOEXEC duplicate of the delegated FD. Setting nonblocking mode
on this duplicate also affects the shared open-file description: the caller must
not concurrently use another duplicate for I/O. All three standard descriptors
must already be open; stdin must be readable and stdout writable.

Права определяются выданным соединением и закреплённым корнем Styx, а не именем
профиля. Никакой ambient-FS или неявного сетевого dial нет. Переход к физическому
носителю остаётся в RemotePhy; этот CLI предоставляет конкретный POSIX-вход.
Выданный FD нельзя параллельно использовать другим reader/writer.

| Option | Default | Accepted value |
|---|---|---|
| `--profile` | `none` | `none`, `ordered` |
| `--phy-fd` | required for ordered | FD3..INT_MAX |
| `--lane` | required for ordered | nonzero unsigned32 outer TX tag |
| `--tag` | required, repeatable | unique unsigned32 inner client tag, including0 |
| `--channel` | 0 | 0 or1 |
| `--root` | `.` | canonical relative Styx root |
| `--rx`, `--tx` | `records` | canonical relative endpoint paths, `.` allowed |
| `--limit` | 4096 | inner body bytes1..65472, including encoded tag |
| `--window` | 1024 | future displacement1..2^63−1 |
| `--timeout-ms` | 3000 | attach/profile deadline1..120000ms |
| `--receive-records` | 0 | 0=unbounded; otherwise owner receive quota up to SIZE_MAX |

Numbers are canonical unsigned decimal; signs, redundant leading zeroes, overflow,
duplicate singleton options and duplicate grants fail. Non-profile transport
options are incompatible with `none`. Paths are copied and validated before I/O.
An inner body limit of1 permits an empty tag0 record and does not restrict private
service-record capacity. Grants whose encoded tag alone exceeds the body limit
refuse during no-I/O parsing. The relay now uses the
[shared captured attachment API](../order/broker/ATTACHMENT.md).

## Application streams / Клиентские потоки

stdin/stdout contain complete ordinary binary S-records with their original CRC
and integer encoding, not text commands. Inner tags are restricted to the explicit
grants. The relay adds/removes transport envelopes internally. Application-local
inner0 is unrelated to transport root0; negotiation, sequence prefixes and unknown
root services never appear in application stdout. Diagnostics use stderr only.
Clients never receive direct access to the carrier or service queues.

A dedicated broker RX dispatcher and retained TX worker serve the carrier. A
native stdin producer and separate native readers consume the granted client
queues. Waiting on stdin, one client inbox, a selected counter or stdout never
puts the transport dispatcher to sleep. stdout serializes complete S-records;
per-tag order is preserved, while interleaving between tags is unspecified.
There is no CLI-introduced fixed application-queue cap or dropping policy.

Чистый EOF stdin завершает только producer: приём продолжает работать. EOF
транспорта обрабатывается после уже принятых полных записей; готовые клиентские
очереди дочитываются. Неисправность транспорта и явная отмена будят ожидания
независимо от продвижения dispatcher. Квота приёма относится ко всему relay,
а не ограничивает очередь брокера.

With quota0, clean peer EOF after draining is success0. A positive quota stops
successfully after that many complete output records; earlier EOF is failure2.
Malformed/truncated input, unauthorized tags, failed negotiation, physical errors
or broken stdout return2. A receive quota does not promise that all stdin records
were transmitted; successful enqueue is not remote execution acknowledgement.
SIGINT/SIGTERM cleanup described below applies after connection setup enters the
relay; initial attach/profile setup uses its configured bounded deadlines.
A native send/renewal wait has a120000ms deadline;
expiration ends the relay without guessing acceptance or replaying a record.

SIGINT/SIGTERM are consumed by a dedicated native sigwait worker: cleanup joins
started workers and restores stdout flags, returning130/143. SIGUSR1 is reserved
for the owner's private signal-worker wake. stdout is temporarily nonblocking so
an undrained pipe remains cancellable. Its shared open-file description must be
exclusive to this relay during execution. An interrupted/failed write may leave a
partial final S-record; no successful completion is claimed for those bytes.

## Verification / Проверка

CTest exercises the actual executable against an original libstyx server over an
inherited socketpair: binary payloads, application0 privacy, independent tags,
reordering, selected renewal, both P9MX sessions, captured file roots, tiny limits,
stdin EOF, clean transport EOF, invalid input and failed negotiation. A native
condition-fenced EOF fixture forces the last accepted packet to overlap retirement.
Undrained stdout tests send SIGINT/SIGTERM and check restoration of shared flags.
Setup faults reject allocations, lifecycle subscriptions and each session-thread
startup while retaining actual native owners and cleanup. These tests demonstrate
this explicit relay, not a complete remote Dis/SWF application or multi-carrier
scheduler.
