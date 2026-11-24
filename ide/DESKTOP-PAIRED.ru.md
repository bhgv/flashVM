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
сборки и справкой. Он вводит `Яpaired` в первый редактор, дожидается этого
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

## Что требуется для всего графического стола

На паре уже проходят реальные записи сцены. Отдельному SWF ещё нужно
превращать их в иерархию MovieClip и TextField, передавать события окна и ввода
в UniBus и подключить метрики шрифта, векторный поток и чернила. Пока эти
возможности пункта 8 обслуживает установленный графический runtime.

[Стартовый модуль Limbo](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop-paired.b) ·
[Контроллер](https://github.com/bhgv/flashVM/blob/master/ide/dis/app/desktop.b) ·
[Проверка на ActionScript](https://github.com/bhgv/flashVM/blob/master/ide/tests/paired-desktop.as)
