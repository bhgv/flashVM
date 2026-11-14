# Встроенный захват аварий scalar RX-кода

Runtime сам устанавливает Linux64 handlers для SIGILL/SIGSEGV/SIGBUS/SIGFPE до
запуска VM threads. GDB, Python, библиотека debugger и внешний supervisor для
этого не нужны. `--gdb` остаётся необязательным способом запуска. Сохраняется
постоянный системный owner каждого Prog, exclusive=0, без глобальной остановки.

## Граница допуска

Сейчас захват/восстановление охватывают выбранные **integer scalar RX fast leaves**,
перечисленные в `jit_fault/invoke.c`. Это не общее восстановление произвольно
повреждённой native memory. Вызовы функций, переходы stack/module, allocation,
managed pointers/refcounts, floating-point/SIMD и unsafe C/helpers не входят в
допущенное подмножество. Их обычное native/interpreted исполнение сохраняется,
но handler не объявляет для них безопасный landing.

До вызова проверяются исходный opcode, адрес callback внутри immutable RX extent,
ACTIVE managed admission, отсутствие object guards и native cleanup checkpoint
своего owner. Чужой callback или другой opcode не может заимствовать landing.
Provenance mapping хранится в native scalars, не в managed roots. Alternate signal
stack создаётся лениво, не меньше64KiB и SIGSTKSZ; существующий enabled stack
может заимствоваться. Свой stack освобождается при retirement либо worker teardown,
включая error shutdown. При невозможности подготовить stack native invocation
отклоняется до effects и операция выполняется обычным интерпретатором.

Handler проверяет положительный kernel si_code и **настоящий kernel PC** внутри
допущенного RX mapping. Он пишет lock-free scalar TLS snapshot, публикует его
готовность и переходит в живой landing того же OS thread. Нет allocation, mutex,
logging, доступа к managed memory, обновления карантина, изменения GC admission
или вызова guest/error handlers. Native landing сохраняет и восстанавливает
signal mask. Продолжения между потоками не переносятся.

После landing обычный код своего owner повторно проверяет ACTIVE admission,
отсутствие guards и чистоту native checkpoint. Изменённое admission/cleanup
запрещает recovery и приводит к abort: нельзя очищать уже потерянные helper
frames. Fault вне RX, недопущенная операция либо raise/kill вместо synchronous
RX fault следуют исходному OS disposition. После завершения всех source threads
исходные dispositions возвращаются. Повторная initialization безопасна.

## Доставка ошибки и диагностика

Для допущенного fault выводится `JITSIGNAL`: signal/code/fault address, native
PC/SP, RX offset и replay=0. Ограниченный `JITCPU` содержит именованные integer
CPU-регистры и PID источника. `JITFAULT` связывает аварию с исходным модулем,
интервалом функции, Dis PC/opcode/backend и отключает JIT **всей функции** для
следующих dispatch.

Затем `sys: JIT native signal` доставляется через обычную границу VM errors.
Сбойная операция не возвращается как declined fast path, не повторяется и не
восстанавливает REG из снимка. Функция не перезапускается с entry. Уже выполненные
scalar writes остаются выполненными. В обычном режиме ошибку может поймать Limbo
handler; непойманная ошибка обрабатывается штатно. Остальные гости продолжают
работу. В debug-режиме first-chance stop удерживает исходные managed stack,
roots и exception до handler/unwind; GC и остальные owners продолжают работать.

CPU snapshot — диагностическая копия, а не состояние CPU для возобновления.
`native-registers PID` возвращает signal context, architecture, actual PC/SP,
RX offset и объект именованных integer registers. `registers PID` по-прежнему
означает VM REG. `backtrace`, `operation`, typed reads и `census` читают исходное
Dis-продолжение. Числа-адреса не дают разрешения читать произвольную память.
Continue доставляет исходный error handler; step не повторяет сбойную операцию.
Следующая admitted debug instruction очищает прежнюю native-fault publication.
Nonlocal VM error path снимает устаревший RX landing до следующей операции.

Проекция CPU: x64 Linux gregset (23 поля), ARM64 x0..x30 и pstate, RV64 Linux
gregset (32 поля с PC). PC/SP выделены отдельно. Floating-point/vector registers
и произвольный native backtrace/frame retention этой проекцией не реализуются.
Dis backtrace удерживается и проверяется.

## Проверки и пределы

Test-only probe создаёт отдельные immutable RX trap pages, не изменяя production
module bytes/mappings. Без инъекции public guest — обычное корректное приложение.
Проверяются настоящие SIGILL, null-read SIGSEGV, SIGBUS после обрезания backing,
scalar write с последующим SIGILL и последовательные SIGILL/SIGSEGV в двух разных
функциях **одного owner**. Работают default/debug и оба poll paths. Kernel argument
register сопоставляется с actual native invocation. До release проверяются
original caller payload, census GC-корней и настоящая завершённая Fast-работа,
пока source удерживается. Требуются верные последующие результаты, quarantine,
normal catch и точное завершение source/collectors.

Пропуск copied CPU publication требует точного DETECTED при обычной семантике
приложения и retirement. Отдельные unsafe-path проверки требуют fatal SIGILL для
C helper и настоящего удержанного object guard, SIGABRT для реального незакрытого
native cleanup checkpoint. Это проверки отказа, а не доказательство recovery.
Core dumps запрещаются только для test driver и его дочерних процессов.

Локальные AMD64/ThreadSanitizer не доказывают новое выполнение ARM64/RV64: оно
проверяется обязательным workload `jit-signal`, список теперь63. C sanitizers не
инструментируют сгенерированные RX bytes. Уже повреждённая native memory, уход
управления за RX, произвольные host faults и сохранение карантина после restart
не входят в гарантию восстановления.

Ограничения POSIX: Linux man-pages [sigsetjmp](https://man7.org/linux/man-pages/man3/sigsetjmp.3.html)
и [signal-safety](https://man7.org/linux/man-pages/man7/signal-safety.7.html).
Проверка synchronous admitted RX PC нужна, чтобы не восстанавливать исполнение
после прерывания unsafe libc/helper call как после безопасного leaf.

Post-landing checks also compare original VM control/root/operand addresses.
A native test that corrupts REG.M before trapping must abort before managed
publication/handler; no saved REG is restored. / После landing сравниваются
исходные VM control/root/operand addresses. Реальная порча REG.M перед trap
должна дать abort до managed publication/handler; REG из снимка не восстанавливается.
