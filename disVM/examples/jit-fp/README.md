# Floating JIT fault example / Пример floating JIT fault

```
build/disvm-opt/bootstrap/limbo-bootstrap -I disVM/upstream/module -I disVM/module -o fp.dis disVM/examples/jit-fp/main.b
build/disvm-opt/disvm --jit fp.dis
python3 disVM/runtime/test_jit_fp.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

EN: Ordinary execution verifies REAL9.0/14.0 and prints caught=0 second=0. The
production binary has no injected defect. The optional test client links separate
immutable RX canaries and a real fault at an actual floating-addition dispatch.
It compares copied FP/SIMD bit patterns, original stack/payload/GC roots and actual
pre-handler rounding/flags. Default/debug and both polls run; x64 also tests real
SSE SIGFPE. Missing copied FP and missing restoration controls require precise
DETECTED while the application catches its error and retires normally.

RU: Обычный запуск проверяет REAL9.0/14.0 и выдаёт caught=0 second=0. Production
binary не содержит injected defect. Optional test client связывает отдельные
immutable RX canaries и настоящую аварию на actual floating-addition dispatch.
Сверяются copied FP/SIMD bits, исходные stack/payload/GC roots и pre-handler
rounding/flags. Работают default/debug и оба poll paths; x64 дополнительно
проверяет настоящий SSE SIGFPE. Пропуски copied FP/restoration требуют точного
DETECTED при штатном catch и retirement приложения.

EN: `native-fp PID` reads the retained copy. Coverage is base XMM/x87, FPSIMD or
RV64 D, not wider vector extensions. Python is only the optional host client.
RU: `native-fp PID` читает retained copy. Coverage — base XMM/x87, FPSIMD либо
RV64 D; более широкие vector extensions не включены. Python нужен только host client.

[EN contract](../../runtime/JIT-FP.md) · [Контракт RU](../../runtime/JIT-FP.ru.md)
