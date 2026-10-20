// Syntax-only E4X fixture: parsing never resolves external entities or runs AS.
var document = <корень имя="&quot;&#65;&#x1F600;">&amp;</корень>; // Unicode XML names and decimal/hex references.
var markup = <a><!-- a-b --><![CDATA[&raw;]]><?target data?></a>; // Separate comment, CDATA and PI delimiters.
var interpolated = <item>{document.@имя}</item>; // Attribute selection remains an actual AS interpolation.
trace(document, markup, interpolated); // This call is recognized but never executed by the syntax checker.
