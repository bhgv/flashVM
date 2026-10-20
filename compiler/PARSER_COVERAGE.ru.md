# Парсеры: серия 0.6.0

Закрыты конкретные лексические пробелы и неоднозначности. Полное соответствие всем версиям пяти языков пока не подтверждено; распознавание синтаксиса отделено от AST, семантики и генерации объектного кода.

[Подробный лексический профиль](LEXICAL_PROFILE.ru.md)

| Parser | Version | Current coverage |
| --- | --- | --- |
| AS1 | 0.4.0 | Expressions/statements, legacy operators, ASI, authoring wrappers, Unicode/escapes |
| AS2 | 0.4.0 | AS1 syntax plus classes/interfaces/types, Unicode/escapes |
| AS3 | 0.5.0 | Packages/classes/interfaces, namespaces, metadata, ASI, E4X, Vector literals, CONFIG blocks, Unicode/escapes, structural regexp validation |
| Limbo | 0.4.2 | Modules/ADT/polymorphism, fixed, exceptions, channels, casts, slices, exports and brace-containing qualifiers |
| FIR syntax | 0.1.1 | FIR 1.0 modules/declarations, composite types, functions, SSA/block/edge parameters, invoke/debug records |

Все ATG авторитетны и форматируются для чтения человеком. Встраиваемые C99 API:
`as1_check/read`, `as2_check/read`, `as3_check/read`, `limbo_check/read`,
`firsyn_check/read`. Они не печатают и не завершают хост; ввод принадлежит
вызывающему. Доступ к одному парсеру сериализуется; reentrancy/thread safety
не обещается. CLI — отдельные адаптеры. Лимиты: 2 MiB, глубина 512,
4 000 000 посещений guarded productions; guard regexp использует тот же счётчик.

FIR tools имеют версию 0.5.1. Исполняемый FIR остаётся отдельным проверяемым
подмножеством: `fir1 syntax` не обходит verifier/codegen. Генератор создаёт SWO,
линкер собирает SWF. Расширенный структурный FIR не превращается автоматически
в исполняемый IR. Версия проигрывателя не менялась.

Остаются AST, типы/контекст/lvalue, полная совместимость regexp-движка,
XML namespace/entity resolution, include expansion, верификация/понижение
расширенного FIR. Лексический профиль Unicode зафиксирован на 3.2.0 с
описанными расширениями; это не заявление точного профиля всех Flash Player.

Проверка Linux x86-64: 27 CTest-групп compiler, 7 FIR, отдельный хост пяти
C API и 27 compiler-групп с UBSan. Отделённые переименованные копии также
собраны и проверены. ARM64 Linux/musl, RISC-V64 Linux/musl и Windows x86-64/GNU
собраны повторно; проверены ELF/PE machine IDs, запуска на целях нет.
Кросс-сборки использовали адаптированный к этой среде Zig 0.13.
Использованы Coco/R b91b652 + отдельный errno fix; внешние исходники не
включены в FlashVM. ASan, Android/macOS execution и полный conformance suite
в этой серии не заявляются.

База FlashVM: 39eb951c867382d27eb05dd151e3ed2243e50c28 (master 0.5.2).
Старые отчёты [VALIDATION](VALIDATION.ru.md) относятся к своим версиям.

## Build / Сборка

```sh
cmake -S compiler -B build-five \
  -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor \
  -DFIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend
cmake --build build-five
ctest --test-dir build-five --output-on-failure
```

[Native/cross build guide](../BUILDING.ru.md) ·
[Five-API C host](tests/embed_five.c) · [Examples](examples.ru.md) ·
[AS1 ATG](as1/frontend/as1.atg) · [AS2 ATG](as2/frontend/as2.atg) ·
[AS3 ATG](as3/frontend/as3.atg) · [Limbo ATG](limbo/frontend/limbo.atg) ·
[FIR ATG](../fir/v1/frontend/firsyn.atg).
