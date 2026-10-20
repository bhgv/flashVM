# AS3 frontend

[Build, coverage and limitations](../README.md).

Grammar: `as3.atg`; driver: `main.c`; tests: `tests/check.py`.

## Current syntax 0.4.1

Supports ASI/restricted line breaks; declaration/member metadata; nested
Vector.<...>; regexp with g/i/m/s/x and duplicate-flag checks; E4X XML/XMLList,
comments, CDATA, PI, expression-interpolated names/values, attribute/wildcard/
namespace/computed selectors, descendants and filters; default xml namespace.
XML interpolations use the ordinary Coco/R expression grammar, including nested
XML/regexp. Shift operators remain ordinary operators outside type context.
Static closing names, delimiters, Unicode XML names, reference spelling,
numeric reference ranges, comments and PI are checked. Entity/namespace/type
resolution, DTD and complete regexp-program validity are not implemented. This is not complete XML validation or E4X execution.
Escaped identifiers, AST construction and assignment-target/context checks still
need work before a complete frontend can be claimed.

[XmlVector.as](../examples/XmlVector.as) is a syntax example; see the examples README.

New rules / [Metadata](as3.atg#L1527) | [TypeClose](as3.atg#L1543) | [XmlLiteral](as3.atg#L1621) | [XmlProperty](as3.atg#L1627) | [XmlSelector](as3.atg#L1635) | [Terminator](as3.atg#L1653)

[Parser coverage 0.5.2](../../PARSER_COVERAGE.md) documents XML/cast fixes and remaining boundaries.

C helpers in the authoritative ATG: [XmlCode](as3.atg#L251) | [XmlStart](as3.atg#L275) | [XmlName](as3.atg#L286) | [XmlChar](as3.atg#L301) | [XmlReference](as3.atg#L307) | [XmlComment](as3.atg#L340) | [XmlUntil](as3.atg#L354) | [XmlElement](as3.atg#L381).
