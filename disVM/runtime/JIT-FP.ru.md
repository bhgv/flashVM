# Аварии FP и диагностика базовых регистров

Встроенный RX guard теперь допускает базовые REAL scalar operations: MOVF, ADDF,
SUBF, MULF, DIVF и NEGF, вместе с прежними integer operations. Обязательны прежние
проверки root/control identity, ACTIVE admission, отсутствия object guards и
чистоты native cleanup. Нет глобальной остановки, exclusive profile и зависимости
от установленного внешнего debugger.

До допущенного вызова source сохраняет native floating environment. Ошибка
сохранения отклоняет native execution до effects. После synchronous RX fault и
успешных safety checks source восстанавливает исходное окружение **до** logging
и доставки VM error. Отказ восстановления запрещает recovery. Штатные native
returns сохраняют прежнюю FP-семантику: флаги не сбрасываются после каждой
успешной операции. Контракты managed-error cleanup сохраняются. Сбойная операция
не повторяется, вся emitted function переходит в interpreter quarantine.
Снимки VM REG не используются для восстановления исполнения.

## Копия базового состояния

В signal handler выполняются integer reads и lock-free stores. Нет FP arithmetic,
allocation, mutex, libc formatting или fenv calls. Копируются:

| Target | Базовые поля |
| --- | --- |
| x64 | 16 XMM128, восемь значимых80-битных x87 slots, control/status/compressed tags x87, MXCSR |
| ARM64 | 32 V128, FPCR/FPSR из bounded FPSIMD record |
| RV64 D | 32 F64 bit patterns и FCSR |

`native-fp PID` возвращает exact hexadecimal fields: architecture, format, width,
`coverage:"base"`. Ответ ограничен отдельно от `native-registers` и VM `registers`.
Валидность x87 slots/TOP определяется по tag/status: пустые slots не объявляются
живыми значениями. Нет floating conversion для NaN, extended precision и payload.

`JITFP pid=...` выводит те же именованные bits вне handler с привязкой к
JITSIGNAL/JITCPU/JITFAULT. Это kernel state в момент аварии; live source уже
восстановил прежнюю FP policy. Снимки не являются CPU continuation для resume
и не дают права читать произвольные системные адреса. Следующая debug instruction
снимает прежнюю publication, как и раньше.

ARM64 decoder обходит только4096-byte reserved region, проверяет16-byte geometry
и полный528-byte FPSIMD payload, пропускает bounded unknown records и никогда
не следует extra-context pointers. Отсутствие/повреждение base state явно
отмечается. RV64 decoder соответствует текущему D target ABI. Верхние XSAVE/AVX
lanes, SVE/SME, RVV и Q-extension state в base view не включены. SIMD guest
operations, произвольные host/helper faults, native corruption и native frame
retention остаются вне admitted scalar recovery.

## Проверки

Public REAL guest выполняет настоящие floating additions, проверяет результаты
и rooted caller data. Test-only RX pages загружают независимые low/high canaries,
меняют hardware rounding/flags и затем падают. На x64 отдельный случай настоящего
SSE divide by zero выдаёт SIGFPE. Остальные targets проверяют FP capture через
illegal instruction, без предположения о доступных hardware FP trap enables.

Default/debug и fast/locked-poll требуют original Dis frames/payload/GC census,
настоящей Fast-работы при удержании source, верной последующей интерпретации,
настоящей peer native work и полного retirement. Kernel argument GPR и исходный
REAL opcode сверяются независимо. Проверяются последний vector register и x64
x87 ST0=1.0 в его80-bit encoding.

До handler source обязан наблюдать исходный downward rounding и inexact flag,
несмотря на другую policy, установленную RX-кодом. Два safe omissions независимо
обнаруживают потерю copied FP publication и пропуск fenv restoration при штатных
catch/results/retirement приложения. Copied flags сохраняют fault-time values,
а не восстановленные значения live source.

Отдельный record unit проверяет560 truncations, неверные lengths, unknown-record
skip, отказ переходить по invalid external pointer, final-vector bits и response
bounds. Это проверка decoder, не actual ARM64 signal delivery на другом CPU.
Host ASan/UBSan использует detect_leaks=0: ptrace этой среды мешает LeakSanitizer.
Memory/UB instrumentation сохранена; allocation-leak coverage не заявляется.
Generated RX bytes не инструментируются C sanitizers.

Предыдущая CI43622919835 прошла все шесть jobs. Новый workload `jit-fp`, список64,
требует собственной AMD64/ARM64/RV64 матрицы; локальные AMD64 проверки её не заменяют.

ABI: [glibc x64](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/x86/sys/ucontext.h),
[Linux ARM64](https://github.com/torvalds/linux/blob/v6.12/arch/arm64/include/uapi/asm/sigcontext.h),
[glibc RV64](https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/riscv/sys/ucontext.h).
Upstream/vendor source для этих проекций не изменялся и не импортировался.

The test probe resets its own policy to defaults only AFTER observing the
pre-handler result, including omission runs; this keeps the negative control
safe and is not production repair. / Test probe возвращает default policy
только ПОСЛЕ наблюдения pre-handler результата, включая omission runs.
Это граница безопасного контроля, не production repair.
