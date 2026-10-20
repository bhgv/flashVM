# Проигрывание логической сцены без окна

[English](OFFSCREEN_PLAYER.md) · [Кадры](FRAME_CAPTURE.ru.md) · [План](ROADMAP_STATUS.md).

Адаптер без окна владеет отдельной ссылкой SDL timer и проверяет геометрию
растра по общему контракту. Он не создаёт окно/renderer/texture, не запускает
video/events, не читает очередь SDL приложения, не запрашивает и не меняет
курсор. Закрытие освобождает только собственную ссылку. Размеры логической
сцены ограничены программным растеризатором.

Результат — программный RGBA-растр. Это не захват GPU и не виджет операционной
системы. SDL остаётся зависимостью сборки/линковки для часов, потоков и
необязательного звука. Сам адаптер не означает приёмку устройств или других ОС.

## Публичные функции (v0.720)

Новые C89-функции объявлены в [frame.h](../frontend/common/frame.h). Структуры
наблюдателей и прежние функции с окном сохраняют ABI и поведение. Достаточно
подключить player.h/frame.h; SDL-типы в публичных аргументах отсутствуют.

| Функция | Аргументы и назначение |
| --- | --- |
| [fvm_desktop_run_source_offscreen](../frontend/common/frame.h#L95) | Обязательны options, source и frame observer; loader/control и error необязательны. Нулевой loader сохраняет обычный бюджет только стартовой загрузки; дочерним запросам нужен явный loader. |
| [fvm_desktop_run_source_background_offscreen](../frontend/common/frame.h#L100) | Обязательны options, source, background provider, load monitor и frame observer; control/error необязательны. Monitor выбирает асинхронные проходы владельца либо блокирующее ожидание ресурсов; явный provider поставляет стартовые и дочерние байты. |

Выполнение синхронное на потоке владельца VM. Сохраняются FILE/MEMORY/URL,
разрешение origin, смена корня, таймеры, GC safe points, дедлайны/отмена и
pause/resume/step/STOP. Сетевые относительные пути разрешаются относительно URL,
linked-байты сохраняют явно заданный origin. Исходный буфер живёт до возврата;
frame/pixels/source заимствованы только внутри observer. Там следует вызвать
проверенное копирование. Текст origin нужно копировать отдельно от пикселей.
Политики callback/userdata живут до возврата. Callback не входит повторно в VM/
плеер и не уничтожает SDK.

SDL video/events не запускаются. События чужого окна, фокус и quit/Escape не
читаются; закрытие окна автоматически не останавливает этот запуск. Native input
не доставляется, HostInput содержит нейтральные состояния клавиш и указателя.
Запросы курсора не меняют курсор приложения. Инъекция ввода и GUI-адаптер в этой
версии не добавлены. IDE может копировать кадры и управлять плеером через свой
синхронизированный mailbox.

Планировщик сохраняет реальную частоту фильма/options.fps. Это не максимально
быстрый offline encoder и не API возврата после одного такта. Для конечного
запуска задайте frames/once либо callback STOP. Изначально приостановленный
плеер без возобновляющего controller останется на паузе. Растеризация и callbacks
могут занимать время; жёсткий timeout выполнения не обещается.

Формат кадра и счётчики тактов прежние. REFRESH обозначает начальный/грязный
растр на паузе, TICK — завершённое продвижение host timeline. Счётчики presentation
в этих функциях считают логические растры без native swap. Observer может
игнорировать ненужные кадры. Его ошибка останавливает после уже выполненных
VM/рисования; активные resource workers отменяются и join-ятся до освобождения.

Звук независим: mute подтверждает события без открытия устройства. При включённом
звуке требуется поддерживаемый backend/device и сохраняются прежние ошибки.
Новый режим не добавляет отсутствующие кодеки, streaming/video или обработку
Android lifecycle/permissions.

Результат 0 — штатное завершение/STOP; 1 — ошибка source/VM/raster/provider/
observer; 2 — неверные аргументы до source/provider/VM startup. Error заимствует
статический текст. Собственные worker/context/raster/timer освобождены к возврату;
копия пикселей остаётся собственностью C-хоста. Прежнее владение SDK сохраняется:
shutdown вызывается лишь после освобождения ссылок хоста на SDK, transfers и
mailboxes. Runner не завершает чужие SDL-подсистемы приложения.

## Сборка и проверка

Нужны [desktop SDK](DESKTOP_FRONTEND.ru.md), SDL2 2.0.9+ и graphics-enabled профиль.
SDL линкуется и при отсутствии окна. Из корня workspace:

```sh
cmake -S flashVM -B build-offscreen -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2
cmake --build build-offscreen --target flashvm_frontend_offscreen_window \
  flashvm_frontend_offscreen_capture flashvm_frontend_host_abi --parallel
ctest --test-dir build-offscreen -R 'offscreen|frontend_host_abi' --output-on-failure
```

Shared-хост: добавьте FVM_PLAYER_HOST_SHARED=ON и runtime library search path по
[правилам владения](PLAYER_HOST_OWNERSHIP.ru.md). Cross: укажите
-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake и SDL SDK именно целевой OS/CPU.
SDL/audio/HTTP заголовки и библиотеки соответствуют target. Тесты запускаются
на target, не при cross compilation. Android по-прежнему требует явного
application owner/lifecycle adapter из общего руководства.

Адаптерный тест задаёт недоступный видеодрайвер, проверяет владение чужими
SDL-подсистемами и оставляет событие приложения в очереди. Реальные SWF в
interpreter/JIT проверяют белый начальный растр, шаг/паузу/перерисовку, смену
корня, файлы/linked-буфер/HTTP и отмену загрузки при ошибке observer. Прежние
native-window тесты с dummy video проверяются отдельно. Эти результаты не
являются приёмкой native устройств Windows/macOS/Android.
