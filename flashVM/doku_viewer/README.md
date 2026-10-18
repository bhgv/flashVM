# FlashVM documentation viewer / Просмотрщик документации

Локальный HTTP-просмотрщик Markdown и исходников. Запуск: [view_docs.py](../view_docs.py).
Весь движок находится в этой папке. Python не исполняет просматриваемые файлы.

## Установка и запуск

Из корня проекта (используйте нужный интерпретатор вместо `python`):

```sh
python -m pip install -r doku_viewer/requirements.txt
python view_docs.py
```

Откроется `http://127.0.0.1:8765/`. Остановить сервер: Ctrl+C.
Запуск без автоматического открытия браузера и с другим портом:

```sh
python view_docs.py --no-browser --port 8780
```

Скрипт находит корень относительно своего файла, поэтому запускать его можно
из другой рабочей папки, указав полный путь. Порт 0 выбирает свободный порт;
фактический адрес печатается при запуске. Занятый порт и отсутствующие пакеты
дают понятную ошибку. Сборка FlashVM и C/C++-компилятор не нужны.

Зависимости закреплены: markdown2 2.3.10 и Pygments 2.5.2 поддерживают Python
2.7 и Python 3. На Python 2.7.9 установите их его собственным pip; если старый
pip не может обратиться к HTTPS-индексу, перенесите совместимые пакеты с другой
машины и установите их локально (`pip install --no-index --find-links ...`).
Пакеты не скачиваются автоматически при старте просмотрщика.

## Навигация и отображение

- Слева: корневые Markdown-файлы, всё дерево `doc/` и `examples/`, по алфавиту.
  Поле поиска фильтрует пути; оно не ищет внутри текста документа.
- README примеров, основной каталог, обычные относительные ссылки, ручные
  якоря и заголовки на русском/английском остаются переходами внутри браузера.
  История назад/вперёд работает штатными кнопками браузера.
- Markdown: таблицы, fenced code, списки, ссылки, изображения, зачёркивание,
  `<small>` и именованные HTML-якоря. Заголовки получают Unicode-якоря с
  сохранением подчёркиваний и суффиксами для повторяющихся названий.
  Суффикс не может занять ID уже встреченного заголовка: последовательность
  `API`, `API-1`, `API` даёт `api`, `api-1`, `api-2`. Каждый документ
  начинает нумерацию заново.
