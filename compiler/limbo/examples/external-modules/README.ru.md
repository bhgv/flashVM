# Один .m: внешний AS-модуль для Dis и AS-клиент Dis-модуля

[English](README.md) · [Приоритет компилятора](../../OWN_COMPILER.ru.md) ·
[Генератор](../../frontend/bindings.c)

Это двусторонние заготовки модулей Limbo/Dis. Общий контракт —
[arithmetic.m](arithmetic.m). Его реализация может быть Dis
([arithmetic.b](arithmetic.b)) либо внешним не-Dis AS-кодом
([as-implementation.as](as-implementation.as)). AS-клиент
([as-caller.as](as-caller.as)) получает Dis-прокси из того же заголовка.
C/Python/JSON используют те же идентификаторы деклараций.

## Генерация

```sh
./build-workspace.sh --projects limbo
mkdir -p build/external-modules
build/workspace/limbo/frontend/limbo_bindings --format as-module compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic-as.as --prefix arithmetic_as
build/workspace/limbo/frontend/limbo_bindings --format as-proxy compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic-dis.as --prefix arithmetic_dis
build/workspace/limbo/frontend/limbo_bindings --format c compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.c --prefix arithmetic_c
build/workspace/limbo/frontend/limbo_bindings --format python compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.py --prefix arithmetic_py
build/workspace/limbo/frontend/limbo_bindings --format json compiler/limbo/examples/external-modules/arithmetic.m build/external-modules/arithmetic.json
```

Есть as1-module/as2-module/as3-module и соответствующие *-proxy.
as/as1/as2/as3 — синонимы module-адаптера. Пока используется общий нетипизированный
AS-поднабор, проверяемый настоящими Coco/R-парсерами всех трёх версий.
Это не утверждение о полном понижении AS в SWF/ABC. `.m` разбирается существующей
Coco/R-грамматикой и concrete tree, без нового ручного/regex-парсера.

## Dis вызывает внешний AS

После загрузки generated arithmetic-as.as и as-implementation.as функция
makeArithmeticASModule() возвращает dispatch-capability внешнего Arithmetic.
Native bridge должен зарегистрировать её как реализацию разрешённой .m-сигнатуры.
Запрос несёт schema=1, engine="as", interface_id, положительный id, declaration,
module="Arithmetic", name="add", args=[3,4]. declaration берётся из generated
contract, а не поддерживается вручную.

Адаптер проверяет контракт/имя/экспорт/форму вызова, вызывает handler и отвечает
один раз: engine="as", тот же id/declaration, status и value/error. Неизменённые
заготовки явно отказывают как unimplemented. В примере сложение завершается
`| 0`: это i32 wrap, обычный AS Number не воспроизвёл бы семантику Limbo int.

## AS вызывает Dis

После загрузки arithmetic-dis.as useArithmeticDis(sendToDis) вызывает .m-экспорт
через generated proxy. sendToDis — явный проверяемый host/channel endpoint.
Запрос engine="dis" маршрутизируется к настоящему Dis-экспорту; bridge переносит
значения и возвращает ответ. AS получает callback, не блокируя event/render owner.
.call ищет модуль/экспорт по заголовку, .callById выбирает точную декларацию.
Неоднозначные короткие имена вложенных модулей отвергаются.

ID растут от 1 до 2147483647 без повторного использования внутри proxy. Чужие
schema/engine/header/declaration/id, неверный status и повторные ответы не
завершают другой вызов. Ошибка транспорта выдаёт один отказ. Native endpoint
владеет namespace и pending roots; ID не глобальны между разными proxy.
Replay cache/cancellation пока отсутствуют. Один ответ на dispatch не означает
exactly-once выполнение при повторной доставке запроса.

## Граница native bridge

JSON хранит точный header snapshot, декларации/типы и source positions. Все
форматы имеют interface_id — FNV-1a64 исходных UTF-8 байтов с версионным префиксом.
Это быстрый mismatch tag, не аутентификация и не native Dis MD5 structural signature.
Host обязан разрешить и проверить полный descriptor, includes/aliases/ADT layout
и настоящие runtime signatures до регистрации. Меняется .m — обе стороны нужно
перегенерировать.

Нужны проверка arity/types/ranges и перенос строк, записей, ref/chan/resources
как значений либо типизированных capabilities. AS Number не сохраняет произвольный
Limbo big. Raw pointers и Dis frames нельзя передавать SWF-объектами. Bridge
удерживает GC roots ожидающих вызовов, освобождает их по завершении и показывает
boundary frames отладчику. Proxy копирует верхний args-массив; сложные значения
требуют host snapshot/serialization. C-thunks используют opaque call и возвращают
-1 до реализации, без выдуманного native layout. Includes только каталогизируются.

Полный native Dis/SWF loader/marshaler и полный AS-source compiler ещё в работе.
Здесь нет заявления о настоящем cross-VM исполнении. Тесты компилируют C,
проверяют AS тремя Coco/R-парсерами и опционально исполняют сгенерированный общий
AS-код в Node с Dis transport double. Node — тестовый хост, не runtime-зависимость
IDE и не доказательство исполнения SWF.
