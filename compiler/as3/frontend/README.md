# AS3 frontend

[Build, coverage and limitations](../README.md).

Grammar: `as3.atg`; driver: `main.c`; tests: `tests/check.py`.

## Current syntax 0.4.0

Supports ASI/restricted line breaks; declaration/member metadata; nested
Vector.<...>; regexp with g/i/m/s/x and duplicate-flag checks; E4X XML/XMLList,
comments, CDATA, PI, expression-interpolated names/values, attribute/wildcard/
namespace/computed selectors, descendants and filters; default xml namespace.
XML interpolations use the ordinary Coco/R expression grammar, including nested
XML/regexp. Shift operators remain ordinary operators outside type context.
Static closing names and delimiter termination are checked. XML entities, DTD,
full Unicode name categories, namespace/type resolution and regexp-program
validity are not checked. This is not complete XML validation or E4X execution.
Escaped identifiers, AST construction and assignment-target/context checks still
need work before a complete frontend can be claimed.

[XmlVector.as](../examples/XmlVector.as) is a syntax example; see the examples README.

New rules / [Metadata](as3.atg#L1378) | [TypeClose](as3.atg#L1394) | [XmlLiteral](as3.atg#L1472) | [XmlProperty](as3.atg#L1478) | [XmlSelector](as3.atg#L1486) | [Terminator](as3.atg#L1504)
