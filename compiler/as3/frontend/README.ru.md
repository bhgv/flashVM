# Фронтенд AS3

[Сборка, покрытие и ограничения](../README.ru.md).

Грамматика: `as3.atg`; драйвер: `main.c`; тесты: `tests/check.py`.

## Текущий синтаксис 0.4.1

Поддержаны ASI и ограниченные переводом строки конструкции; metadata перед
декларациями/членами; вложенные `Vector.<…>`; regexp с g/i/m/s/x и проверкой
повторов флагов; E4X XML/XMLList, комментарии, CDATA, PI, имена/значения через
`{выражение}`, атрибуты, wildcard/namespace/вычисляемые селекторы, потомки и
фильтры; `default xml namespace`. Выражения внутри XML разбираются обычной
Coco/R-грамматикой, включая вложенные XML/regexp. Сдвиги остаются обычными
операторами вне типов. Проверяются статические закрывающие имена XML, Unicode-имена, разделители,
форма entity-ссылок, диапазоны числовых ссылок, комментарии и PI. Разрешение
сущностей/пространств имён/типов, DTD и полная regexp-программа не реализованы.
Это не полная валидация XML и не выполнение E4X. Escape в идентификаторах,
AST и проверки контекста/левой части ещё нужны для полного фронтенда.

[XmlVector.as](../examples/XmlVector.as) — пример синтаксиса; см. README примеров.

Новые правила / [Metadata](as3.atg#L1527) | [TypeClose](as3.atg#L1543) | [XmlLiteral](as3.atg#L1621) | [XmlProperty](as3.atg#L1627) | [XmlSelector](as3.atg#L1635) | [Terminator](as3.atg#L1653)

[Parser coverage 0.5.2](../../PARSER_COVERAGE.ru.md) documents XML/cast fixes and remaining boundaries.

Вспомогательный C-код в авторитетной ATG: [XmlCode](as3.atg#L251) | [XmlStart](as3.atg#L275) | [XmlName](as3.atg#L286) | [XmlChar](as3.atg#L301) | [XmlReference](as3.atg#L307) | [XmlComment](as3.atg#L340) | [XmlUntil](as3.atg#L354) | [XmlElement](as3.atg#L381).
