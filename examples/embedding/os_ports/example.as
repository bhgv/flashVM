// ActionScript 2.0 illustrative host integration; NOT wired to AVM1 yet.
// A host polls native input through ports/, then calls fvm_input_dispatch.
// TCP sockets currently have a C host API, not a built-in AS Socket class.
_root.onMouseMove = function() { trace("mouse moved"); };
