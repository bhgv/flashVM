# Рабочее пространство FlashVM

Версия разработки: **0.572**. [English](README.md).

## Просмотр документации

Из корня репозитория:

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python view_docs.py
```

Корневой запуск сейчас открывает [документацию FlashVM](flashVM/README.ru.md).
Он передаёт управление `flashVM/view_docs.py`, сохраняя параметры `--port`
и `--no-browser`. Совместимость с Python 2.7.9 / Python 3 сохраняется;
данное изменение структуры проверено на Python 3, без заявления о новой
проверке исполнения на Python 2.7.9.

## Структура

| Путь | Назначение |
| --- | --- |
| [LICENSE](LICENSE) | Общая лицензия MIT для собственного кода, автор bhgv |
| [flashVM/](flashVM/README.md) | Исходники VM, встроенные зависимости, примеры, документация, инструменты и настройки сборки |
| [flashVM/README.md](flashVM/README.md) | Английское руководство по VM и текущие возможности |
| [flashVM/README.ru.md](flashVM/README.ru.md) | Русское руководство по VM |
| [flashVM/VERSION.txt](flashVM/VERSION.txt) | Версия разработки FlashVM |
| [view_docs.py](view_docs.py) | Вход в документацию рабочего пространства, пока передающий запуск FlashVM |

Будущие компоненты компиляторов будут размещаться рядом с `flashVM/`.
Сам перенос их не реализует. Сторонние зависимости сохраняют собственные
лицензионные уведомления внутри компонента.

## Сборка FlashVM

```sh
cmake -S flashVM -B flashVM/build
cmake --build flashVM/build
ctest --test-dir flashVM/build --output-on-failure
```

Также можно выполнить `cd flashVM` и пользоваться прежними командами проекта.
DEFS, публичные заголовки, структура исходников и внутренние относительные
пути остаются локальными для компонента. См. [ключи сборки](flashVM/DEFS.txt),
[роудмап](flashVM/doc/ROADMAP.md) и [приёмку](flashVM/doc/RELEASE_ACCEPTANCE.md).
Перенос файлов не означает завершения стадий совместимости или платформенных проверок.
