# AS3 frontend

[Build, coverage and limitations](../README.md).

Grammar: `as3.atg`; driver: `main.c`; tests: `tests/check.py`.

## Current syntax 0.5.0

Supports ASI/restricted line breaks; declaration/member metadata; nested
Vector.<...>; regexp with g/i/m/s/x and duplicate-flag checks; E4X XML/XMLList,
comments, CDATA, PI, expression-interpolated names/values, attribute/wildcard/
namespace/computed selectors, descendants and filters; default xml namespace.
XML interpolations use the ordinary Coco/R expression grammar, including nested
XML/regexp. Shift operators remain ordinary operators outside type context.
Static closing names, delimiters, Unicode XML names, reference spelling,
numeric reference ranges, comments and PI are checked. Entity/namespace/type
resolution, DTD and complete regexp-engine compatibility are not implemented. This is not complete XML validation or E4X execution.
AST construction and assignment-target/context checks still
need work before a complete frontend can be claimed.

[XmlVector.as](../examples/XmlVector.as) is a syntax example; see the examples README.

New rules / [Metadata](as3.atg#L2200) | [TypeClose](as3.atg#L2216) | [XmlLiteral](as3.atg#L2297) | [XmlProperty](as3.atg#L2303) | [XmlSelector](as3.atg#L2311) | [Terminator](as3.atg#L2329)

[Parser coverage 0.6.0](../../PARSER_COVERAGE.md) documents XML/cast fixes and remaining boundaries.

C helpers in the authoritative ATG: [XmlCode](as3.atg#L490) | [XmlStart](as3.atg#L514) | [XmlName](as3.atg#L525) | [XmlChar](as3.atg#L540) | [XmlReference](as3.atg#L546) | [XmlComment](as3.atg#L579) | [XmlUntil](as3.atg#L593) | [XmlElement](as3.atg#L620).

[Lexical profile 0.6.0](../../LEXICAL_PROFILE.md).

0.6.0: [Identifier](as3.atg#L2341) | [IdentifierUnit](as3.atg#L33) | [IdentifierLine](as3.atg#L215) | [as3_identifier_valid](as3.atg#L86) | [as3_identifier_space](as3.atg#L110) | [as3_string_valid](as3.atg#L175) | [RegexPattern](as3.atg#L1192) | [RegexExpression](as3.atg#L1152) | [RegexEscape](as3.atg#L941) | [RegexQuantifier](as3.atg#L1062) | [RegexClass](as3.atg#L1025) | [VectorLiteral](as3.atg#L1968) | [ConfigBlock](as3.atg#L1436) | [ConfigMembers](as3.atg#L1442).
