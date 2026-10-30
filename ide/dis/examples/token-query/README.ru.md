# Пример запросов к токенам

[English](README.md) · [Контракт](../../../TOKEN-QUERIES.ru.md) ·
[Ленивый анализ](../../../SYNTAX-LAZY.ru.md) · [Полностью комментированный код](main.b)

Исполняемая программа Limbo получает настоящий каталог AS из Coco/R, показывает
контекст `alpha`, печатает уникальные имена `alpha`/`alpine`, ищет следующее имя
вне строк/комментариев, затем анализирует локально изменённый снимок и показывает
reuse. Файлы, документ и выделение не меняются; первый снимок сохраняет значение.

Нужны согласованная native-cell DisVM и поставляемые idesyntax.dis,
idetokenquery.dis, limbolex.dis, aslex.dis. Временный native bootstrap позволяет
пересобрать пример, но не становится основным компилятором проекта. При
исполнении не нужны Draw/Tk, внешний emu, шрифт, окно, Python или генератор.

После сборки/установки выберите пункт17 либо:

```sh
python3 build-menu.py --projects disvm-runtime --backend cmake --launch tokens
sh out/disvm-runtime/run-token-query.sh --no-jit
sh out/disvm-runtime/run-token-query.sh --jit
```

В перенесённом установленном runtime/editor можно пересобрать исходник без
исходного checkout, используя поставленные compiler/headers:

```sh
./limbo-native -I include/limbo -o tokenquery-local.dis examples/token-query/main.b
./disvm --no-jit tokenquery-local.dis
./disvm --jit tokenquery-local.dis
```

Ожидаются `Context: alpha [4,9)`, два кандидата, координаты следующего alpha,
реальные scanned/reused/restart и финальная строка `PASS token-query example`.
Счётчики относятся к записям, не к скорости. После запуска worker на каждом
пути проверки статуса выполняется stop и приём подтверждения, затем сообщается
успех/ошибка. Rendezvous блокирует этот консольный пример намеренно; GUI
использует lazy owner.

| API | Назначение и владение |
| --- | --- |
| Idesyntax.start | Частный worker и две принадлежащие вызывающему канальные возможности |
| Idesyntax.Request | Неизменяемый снимок/корреляция; один приём на запрос, включая stop |
| Idetokenquery.at | Заимствовать каталог, вернуть скалярный токен без изменения выделения |
| Idetokenquery.complete | Заимствовать точные source/catalog, вернуть новый ограниченный массив отсортированных имён |
| Idetokenquery.find | Заимствовать source/catalog, вернуть первое литеральное совпадение допущенных классов |

Revision/профиль должны совпадать, исходник быть точным; одновременно менять
массивы нельзя. Нативные/VM указатели не передаются. Кандидаты лексические:
нет типов, scopes/import resolution. Ресурсы/Unicode и объединённый класс5
пробелов/комментариев описаны в контракте. Пример не исполняет новый формат
Dis64, не доказывает calling ABI Frame64 или физическую задержку GUI.
