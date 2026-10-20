// Syntax-only example: E4X, regexp, metadata and nested Vector types.
[Event(name="ready", type="flash.events.Event")] // Metadata applies to the following declaration.
public class XmlVector { // Name/type resolution and ABC generation are separate work.
    private var rows:Vector.<Vector.<int>> // Split nested type closers only in type context.
    public function select(name:String):XMLList { // Retain ordinary typed method syntax.
        var pattern:RegExp = /[a-z]+/gim // Regexp lexical goal differs from division.
        var document:XML = <root><item name={name}>{pattern.test(name)}</item></root> // AS expressions are parsed inside braces.
        return document..item.(@name == name) // Descendant selection and E4X filter expression.
    } // Finish the method; the syntax checker does not execute XML operations.
} // Finish the class declaration.
