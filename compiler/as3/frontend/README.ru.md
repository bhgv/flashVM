# Фронтенд AS3

[Сборка, покрытие и ограничения](../README.ru.md).

Грамматика: `as3.atg`; драйвер: `main.c`; тесты: `tests/check.py`.

## Текущий синтаксис 0.5.0

Поддержаны ASI и ограниченные переводом строки конструкции; metadata перед
декларациями/членами; вложенные `Vector.<…>`; regexp с g/i/m/s/x и проверкой
повторов флагов; E4X XML/XMLList, комментарии, CDATA, PI, имена/значения через
`{выражение}`, атрибуты, wildcard/namespace/вычисляемые селекторы, потомки и
фильтры; `default xml namespace`. Выражения внутри XML разбираются обычной
Coco/R-грамматикой, включая вложенные XML/regexp. Сдвиги остаются обычными
операторами вне типов. Проверяются статические закрывающие имена XML, Unicode-имена, разделители,
форма entity-ссылок, диапазоны числовых ссылок, комментарии и PI. Разрешение
сущностей/пространств имён/типов, DTD и полная совместимость regexp-движка не реализованы.
Это не полная валидация XML и не выполнение E4X. AST и проверки контекста/левой части ещё нужны для полного фронтенда.

[XmlVector.as](../examples/XmlVector.as) — пример синтаксиса; см. README примеров.

Новые правила / [Metadata](as3.atg#L2200) | [TypeClose](as3.atg#L2216) | [XmlLiteral](as3.atg#L2297) | [XmlProperty](as3.atg#L2303) | [XmlSelector](as3.atg#L2311) | [Terminator](as3.atg#L2329)

[Parser coverage 0.6.0](../../PARSER_COVERAGE.ru.md) documents XML/cast fixes and remaining boundaries.

Вспомогательный C-код в авторитетной ATG: [XmlCode](as3.atg#L490) | [XmlStart](as3.atg#L514) | [XmlName](as3.atg#L525) | [XmlChar](as3.atg#L540) | [XmlReference](as3.atg#L546) | [XmlComment](as3.atg#L579) | [XmlUntil](as3.atg#L593) | [XmlElement](as3.atg#L620).

[Лексический профиль 0.6.0](../../LEXICAL_PROFILE.ru.md).

0.6.0: [Identifier](as3.atg#L2341) | [IdentifierUnit](as3.atg#L33) | [IdentifierLine](as3.atg#L215) | [as3_identifier_valid](as3.atg#L86) | [as3_identifier_space](as3.atg#L110) | [as3_string_valid](as3.atg#L175) | [RegexPattern](as3.atg#L1192) | [RegexExpression](as3.atg#L1152) | [RegexEscape](as3.atg#L941) | [RegexQuantifier](as3.atg#L1062) | [RegexClass](as3.atg#L1025) | [VectorLiteral](as3.atg#L1968) | [ConfigBlock](as3.atg#L1436) | [ConfigMembers](as3.atg#L1442).
