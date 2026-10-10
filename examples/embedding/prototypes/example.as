// ActionScript 2.0 (SWF 7/8): conceptual equivalent of the C example.
function Base() {}
Base.prototype.inherited = 42;
function Child() {}
Child.prototype = new Base();
var instance = new Child();
trace(instance.inherited);
// NOTE: DefineFunction/NewObject for arbitrary AS constructors is not yet supported.
