# Backend сборки для IDE

Версия **0.2.0**. [English](README.md).

**Целевое приложение:** IDE на Limbo со встроенной Dis внутри FlashVM; управление UI только через каналы. Этот Python-драйвер — host-инструмент разработки. [Принятая архитектура](ARCHITECTURE.ru.md).

Это независимый host-backend сборки на Python 3.9+ для редактора, а не графическая IDE. Явно выбранные C99-компилятор, FIR tools и SWL остаются отдельными проектами. Сейчас доступны их численные профили AS1/AS2/AS3/Limbo; AS3 создаёт совместимый AVM1, а не ABC. Полный компилятор AS2 не заявляется.

## Запуск

Соберите инструменты по [руководству FIR](../fir/v1/doc/PIPELINE.ru.md). Затем укажите пути к исполняемым файлам — абсолютные либо относительно текущего каталога:

```sh
python3 ide/build.py program.as movie.swf --language as2 \
  --compiler /host/build/fir1_source --ir /host/build/fir1 \
  --linker /host/build/swl --module application
```

Для разобранного запроса stdout содержит один JSON-объект; exit 0 означает публикацию, 1 — возвращённую ошибку сборки. argparse help/недопустимые параметры используют обычный текст и exit 0/2. Инструменты запускаются без shell и stdin; пробелы, Unicode и shell metacharacters в путях остаются символами пути. Это должны быть доверенные host executables, а не cross-built target binaries. Неявных скачиваний и сборок нет.

Опции `--runner /host/flashvm_runner --expect-trace TEXT` требуют успешного headless-запуска и точного stdout до публикации. В shell строка ожидания должна содержать настоящий перевод строки, например внутри многострочных кавычек; буквальный backslash-n не преобразуется. `--no-optimise` убирает оптимизацию и повторный verifier; `--timeout 30` задаёт конечный положительный лимит каждой стадии.

## Транзакции и владение процессами

До компиляции читается ограниченный снимок исходника до 64 KiB. Приватный каталог рядом с output владеет промежуточными FIR, optimized FIR, SWO и SWF. Каждый child получает этот каталог как cwd, TMPDIR, TEMP и TMP: неверный ambient temp path не ломает staging компилятора. Source, verifier, optimizer, verifier, object emitter и linker выполняются последовательно. После ошибки следующие стадии не запускаются. Проверяются SWO1 и несжатый SWF8 с совпадающей заявленной длиной; это базовый admission артефакта, а не полная валидация SWF.

Только после всех выбранных стадий os.replace публикует movie в той же файловой системе. Прежний output сохраняется при всех ошибках до публикации, включая несовпадение trace. Отклоняются outputs, совпадающие с source/tool по пути или hard link, и symbolic-link output entries. У параллельных сборок в один output побеждает последняя успешная публикация; блокировки не устанавливаются. Атомарная замена не гарантирует сохранность при потере питания и зависит от filesystem semantics. Вход может быть symlink; диагностика указывает разрешённый путь.

При timeout/cancellation прямой child завершается и дожидается reap. Группы потомков не управляются; выбранные инструменты не должны отделять children. KeyboardInterrupt становится возвращённой диагностикой отмены. Файловые дескрипторы и процессы вызывающей стороне не передаются. Другие Python exceptions вне документированных request/OS ошибок могут передаваться embedding host.

## JSON и Python API

Синхронная функция [`build`](build.py#L103) возвращает данные, принадлежащие вызывающей стороне, и не меняет глобальное окружение. Поля результата: schema=1, ok, published, steps, diagnostics; допустимый ввод добавляет canonical source/output, language и module, успешная публикация — sha256. Стадия содержит stage, argv, returncode, ограниченные stdout/stderr и truncation flags; timeout добавляет timed_out. В результате сохраняется до 65536 байт каждого потока с replacement decoding некорректного UTF-8. Temporary log files не имеют disk quota. JSON содержит локальные executable/temp paths; хранение и распространение выбирает вызывающая сторона.

Диагностика содержит severity, stage и исходное сообщение. Сообщения source:line:column или source line N получают сообщённые координаты и исходный resolved source file. Для остальных координаты не выдумываются. При одной только строке column отсутствует. Стадии: input, source, verify, optimise, verify_optimised, object, link, run и publish. Ошибка запуска может дать диагностику без завершённой записи стадии.

## Проверки и ограничения

```sh
python3 -m unittest discover -s ide/tests -v
python3 ide/tests/integration.py --compiler /host/fir1_source --ir /host/fir1 \
  --linker /host/swl --runner /host/flashvm_runner
```

Unit fixtures используют детерминированные process doubles и проверяют orchestration, а не семантику компилятора. Integration требует настоящих инструментов и проверяет optimized/unoptimized запуск и rollback после syntax/trace ошибок для четырёх численных профилей. Проверки и driver работают после копирования/переименования IDE-каталога; отдельному дистрибутиву нужен общий MIT LICENSE. Windows, физический Android и desktop UI проверяются отдельно. Editor buffers, incremental compilation, source maps, project files и debugger UI здесь ещё не реализованы.

[Numeric example](examples/numeric/README.ru.md) · [Validation / Проверки](VALIDATION.md).

[Правила читаемого кода](AGENTS.md): комментарии перед функциями/типами, явное канальное владение, инварианты GC и JIT/debug.

[AS1/AS2 ↔ Dis module interface](MODULE_INTEROP.ru.md): planned bidirectional module support; later AS IDE port is allowed.
