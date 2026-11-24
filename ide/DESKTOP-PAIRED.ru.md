# Рабочий стол из пункта 8 на двух VM

[English](DESKTOP-PAIRED.md) · [Точки входа](https://github.com/bhgv/flashVM/blob/master/ide/ENTRYPOINTS.md)

Обычная сборка DisVM теперь выпускает тот же `desktop-ide.dis`, который
использует пункт 8. Владельцы панелей, компоновка, диалоги и работа с файлами
остаются в существующем Limbo-коде. В графическом пакете пункт 8 вызывает
`run-desktop-ide.sh`, который запускает `disvm-editor` для отрисовки всего стола.

## Что проверено на паре VM

При сборке с терминальным SDK DisVM также выпускает `desktop-paired.dis`.
Его запускают из каталога с модулями и `desktop-view.swf`, смонтировав
плеер в `/rpc`. Стартовый Limbo-модуль передаёт SWF через `/rpc/.upload`,
пишет `start` в `/rpc/.state` и вызывает `DesktopIDE.init()` в том же
процессе. `$Unibus.attach` передаёт сцены и ввод через одну сессию libstyxrpc.

Проверочный SWF на ActionScript получает сцену с двумя редакторами, консолью
сборки и справкой. Он запрашивает ширину 860 вместо 960 по билету сцены, и
Limbo перестраивает панели в одну колонку. Затем SWF вводит `Яpaired` в первый редактор, дожидается этого
текста в следующей сцене и через штатный диалог сохраняет `paired-output.txt`.
Получив заголовок сохранённого редактора, он запрашивает обычный выход.
Проверка требует подтверждённого завершения от настоящего desktop и сверяет
байты созданного файла в отдельном рабочем каталоге. Проверены JIT и интерпретатор.

Из корня репозитория настройте `disVM/runtime` с собранным из тех же исходников
`DVM_TERMINAL_LAUNCH_SDK`. Соберите обычную VM и запустите проверку.

```sh
cmake --build build/disvm-runtime --target disvm disvm_desktop_paired
python3 ide/tests/check_paired_desktop.py --disvm build/disvm-runtime \
  --player build/flashvm/flashvm_styx_player --as2 build/as2/view/as2_viewc \
  --fir build/fir --swl build/linker/swl
```

Проверка собирает SWF по цепочке AS2 → FIR → SWO → SWL, копирует собранные
Limbo-модули во временные рабочие каталоги и выдаёт двум VM концы одного
соединения. Прикладная логика исполняется из Dis и SWF.

## Графический рабочий стол на паре VM

Соберите SWF предпросмотра теми же инструментами.

```sh
python3 ide/build_paired_view.py build/disvm-runtime/desktop-view.swf \
  --as2 build/as2/view/as2_viewc --fir build/fir --swl build/linker/swl
```

SWF обрабатывает `BeginView`, `ItemView` и `EndView`. Он собирает MovieClip на
невидимой глубине и показывает результат после совпадения билета и числа
элементов. Из координат сцены рисуются фон, полки и прямоугольники панелей.
У активной панели выделяется заголовок. Смещение, прокрутка и масштаб стола
берутся из записей сцены.
Штатные TextField показывают заголовки и содержимое панелей, а также подписи
элементов управления. Текст приходит в тех же записях сцены, что и координаты.
Каретка и выделение берутся из того же завершённого снимка редактора. FlashVM
сопоставляет позицию указателя с индексом символа Unicode по раскладке поля.
DisVM проверяет билет сцены и применяет правку.

Для текста в окне соберите плеер с профилем `flashVM/cmake/profiles/text-jit.defs`
и SDL2. Положите `ide/assets/fonts/NotoSansMono-Regular.ttf` по пути
`fonts/NotoSansMono-Regular.ttf` в каталоге запуска DisVM. FlashVM читает его
из живой VFS DisVM через парное соединение при загрузке фильма. Плеер без
поддержки шрифтов или без файла показывает геометрию панелей.

С `--window` доступны клавиатура, ввод Unicode, выделение мышью и вертикальная
прокрутка. Ctrl+колесо передаёт штатную команду масштабирования. При потере
фокуса Dis сбрасывает зажатые модификаторы. Закрытие окна отправляет обычный
запрос Quit с проверкой несохранённых правок. SWF опрашивает очередь `hostInput`
и передаёт события по тому же соединению UniBus, по которому получает сцену.

Проверка из репозитория с `--text --window`
публикует шрифт в VFS и сверяет пиксели текста редактора и консоли после
изменения ширины и правки. Параметр `--capture` отдельно проверяет геометрию.
Проверочный код на ActionScript линкуется отдельно с тем же
исходником отрисовки. В рабочем SWF сценарий ввода и сохранения отсутствует.
В архиве `desktop-view.swf` открывает рабочий стол с вводом, а `desktop-check.swf`
проходит проверочный сценарий. Там же лежат шрифт и его лицензия.
Для повторения проверки положите второй
фильм под именем `desktop-view.swf` в отдельную копию каталога.

В Linux проверка ставит в очередь SDL2 события клавиатуры, фокуса, текста и
указателя. Рабочий SWF с драйвером SDL `dummy` редактирует и сохраняет
`Я🙂paired`. Проверка сравнивает пиксели выделения и каретки после
масштабирования Ctrl+колесом в обоих режимах.

```sh
cc -shared -fPIC $(pkg-config --cflags sdl2) ide/tests/paired-sdl-input.c -ldl -o build/paired-sdl-input.so
SDL_VIDEODRIVER=dummy python3 ide/tests/check_paired_desktop.py \
  --disvm build/disvm-runtime --player build/flashvm/flashvm_styx_player \
  --as2 build/as2/view/as2_viewc --fir build/fir --swl build/linker/swl \
  --native-events build/paired-sdl-input.so
```

Изменение размеров окна пока масштабирует авторскую сцену. Остаётся связать
это событие с новой компоновкой Dis, поддержать жесты с несколькими касаниями,
MDI, живые миниатюры, векторный поток и чернила. Пункт 8 по-прежнему запускает
установленный графический runtime.

[Стартовый модуль Limbo](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Контроллер](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[Проверка на ActionScript](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
