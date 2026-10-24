# Один протокол для AS-интерфейса и Limbo-сервиса

[Контракт RU](../../UI-BRIDGE.ru.md) · [Contract EN](../../UI-BRIDGE.md)

`bridge.as` — подробно прокомментированный AS2-кодек и неблокирующая оболочка.
Хост создаёт `DVM_UIBridge`, получает UI-пару через `dvm_ui_channels`, оборачивает
оба канала `fvm_extensions_wrap_channel` и публикует удерживаемые глобальные
объекты `UiTx` и `UiRx`. Сначала укореняется первый wrapper, затем создаётся второй.
После загрузки функции создаётся `makeUiBridge(UiTx, UiRx)`.
Эта заготовка не создаёт окна или TextField и не содержит готового SWF-редактора.

`uiecho.m`/`uiecho.b` — настоящий Limbo-модуль без Sys/Draw/Tk. Сервис получает
кодек `Uibridge` и два обычных `chan of array of byte`. Операция 1 возвращает тот
же текст в Reply с исходным id; операция 2 сначала отвечает, затем завершает
сервис. `<-` и `<-=` блокируют процесс Limbo штатным планировщиком. Неверная
запись/операция возвращает -1 вызывающему хосту, а не фиктивный успешный ответ.
Исходник комментирует каждую строку учебного сервиса.

Кодек находится в `disVM/limbo/uibridge.b`, интерфейс — в `disVM/module/uibridge.m`.
Он сериализует поля в независимый массив байтов и проверяет точное обратное
преобразование Unicode. C-адаптер создаёт такой массив в куче Dis; AS создаёт
собственную строку в куче FlashVM. Кучи и их сборщики не смешиваются.

## Сборка модулей

Из корня проекта, после сборки собственного исходного bootstrap:

```sh
# Source-built temporary bootstrap; our Coco/R Limbo compiler remains the priority.
./build-workspace.sh --projects limbo-bootstrap

# Set the actual output path used by your selected build engine.
LIMBO="$PWD/build/workspace/limbo-bootstrap/limbo-bootstrap"
mkdir -p build/ui-channel
"$LIMBO" -I disVM/module -o build/ui-channel/uibridge.dis disVM/limbo/uibridge.b
"$LIMBO" -I disVM/module -I disVM/examples/ui-channel \
    -o build/ui-channel/uiecho.dis disVM/examples/ui-channel/uiecho.b
```

Либо передайте явный `-DDVM_LIMBO_EXECUTABLE=/absolute/path/to/limbo-bootstrap`
при CMake-конфигурации `disVM`: цель `disvm_ui_modules` соберёт оба модуля.
Команды конфигурации совместимы с CMake 3.10: `cmake -HdisVM -Bbuild/disvm ...`.
Нативная граница Dis собирается при `-DDVM_NATIVE_REGISTERS=ON` и явном
`DVM_FLASHVM_SOURCE_ROOT`. Тесты с реальной VM требуют также явного списка
архивов и зависимостей `DVM_FLASHVM_LIBRARIES`; он должен соответствовать профилю
FlashVM. Нельзя заменять отсутствующие библиотеки тестовыми реализациями каналов.

При интеграции хост регистрирует точные типы Dis-каналов и обслуживает обе стороны:
читает UI-очередь, удерживает недоставленную запись при `FVM_NOT_FOUND`, предлагает
её через `dvm_ui_dis_offer`, принимает ответ обычным Dis-путём и вызывает
`dvm_ui_dis_export`. Экспорт также может требовать повторной попытки. Нельзя
удалить сообщение из очереди и забыть его при первом отказе доставки.

После close необходимо остановить/join работников и только затем освобождать
каналы. Shutdown учебного сервиса — подтверждаемая операция протокола; транспортный
close — прекращение работы самого соединения. Это разные действия.

## Проверки

`disvm_ui_bridge` проверяет реальную FlashVM и её AS wrappers, ввод, квоты и
rendezvous. `disvm_ui_dis_channels` связывает настоящий Dis mover/heaparray с
реальным FlashVM transport; scheduler/pool остаются фикстурными границами.
Для последнего теста используется Linux native ABI и явные архивы, не cross-build.
Опционально `node disVM/tests/ui_codec.js` проверяет алгоритм AS2-кодека.
Разбор AS2-фронтендом и компиляция Limbo не доказывают запуск полной IDE.

## English

The AS2 helper and Limbo service share the documented value envelope. The host
provides two rooted AS channel wrappers and registered Dis byte-array channels.
The teaching service echoes operation 1 and acknowledges operation 2 before
shutdown, using ordinary blocking Dis channels. The sample is not a finished
editor or standalone runnable host. Compile the codec/service with an explicit
source-built bootstrap as shown above, or select DVM_LIMBO_EXECUTABLE in CMake.
Keep pending records on backpressure, preserve ownership/order, and quiesce all
workers before destruction. Tests distinguish real channel/mover execution from
fixture scheduler boundaries and whole-application execution.
