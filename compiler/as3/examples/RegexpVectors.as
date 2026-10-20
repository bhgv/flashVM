// Syntax-only fixture: the parser neither matches patterns nor emits ABC/SWO/SWF.
var pattern = /(?P<digits>\d{2,4})-\k<digits>/gi; // Named groups, bounds and a named backreference.
var bytes:Vector.<int> = new <int>[1, 2, 3]; // A typed initializer requires values rather than array holes.
CONFIG::debug { // Recognize a conditional block without evaluating CONFIG or discarding source.
    trace(pattern, bytes[0]); // The block contains ordinary AS3 items and expressions.
} // End the conditional block.
class Example { // Class guards contain member declarations rather than executable statements.
    CONFIG::debug { // This namespace-qualified condition is syntactically contextual.
        public var enabled:Boolean = true; // The member remains available for future semantic analysis.
    } // End the guarded members.
} // End the class declaration.
