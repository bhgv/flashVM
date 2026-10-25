# AS: подсветка, контекст ввода и представление IDE

[Архитектура](../EDITOR-CHANNELS.ru.md) · [Dis-сервисы](../dis/bridge/README.md) ·
[Недостающее mixed-colour API](../TEXT-COLOR-EXTENSION.md)

## Что находится здесь

`syntax-worker.as` — код AS2, разбираемый нашим Coco/R-фронтендом AS2. Фабрика
`makeIdeSyntaxWorker()` создаёт worker-local `scan(job)` и `context(job)`.
Они работают с текстом/числами/массивами, не с MovieClip/TextField или Dis pointers.
Приложение должно загрузить код в конкретный worker context; один файл сам по
себе не создаёт потоки и не доказывает весь путь компиляции AS→SWF/JIT.

`scan` поддерживает два режима языка: `as` и `limbo`. Стили — keyword, name, number,
string, comment. Состояния normal/line/block/single/double, trailing escape и
trailing star передаются следующему смежному окну. Учтён `*/`, разрезанный границей.
Для произвольного viewport сначала нужен актуальный лексический checkpoint Dis;
нельзя объявлять середину многострочного комментария обычным кодом.

Пределы одного задания: 16384 UTF-16 units, 4096 цветовых диапазонов, int32 revision,
base и ticket. Диапазоны содержат и Dis Unicode-координаты, и локальные AS UTF-16
границы. Surrogate pair считается одним Unicode-символом; разрезанные/одиночные
surrogates отвергаются. Ошибка бюджета возвращает ok=false, не частичную окраску.

```actionscript
var worker = makeIdeSyntaxWorker(); // Создаём состояние только данного AS worker.
var job = new Object(); // Передаём значения, а не display-объекты.
job.source = "var answer = 42;"; // Неизменяемый текст ограниченного окна.
job.language = "as"; // Для Limbo здесь используется "limbo".
job.revision = 7; // Ревизию назначает владелец документа Dis.
job.base = 0; // Unicode-смещение начала окна в документе.
job.ticket = 11; // ID выбранного Dis-контроллером viewport/checkpoint задания.
job.state = "normal"; // Это известное состояние начала документа, не догадка.
job.escaped = false; // Нет перенесённого строкового escape.
job.star = false; // Нет перенесённого конца block comment.
var colours = worker.scan(job); // Получаем только значения диапазонов и конечного состояния.
```

Для короткой справки окно `context(job)` заканчивается у каретки. Из тех же
лексических диапазонов вычисляются prefix/qualified prefix, receiver, диапазон
замены и innermost call с номером аргумента. Строковые/комментарные запятые,
вложенные вызовы, массивы и объекты не увеличивают внешний argument index.
Обрабатываются `Math.max(`, `MovieClip.` и Limbo `Sys->open(`; внутреннее имя
каталога для последнего — Sys.open. Continuation stack ограничен 64 уровнями,
номер аргумента — 1024. Checkpoint копируется, а не изменяется через чужую ссылку.

Это лексическая помощь, не второй compiler parser и не полное type inference.
Для алиасов `clip`/`sys` тип receiver должен предоставить Dis/frontend provider;
неизвестный динамический объект не получает выдуманный тип. Regex/E4X, полная
семантика чисел, сложный контекст типов и деление идентификатора посередине окна
ещё не решены. Практические окна должны завершаться у осмысленной lexical/line
границы или происходить из валидного checkpoint; compiler diagnostics авторитетны.

## Представление, dropdown и короткая справка

`presentation.as` принимает уже декодированные Reply/View. Фрагменты снимка
проверяются по идентичности и byte offsets, соединяются один раз и лишь затем
передаются `target.document`. Выбор задаёт Dis через View/65536. AS не реализует
историю, правки документа или политику вставки completion-кандидата.

`expectSyntax/applySyntax`, `expectCompletion/applyCompletion` и
`expectCalltip/applyCalltip` сравнивают revision, catalog generation, prefix/callee,
активный аргумент и ticket. Запоздалый ответ не окрашивает другой текст и не
подменяет текущую подсказку. Параметры приходят из Dis как name/type_name/optional/
summary; полный signature и краткое описание поддерживают Flash-editor-style UI.

target — UI-owner адаптер с методами document, selection, syntax, completion,
calltip и fault. Последние три операции визуализации возвращают true только при
реальном принятии renderer. Именно здесь создаются/обновляются dropdown и панели
справки; оконечные виджеты пока не подключены. Сам протокольный адаптер не рисует
успешный список-заглушку и не подменяет отсутствие mixed-colour backend.

В текущем FlashVM TextField стиль uniform. Для окраски отдельных диапазонов
потребуется нативное расширение с сохранением общего shaping/layout. Здесь нет
фиктивного вызова неподдерживаемого setTextFormat(begin,end,format).

## Проверка

```sh
# Необязательные algorithm tests: явный путь к общему AS codec SDK.
node ide/as/tests.js "$PWD/disVM/examples/ui-channel/bridge.as"
# Собственный Coco/R AS2 parser: используйте фактический путь вашего build engine.
build/workspace/compilers/as2/frontend/as2_parse ide/as/syntax-worker.as
build/workspace/compilers/as2/frontend/as2_parse ide/as/presentation.as
```

В установленном out/ide общий codec находится в as/bridge.as:
`node as/tests.js as/bridge.as`. Node не требуется для работы IDE или обычной
сборки; алгоритмические проверки не являются исполнением helper как SWF/JIT.
Поддержка JIT самого FlashVM не подменяет приёмку worker loading/compilation.

## English

The AS2 worker supports lexical colouring and caret context for both AS and Limbo,
using bounded Unicode-aware windows and explicit lexical/call checkpoints.
The view atomically assembles Dis snapshots and gates independent syntax,
completion and parameter-help responses by exact context. Dis owns symbols,
editing/history policy and insertion decisions; AS owns fast display-local work.
Builtin catalogs contain structured optional/rest parameter metadata. Actual
popup widgets, mixed-colour TextField rendering and end-to-end worker/SWF launch
remain unfinished. Tests use an explicit codec path, the own AS2 parser and an
optional Node algorithm host; none is mistaken for full application execution.
