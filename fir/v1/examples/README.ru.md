# Примеры FIR v1

[English](README.md). [Сборка](../README.ru.md), [покрытие](../doc/COVERAGE.ru.md).

| Папка | Что показывает |
| --- | --- |
| [as1](as1/README.ru.md) | Числовой исходник, приоритеты, ожидаемый FIR |
| [as2](as2/README.ru.md) | Скобки и умножение |
| [as3](as3/README.ru.md) | Ограниченный числовой профиль, дроби |
| [limbo](limbo/README.ru.md) | Модуль с int-результатом и явная trace-обвязка |
| [control_flow](control_flow/README.ru.md) | if/jump и доминирование SSA |
| [channels](channels/README.ru.md) | Существующее расширение каналов FlashVM из FIR |
| [profiles](profiles/README.ru.md) | Профиль исходного языка, hex/дробные константы и f64.div |

В каждой папке — прокомментированный вход, ожидаемый trace и два README.
Source-примеры содержат также ожидаемый читаемый FIR. Команды и значения
параметров API описаны в каждом README. Тесты отдельно проверяют созданный
адаптером и сохранённый FIR. `python fir/v1/examples/view_docs.py` открывает
этот каталог; корневой просмотрщик показывает те же документы.


[Сборка всех примеров для своей/другой ОС и CPU](../../../BUILDING.ru.md).

[structural](structural/README.ru.md) — параметры функций/блоков, только syntax-профиль.

[Сквозной Limbo→FIR→Dis и C99 API](dis/README.ru.md).

[Исходник гибридной таблицы](tables/main.b) · [Контракт](../doc/TABLES.ru.md) · [English](../doc/TABLES.md). Пример проходит наш путь Coco/R → FIR и эталонный C-исполнитель; [исполнение через Dis](../doc/TABLES_DIS.ru.md) тоже поддерживается; SWF ещё предстоит.

[Сквозная реализация расширений Limbo: таблицы, проекции и any](../doc/EXTENSIONS.ru.md).
