// The embedding host publishes NativeDemo before these linked actions execute.
// This supported display-profile assignment invokes a real rooted C setter.
NativeDemo.accumulate = 19;
// The second native operation updates the host-owned bounded total to42.
NativeDemo.accumulate = 23;
// Full AVM1 clients may also call NativeDemo.add(19,23) and NativeDemo.seed().
// The executable AS profile currently lowers assignments, not general method calls;
// main.c separately invokes those actual methods through the public C API.
