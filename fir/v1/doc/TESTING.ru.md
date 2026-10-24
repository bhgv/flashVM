# Проверки

[English](TESTING.md). [check.py](../tests/check.py) — acceptance suite компилятора.
CTest проверяет отрицательные синтаксические/семантические случаи, все примеры
до/после peephole, round trip и независимо разбирает framing SWF/actions.
Проверяются повторы SSA, доминирование, метки, типы, версия, UTF-8, комментарии,
границы, неподдержанный source, дальние переходы и сохранение прежнего выхода.

Реальное исполнение проверяется отдельно и явно отмечается:

```sh
cc -std=c99 -IflashVM/include -c fir/v1/tests/run_flashvm.c -o /tmp/fir-run.o
c++ /tmp/fir-run.o VM_BUILD/libflashvm.a \
  VM_BUILD/libfvm_zlib.a -lpthread -ldl -lm -o /tmp/fir-run
FIR_SWL="$PWD/build/linker/swl" FIR_FLASHVM_RUNNER=/tmp/fir-run ctest --test-dir build/fir1 --output-on-failure
```

`VM_BUILD` — каталог существующей совместимой Linux static сборки FlashVM;
её параметры могут потребовать дополнительные зависимости. Хост устанавливает
расширения и callback ActionTrace, загружает SWF и выполняет DoAction через
публичный C API. Trace сравнивается с ожиданиями до и после оптимизации:
16 пар полноразрядного знакового умножения, динамическая конкатенация, double,
оба исхода if и передача по каналу. Хост не линкуется в компилятор и не требуется
для отдельной сборки. Без FIR_FLASHVM_RUNNER CTest не заявляет runtime-проверку.

Для sanitizer-сборки задайте C/linker flags
`-fsanitize=address,undefined -fno-omit-frame-pointer` и запустите тот же suite.
Инструкции Termux/Windows не означают свежей проверки на физических устройствах.

## Проверки v0.611

Linux/C99: прошли 335 запусков с реальным хостом FlashVM, для исходного и
оптимизированного SWF. ASan/UBSan: те же 335 проверок прошли с
`ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer недоступен при трассировке процессов
в этой среде. Проверка утечек этим не заявляется. Прошли сборка переименованной
отдельной копии в пути с пробелами и локальная сборка из compiler/.
Корневой и шесть локальных launchers отдали по loopback HTTP главную, русский
формат, пример каналов и грамматику; все локальные ссылки FIR разрешились.
Использована ревизия внешнего Coco/R: `b91b65233c264d92c63a5c8cd855e7fc784bca53`.

## Серия компиляторов 0.2.2

Linux, GCC 13.3, C99: 472 проверки подпроцессов с настоящим FlashVM host,
до и после оптимизации, прошли. Те же 472 прошли с ASan/UBSan для FIR и
source-адаптера. В этой среде отключены ASan stack instrumentation и LSan;
проверены heap/global ASan и UBSan, это не проверка утечек или стека.
Четыре CTest-набора синтаксических фронтендов прошли. Отдельные переименованные
копии AS1, AS2, AS3, Limbo и FIR в путях с пробелами собраны и проверены.
Проверено 512 локальных Markdown-ссылок без пропусков. Coco/R revision:
`b91b65233c264d92c63a5c8cd855e7fc784bca53`. Физический Android не проверялся.
Для среды без /tmp задайте существующий доступный TMPDIR; временные файлы
создаются mkstemp и сразу удаляются из каталога, оставаясь открытыми потоками.

## SWO toolchain 0.3.2 / SWL 0.3.1

Текущий compiler-only CTest проверяет SWO1. Для сборки SWF задайте
`FIR_SWL` (полный путь к `swl`); для выполнения — также `FIR_FLASHVM_RUNNER`.
Нельзя задавать runner без линкера. Верификация/оптимизация выполняется до
объектной генерации; собственно SWF создаётся только SWL.

Linux GCC 13.3 C99: прошли все 15 CTest-тестов (FIR 3, frontends 9, linker 2,
IDE 1) в обычной и ASan/UBSan-сборке. FIR: 558 проверок подпроцессов с настоящим
FlashVM runner до/после оптимизации. SWL: 96 проверок объектов, библиотек,
ресурсов и ошибок, включая все 16 сочетаний PCM rate/channels/sample width.
C embedding-тесты проверяют повторные вызовы после ошибок, совместную линковку
всех парсеров, snapshots, некорректные C-структуры IR, откат архива и повторный
вывод после отказа callback. IDE SWF выводит 42 и 36 через FlashVM.
Отделённые переименованные AS1/AS2/AS3/Limbo/FIR/SWL собраны и протестированы
без скрытых sibling-зависимостей. Санитайзерный host отключает LSan и ASan
stack instrumentation; проверены heap/global ASan и UBSan, не утечки/стек.
Ресурсный SWF загружен VM; физическая отрисовка/аудиовывод не проверялись.
Coco/R revision: `b91b65233c264d92c63a5c8cd855e7fc784bca53`.

[0.5.3 lexical-source and IDE continuation acceptance](../../../ide/dis/SERIES_0_3_1.md).
