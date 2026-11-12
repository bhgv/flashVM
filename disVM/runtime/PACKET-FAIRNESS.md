# Independent packets progress beside an unsupported destructor

Shared Fast previously returned immediately when the oldest frame packet reached
an unsupported final owner or a refused publication. Every subsequent packet
waited behind that child, even when it contained only supported strings. Under
native admission, an unknown destructor could therefore hold independent memory
until all guests became quiescent.

`drain_shared_children` now rotates a blocked packet without changing its cursor,
references or remaining root batch. Each refused child consumes one budget unit.
A pass visits at most the packet count observed at entry; newly published packets
wait for a later pass. Successfully consumed children continue to cost one unit.
A partially drained packet keeps its normal position when its budget expires.

| Packet state | Shared action |
| --- | --- |
| Supported child or transferable cycle seed | Consume once and advance cursor |
| Unsupported callback or publication refusal | Keep child/cursor/roots, charge visit, rotate packet |
| Completed packet | Withdraw entry, clear/destroy root batch, free metadata |
| Only blocked packets remain | Return no completed work; allow retained assistance |

Rotation is deliberately excluded from the completed-work count. Reporting every
rotation as progress would keep choosing shared work at quiescence and starve
the retained callback needed to resolve the blockage. No arbitrary destructor is
authorized by this change; later children within the blocked packet stay ordered.
Queue rotation uses the existing native metadata mutex. No global execution lock,
new root representation, Prog migration or register restoration is introduced.

## Actual runtime regression

`disvm_packet_fairness` runs a real Limbo application and two permanently bound
native residents. They alternate genuine reader epochs while at least one stays
admitted. The parent publishes a mapped frame packet containing a string prefix,
an unsupported final owner and a string suffix, followed by two independent real
owner packets through `dvm_runtime_rc_transfer_frame`.

The fixture observes actual allocator frees and allocation generations. The
prefix and two independent strings must physically free with active peers and
without stopped admission. The unsupported owner and suffix must each retain
exactly one reference and one registered root. The cursor must not revisit the
already freed prefix. Four real zero-work Fast callbacks must occur after the
independent frees, proving that rotating the blocked packet invents no progress.
Only after peer release may the actual retained callback and suffix free once;
the independent Type creator must regain its sole descriptor reference.

Twelve positive profiles cover C/JIT, both polling paths and real Fast budgets
1, 2 and the configured default. Two private semantic controls remove rotation
or incorrectly count it as completed work. They must fail with their precise
protocol diagnostic, not with compilation errors, signals or sanitizer warnings.
Existing packet-cycle, managed-owner and large-owner tests cover adjacent retry,
root transfer, reader epoch and quiescent resource-assistance paths.

This remains Linux x86-64 evidence. TSan instruments native C/host/fixture code;
it does not instrument emitted JIT instructions. General tracing in quiescence,
unsupported callbacks and unsupported graphs still need further work before all
global exclusion can disappear. ARM64/RV64 execution is not certified here.

# Независимые пакеты не ждут неизвестного финализатора

Раньше неподдержанный ребёнок в голове очереди прекращал весь shared-проход
пакетов. Теперь Fast сохраняет его ссылку, корень и курсор, учитывает просмотр
в бюджете и перемещает пакет в хвост. За проход проверяется не больше исходного
числа пакетов. Независимые пакеты могут освобождаться при активных гостях;
порядок детей внутри заблокированного пакета сохраняется.

Само перемещение не считается выполненной работой: иначе оно вытеснило бы
резервную очистку в покое. Неизвестный callback по-прежнему вызывается только
на резервном пути. Новых замков исполнения, переноса Prog и восстановления
живых регистров из сохранённого снимка нет.

Проверка использует настоящие Limbo-потоки, пакеты, корни, эпохи и физические
free: смешанный пакет с уже освобождённым префиксом, заблокированным владельцем
и остатком, затем два независимых пакета. Бюджеты 1/2/обычный проверяются в C/JIT
и обоих polling. Два намеренных нарушения обязаны обнаруживаться отдельно.
Это ещё не полностью конкурентная общая трассировка GC.
