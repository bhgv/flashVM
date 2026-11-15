# Directory union bindings / Union-привязки каталогов

Copyright (c) 2026 bhgv

Runtime 0.126 implements ordered directory layers through ordinary `Sys.bind`.
It adds no guest module, opcode or flag. `Vfsctl` remains an unimplemented proposal;
this work does not introduce an `nsadmin` authority or bypass filesystem rights.
Runtime 0.126 реализует слои каталогов через обычный `Sys.bind`, без нового guest
ABI. `Vfsctl` пока предложение; нового права `nsadmin` в этой реализации нет.

| Call / Вызов | Behavior / Поведение |
| --- | --- |
| `bind(a, on, MREPL)` | Replace the complete stack / Заменить всю стопку |
| `bind(a, on, MBEFORE)` | Prepend; retain original target on first union bind / Добавить сверху; при первом union сохранить исходный target |
| `bind(a, on, MAFTER)` | Append; retain original target first / Добавить снизу; исходный target остаётся первым |
| `MCREATE` with one order | Permit missing-file creation in this branch / Разрешить создание отсутствующего файла в этой ветви |
| `unmount(a, on)` | Remove the first matching source identity / Удалить первый слой с совпавшей source identity |
| `unmount(nil, on)` | Remove all layers at this exact target / Снять все слои exact target |

## Ordinary Sys example / Пример обычного Sys

The directories and target must already exist. Check every result in real code.
Каталоги и target должны существовать; в программе проверять каждый результат.

```limbo
sys->bind("/src/top", "/view", Sys->MREPL);
sys->bind("/src/middle", "/view", Sys->MAFTER);
sys->bind("/src/work", "/view", Sys->MAFTER | Sys->MCREATE);
fd := sys->open("/view/config", Sys->OREAD);
created := sys->create("/view/new", Sys->OWRITE, 8r666);
sys->unmount("/src/top", "/view");
```

Lookup chooses the first existing entry. If `config` exists in top, a write opens
that entry and can fail on its permissions: it never retries middle/work. If `new`
is absent everywhere, create uses the first `MCREATE` branch (work here). Existing
files are not concatenated and seek does not switch layers. A later open observes
changed host permissions; an opened FD retains its original node/cursor.
Lookup выбирает первый существующий entry. Отказ записи в top не перенаправляет
запись вниз. Только отсутствующий `new` создаётся в первой `MCREATE` ветви.
Файлы не склеиваются; seek не переключает слой. Следующий open видит обновлённые
host permissions; открытый FD сохраняет прежний node/cursor.

Order flags together, unknown flags and before/after on regular files fail.
Binding a multi-branch union copies its ordered branches without granting create.
A mounted source can be bound with `MCREATE` only if it has one branch already
marked `MCREATE`, matching Inferno's cmount restriction.
Несовместимые order flags, неизвестные flags и union обычных файлов отвергаются.
Копирование union сохраняет порядок, но не даёт create. Mounted source с `MCREATE`
допустим только с одной уже creatable ветвью, как в Inferno cmount.

## Directory streams and lifetime / Каталоги и время жизни

`dirread` enumerates branches in order and preserves duplicate names. Streams open
lazily. EOF/open/read failure advances to the next branch, following Inferno
`unionread`. Each FD has its own index/current stream. Rewind to zero restarts
enumeration. An already opened branch stream remains current through mutations;
subsequent branches follow the live mount-head order. Removing all branches and
creating a new mount head cannot attach an old directory FD to the new stack.
`dirread` сохраняет совпадающие имена и порядок слоёв; ветви открываются лениво.
EOF/ошибка переходят к следующей ветви. Каждый FD имеет свой cursor; seek в ноль
сбрасывает enumeration. Уже открытая ветвь сохраняется при изменении namespace,
следующие выбираются из текущего mount head. Новый mount head не подменяет старый FD.

Spawn shares the namespace; FORKNS copies the stack in the same order. Open plans
retain every selected anchor/provider. Directory FDs retain their original group,
including across FORKNS. Namespace locks protect native topology only; provider
callbacks and native I/O run unlocked outside managed admission. Mutation commits
check the captured revision and boundedly retry stale plans. No global stop.
Spawn разделяет namespace, FORKNS копирует порядок. Plans удерживают anchors и
providers; directory FD удерживает исходную группу. I/O и callbacks идут вне
namespace mutex и managed admission; commit проверяет revision. Global stop нет.

## Compatibility limits / Границы совместимости

The semantic reference is Inferno experiment revision
`7f2241455734e1142a3254fad9d1ec4f94032ac7`: `cmount`, `cunmount`, `createdir`
in the retained [chan.c](../upstream/emu/port/chan.c), and `unionread` in
[the same revision's sysfile.c](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/emu/port/sysfile.c).
No upstream snapshot was modified or whole-OS scheduler imported.
Семантика сверена с указанной ревизией Inferno; upstream snapshot не изменён.

This is still a hosted path adapter, not full retained `Chan.walk`. It tries the
complete relative suffix in each layer; nested partial walks, mount crossing and
symlink identity do not yet have full Inferno semantics. Exact multi-layer union
root `chdir` explicitly refuses: a single native cwd anchor cannot retain its
union. Provider chdir/remove/module loading remain unsupported. `Sys.mount`, Styx,
guest credentials and filesystem confinement are not implemented by this step.
Native host credentials/rwx and trusted provider policy still govern access.
Это hosted path adapter: проверяется полный relative suffix каждой ветви, а не
полный component-by-component Chan.walk. Вложенный walk, mount crossing и symlinks
ещё не полностью совместимы. `chdir` в multi-layer union root явно отказывает;
один cwd anchor не сохраняет union. Provider chdir/remove/load, Sys.mount, Styx,
guest credentials и confinement требуют следующей работы. Действуют host rwx и
policy trusted provider, не независимая guest-модель безопасности.

The proposed read-top/write-lower routing, dynamic read-only retry and concatenated
files would be extensions. They are deliberately not part of these bind semantics.
Предложенные read-top/write-lower, retry после read-only и склейка файлов — будущие
расширения; они не включены в эти bind semantics.

The [guest fixture](fixtures/namespace-union/main.b) and host driver check real
bytes, chmod handshakes, readonly refusal, creation targets, copied/shared groups,
replacement, layer removal and retained streams. Provider acceptance also combines
a memory directory with a native directory through the same ordinary Sys calls.
Fixture проверяет реальные bytes, chmod, отказы, цели create, namespace copy/share
и lifetime; provider test объединяет memory и native directory через тот же Sys.