- Ссылки на `.h`, `.cpp`, `.c`, `.py`, `.java`, `.as` и другие текстовые
  исходники показывают Pygments-подсветку и физические номера строк.
  `#L123` прокручивает строку 123 в центр окна и выделяет её. Номер строки
  сам является ссылкой. Например: [stage API](../include/flashvm/stage.h#L19).
- Raw открывает исходный текст текущего документа без Markdown-обработки.
- Мобильная раскладка помещает навигацию над документом.

Это ограниченный локальный Markdown-просмотрщик, а не полная реализация GitHub:
Mermaid, формулы, GitHub alerts и удалённые включения не исполняются. Встроенные
скрипты и обработчики событий удаляются. Внешние ссылки открываются обычным
переходом; внешние изображения автоматически не загружаются. SVG, PDF, архивы,
бинарники и неизвестные типы файлов не выдаются. Растровые PNG/JPEG/GIF/WebP/ICO
показываются непосредственно. Лимит просматриваемого файла — 4 MiB.

## Границы сервера

Сервер всегда слушает IPv4 loopback `127.0.0.1`; параметра публичного адреса нет.
Нет регистрации, редактирования, shell-команд и доступа за пределы репозитория.
Скрытые пути, `..`, Windows drive paths и выходящие за корень symlinks запрещены.
Content Security Policy разрешает только локальные CSS/JS/изображения; HTML
Markdown очищается отдельным allowlist-парсером. Не используйте этот локальный
просмотрщик как публичный сервер документации.

## Реализация

[server.py](server.py) содержит HTTP-маршрутизацию, проверку файлов, Markdown-
рендер, фильтрацию HTML, генерацию навигации и Pygments-строки. [style.css](style.css)
задаёт адаптивную раскладку и подсветку; [viewer.js](viewer.js) фильтрует пути
и прокручивает выбранную строку. Никаких CDN, сторонних веб-сервисов и сборки JS.

## Проверки

```sh
python -m unittest doku_viewer.test_viewer
```

[test_viewer.py](test_viewer.py) проверяет настоящий loopback HTTP: README,
руководство, README примера, исходник с номером строки, raw и static assets;
Markdown-таблицы/код, Unicode/повторные якоря, удаление executable HTML,
кодирование кириллических имён и запрет traversal/escaping symlinks.

## Проверенный объём v0.544

На Linux выполнены все пять HTTP/render/confinement тестов отдельно на точном
CPython 2.7.9 (собран из исходников) и CPython 3.12 с закреплёнными пакетами.
На Python 3.12 дополнительно отрендерены все 676 Markdown-страниц текущих
`doc/`, `examples/` и входных README без ошибок. Проверены обе CLI help-ветки.
Это не подтверждает запуск на Windows/macOS и визуальную приёмку браузерной
раскладки: Chromium в среде отсутствует, загрузить его для screenshot-проверки
не удалось. HTTP и HTML проверены автоматически; мобильный CSS требует
дополнительной визуальной проверки в браузере пользователя.

## English

Install the pinned packages with the chosen Python interpreter, then run
`python view_docs.py`. The root launcher starts a read-only loopback HTTP server
and opens the browser. Use `--no-browser` and `--port` for manual startup.
The engine supports Python 2.7.9 and Python 3 syntax and dependencies. Navigation
indexes project Markdown, documentation and examples; relative source links keep
`#L<number>` line anchors, syntax highlighting and selected-line scrolling.
There is no remote bind option, command execution, editing or external asset CDN.
Files stay within the visible repository tree, with a 4 MiB limit and explicit
text/raster type allowlists. Markdown HTML is sanitized; SVG, PDF, Mermaid,
math rendering and complete GitHub Markdown parity are outside this version.
Run the same unittest module under each supported interpreter to verify HTTP,
rendering and confinement. Third-party Python packages retain their own licenses;
FlashVM's viewer code is covered by the root [MIT license](../../LICENSE).

## Unique heading IDs / уникальные якоря заголовков (v0.567)

The browser tracks every emitted heading ID, including suffixes generated for
previous headings. A repeated heading skips IDs already taken by literal suffixed
headings. For example, API / API-1 / API becomes api / api-1 / api-2. State is
local to each rendered document. Explicit authored HTML IDs remain unchanged;
ensuring those authored IDs are unique is the document author's responsibility.
The HTTP/renderer regression suite checks collisions and independent renders.
This checkpoint was run on Python 3; Python 2.7.9 was not rerun in this environment.

Проверка охватывает конфликты с автоматически созданными и буквальными суффиксами,
а также повторный рендер другого документа. Ручные HTML id не переписываются;
их уникальность обеспечивает автор. В этой контрольной точке выполнены проверки
на Python 3; нового прогона Python 2.7.9 в данной среде нет.


## Repository layout since v0.572 / Структура с v0.572

From the repository root / Из корня репозитория:

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python view_docs.py
```

The root launcher delegates to `flashVM/view_docs.py`; arguments such as
`--no-browser` and `--port` are forwarded. Both launchers work independently
of the working directory. `/` opens `/view/flashVM/README.md`. Navigation covers
FlashVM and the root README files; the shared `/view/LICENSE` remains readable.
The server is confined to the repository, including symlink checks.

Корневой файл передаёт управление `flashVM/view_docs.py`, сохраняя параметры.
Оба запуска не зависят от рабочей папки. Главная страница открывает README
FlashVM; общая лицензия доступна из корня. Доступ за пределы репозитория запрещён.
The earlier project-local commands above remain valid after `cd flashVM`.
Ранее приведённые команды проекта остаются верными после `cd flashVM`.
