# Обновление Dis/JIT и запуск IDE

[DisVM](README.ru.md) · [English summary](#english-summary) ·
[Состав снимка](UPSTREAM.json) · [Ядро IDE](../ide/dis/README.ru.md)

Источник — https://github.com/bhgv/Inferno-OS-bhgv, ветка `experiment`.
Снимок обновлён с `3e3536578b969cf1a460210d8ec667713869ccfc` до
`24892373ee09a2a23dc39b9dedc01fb7af61d8c1`. Это точный коммит,
полученный при обновлении; обычная сборка не скачивает меняющийся HEAD.

## Что перенесено

В 64-битных leaf JIT обновлены операции CALL, MOVPC, TCMP, фиксированная
арифметика и WORD32-адресация. RISC-V различает возможности умножения и деления;
добавлены реализации умножения для RV64I. Исправлены выравнивание и жизненный
цикл кадров стека, публикация ссылок при вызовах, возвратах и исключениях,
строковые операции, ALT и результаты операций Sys.

Манифест содержит 153 файла вместо 109. Помимо прежнего ядра добавлены только
выбранные регрессии и их зависимости: преобразование рун, форматирование lib9
и один OS-файл обработки исключений. Последний используется тестом извлечения
тел функций, не собирается как ОС. Приложения, Draw/Tk/Prefab, графический сервер
и полные платформенные деревья Inferno не импортированы. Каждый upstream-файл
сохранён побайтово и имеет SHA-256. Наши адаптеры находятся вне `upstream/`.

## Насколько готов запуск IDE

| Часть | Текущее состояние | Что требуется для запуска |
| --- | --- | --- |
| Документ IDE | Limbo-сервис `idecore.dis`: текст, правки, поиск, координаты, Undo/Redo, типизированные запросы/ответы | Хост должен загрузить модуль и вызвать `start`, затем обслуживать каналы |
| Интерпретатор и JIT | Собирается статический архив с настоящими upstream-объектами | Самостоятельного исполняемого хоста пока нет |
| Загрузка | Источники байтов и адаптер настоящего парсера уже существуют | Связать разрешение импортов, подписи, регистрацию модулей, экземпляры и время жизни с хостом |
| Процессы и каналы | Есть отдельные адаптеры потоков ОС, TLS `up` с вложенным `REG`, проверки каналов | Подключить `newprog`, очередь выполнения, блокировку/пробуждение, spawn, завершение и исключения к настоящему исполнению |
| Память | Есть upstream GC, координация mutator/collector и отдельные workers | Провести корни, владение и безопасные остановки через все исполняемые пути и блокирующие вызовы |
| Внешние модули | Интерфейсы `.m` и внешние заготовки описаны отдельно | Реализовать согласованные builtin-кадры и нужные модули; не подменять неизвестные вызовы успешными заглушками |
| Интерфейс IDE | Канальный протокол документного сервиса есть | Реализовать виджеты, ввод, сохранение, компиляцию/отладку и клиент FlashVM/AS |
| Собственный Limbo | Разрабатывается Coco/R-фронтенд и дальнейший конвейер | Полная компиляция произвольного IDE Limbo в Dis собственным компилятором ещё не подтверждена; пока используется явный bootstrap из `tools/limbo-bootstrap` |

`idecore` не импортирует Sys, Draw, Tk или Prefab. Для первого запуска его
документного сервиса не требуется переносить эти подсистемы Inferno.
Тестовые программы могут импортировать Sys для вывода и завершения; это
дополнительный контракт хоста, а не зависимость самого документного сервиса.

Список неопределённых символов архива включает, например, `newprog`, `currun`,
`addrun`, `delrun`, `acquire`, `release`, `kprotect`, `error`, `kopen`, `kread`
и Sys-функции. Часть символов предоставляет стандартная библиотека или наш
адаптер. Само наличие архива не доказывает готовность связанного приложения.
Ненужные графические/security builtin-декларации требуют явного исключения
из регистрации или соответствующего отказа; реализации этих подсистем сюда
не должны попадать ради удовлетворения линкера.

Первый критерий готовности: запустить `idecore.start`, передать Open → Snapshot →
Replace → Undo → Shutdown, получить реальные ответы через каналы и корректно
освободить процесс/модуль. Далее подключаются клиент FlashVM и инструменты IDE.
Внешний `emu` не является целевым решением этой интеграции.

## Проверка и повторение

Linux AMD64, GCC 13.3, CMake **3.10.2**, `DVM_NATIVE_REGISTERS=ON`:
архив ядра и общий хост собраны. **39/39 тестов прошли**. Набор включает: прежние проверки
плюс 15 новых драйверов для стека, вызовов, исключений, строк, Sys и покрытия JIT.
Отдельные нативные тесты сохраняют TLS `up`/`REG`, остановку мира и GC workers.
WORD32/WORD64 в фикстурах — модели полей VM, а не запуск 32-битной ОС.
Исполнение сгенерированного кода проверяется на AMD64. Проверка покрытия
эмиттеров ARM64/RISC-V64 не заменяет исполнение на соответствующих процессорах.
Запуск IDE и Windows/macOS/Android этой синхронизацией не подтверждены.

```sh
# Команда использует уже полученный локальный checkout исходного репозитория.
python3 tools/vendor-sync.py disVM --update \
    --source /path/to/Inferno-OS-bhgv \
    --ref 24892373ee09a2a23dc39b9dedc01fb7af61d8c1

# Проверка точного состава и контрольных сумм не обращается к сети.
python3 tools/vendor-sync.py disVM

# Старый CMake 3.10 поддерживает этот синтаксис конфигурации и сборки.
cmake -HdisVM -Bbuild/disvm \
    -DDVM_NATIVE_REGISTERS=ON \
    -DDVM_FLASHVM_SOURCE_ROOT="$PWD/flashVM"
cmake --build build/disvm -- -j2
(cd build/disvm && ctest --output-on-failure)
```

## English summary

The selected interpreter/JIT snapshot is pinned to experiment commit
`24892373ee09a2a23dc39b9dedc01fb7af61d8c1`. Its 153 files retain original
bytes, notices and hashes; no complete Inferno OS or GUI is imported.
Changes cover native leaf operations, RV64I multiplication, stack/exception
alignment, reference publication and Sys/string handling. Fifteen regression
drivers and their exact dependencies extend the native-register test suite to
39 tests. AMD64 execution and ARM64/RISC-V emitter coverage are distinct claims.

DisVM remains a static core plus host adapters, not an executable IDE runtime.
The document service itself needs no Sys/Draw/Tk/Prefab implementation. Missing
integration includes module instances/imports, process queues and lifecycle,
channel wakeups, exception boundaries, complete GC ownership/safepoints, builtin
ABI registration and the FlashVM/AS client. The first end-to-end acceptance is
Open/Snapshot/Replace/Undo/Shutdown through actual Limbo channels, followed by
clean process/module teardown. This update does not claim that acceptance.
