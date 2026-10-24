# Source-map 0.1.20 checkpoint

All seventeen Python test scenarios and the C embedding test pass with generated parsers from bhgv/coco-r-C and the C-output path of bhgv/coco-r-limbo a7364dcd. The same two CTest groups pass under ASan/UBSan. The source-map prefix contains 4096 independently compressed oracle records; a separate explicit-path harness compiles actual sblsrcconv from Inferno experiment 371e3a94 and passes another 4096 producer-generated records natively and under ASan/UBSan. Native Limbo generator execution was not rerun.

A renamed detached copy, with the root MIT LICENSE, configures/builds and passes both CTests using only explicit external generator paths. The .atg formatter and documentation targets/source-line checks pass. Generated parser globals are namespaced; library queries use owned immutable maps, not VM state. LeakSanitizer is disabled and executable-name warnings may limit sanitizer symbolization. Exhaustive allocation-fault cleanup and foreign/physical target execution are not accepted.

The twenty-commit format-patch series is checked against a clean published base, with consecutive component versions, both bhgv identities and final applied tree compared. No Dis runtime, Tk/Draw/crypto code, JIT backend, complete symbol reader or new compiler backend is imported. This is source-map infrastructure; types/functions/locals, debugger channels and full C/Limbo/AS compiler work remain open.

## RU

17 Python-сценариев и C embedding test проходят с parsers от bhgv/coco-r-C и C-output пути bhgv/coco-r-limbo a7364dcd. Те же две CTest-группы проходят ASan/UBSan. Проверяются 4096 независимо сжатых oracle records; отдельный explicit-path harness компилирует настоящий sblsrcconv из Inferno experiment 371e3a94 и проходит ещё 4096 producer records нативно и с ASan/UBSan. Native Limbo generator execution повторно не запускалось.

Переименованная отдельная копия с корневой MIT LICENSE собирается и проходит оба CTests только с явными внешними generator paths. .atg formatter и проверки documentation targets/source lines проходят. Generated globals имеют namespace; queries работают с собственными неизменяемыми maps, без VM state. LeakSanitizer отключён, executable-name warnings могут ограничивать символизацию. Полная allocation-fault cleanup и foreign/physical target execution не приняты.

Серия format-patch из 20 коммитов проверяется на чистой опубликованной базе с последовательными версиями компонента, обеими подписями bhgv и совпадающим итоговым деревом. Dis runtime, Tk/Draw/crypto, JIT backend, полный symbol reader и новый compiler backend не импортируются. Это source-map infrastructure; types/functions/locals, debugger channels и полные C/Limbo/AS компиляторы остаются открытыми.

## Query extension 0.1.24

Native C99 and ASan+UBSan builds using external coco-r-limbo C-host/C-output
both pass all three CTest targets (embedding, query API, Python source-prefix
suite). The Python suite now has 19 test methods, including 240 deterministic
randomized query/oracle comparisons and malformed CLI argument checks.
Sanitizer leak detection is disabled as in the previous checkpoint; this is not
an allocation-fault, thread-race, native Limbo generator or runtime Dis acceptance.
An initial test fixture used an incorrect literal byte length; corrected to strlen
before acceptance. No source behavior was changed to accommodate the fixture.
