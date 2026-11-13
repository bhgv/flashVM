# Guarded builtin scalar result publication

Copyright (c) 2026 bhgv

Native result address может указывать прямо на поле shared heap ADT. Compiler не
обязан создавать private temporary: `capture.value = control->pid()` способен
передать адрес `capture.value` в generated builtin frame. Захват frame/result
проверяет геометрию и lifetime, но сам по себе не защищает запись от другого Prog.

Network orphan-server acceptance обнаружила настоящую TSan race: native
`Processctl_pid` делал raw memmove в Cell, одновременно main Prog читал её через
обычный integer branch. Отказ от JIT или перенос записи в fixture local temp
скрыл бы дефект. Исправление находится в production builtin publication.

`sys_scalar_result.c` реализует `dvm_sys_scalar_result(snapshot, destination, size)`:
private snapshot содержит только scalar bytes; caller уже захватил валидный result
и сохраняет frame/managed owners rooted. Span ограничен 64 bytes. Общий range
cell domain защищает **весь** destination, включая unaligned/multiple-cell span.
Копирование не вызывает allocation, VM callback или error под guards.

Transient BUSY освобождает guards и повторяет только публикацию после poll.
Повторять native operation нельзя: `killwait_result` мог уже потребить точный
completion, cancellation мог быть отправлен, sleep завершён, export создан.
REFUSED/невалидная геометрия бросают до записи. Во время ожидания сохраняется
исходный Prog/TLS REG; не используется global exclusive или чужой executor.

| Подключённый builtin path | Что публикуется |
| --- | --- |
| Processctl.pid/cancelwait/kill/killwait | Один staged native WORD |
| Sys.millisec | Staged WORD bit pattern монотонных milliseconds |
| Sys.sleep | Один staged WORD после native reentry либо отказа |
| Proxyhost.open/canwait/finish/drop/close | Scalar status/permission |
| Proxyhost.publish | Scalar-only status/session/slot/generation tuple |

Это внутренний helper, а не автоматический перехват всех native ABI. Его нельзя
применять к pointer-bearing result tuple как замену retain/root/rollback:
`Proxyhost.begin` и managed Sys results требуют своих ownership adapters.
Идентификаторы/wire layout в результате этой правки не изменяются.

`fixtures/proxy-network/scalar-results.b` намеренно пишет native returns прямо в
shared fields с четырёх постоянных writers при одновременных branch readers.
Она входит в network regression в C/JIT и обоих poll paths. В TSan omission
control helper заменяется raw memmove, и принимается только actual data race с
`__wrap_dvm_sys_scalar_result` в stack, не crash или compiler failure. Обычная
сборка и TSan используют те же production helpers и native-only GC policy.
