# Experimental AVM1 execution API

This is an intentionally narrow interpreter, not a complete Flash 8 runtime.
The implementation is written in C++98; the API and tests use C89.

- `fvm_avm1_execute(context, data, size)` validates and executes one complete
  AVM1 action stream, ending with opcode `0x00`.
- `fvm_swf_execute_actions(context)` walks the top-level tags of an FWS SWF
  previously loaded using `fvm_load_memory`, executing each `DoAction` tag
  in file order. This does **not** implement frame timing or sprite actions.
- `fvm_register_function(context, name, callback, userdata)` makes a C callback
  available to AVM1 `CallFunction` by name. `fvm_get_global` and
  `fvm_set_global` share the same global object as AVM1 variables.

Supported bytecode: End, Push (string, signed 32-bit integer, boolean, null,
undefined), Pop, GetVariable, SetVariable, Add, Subtract, Multiply, Divide,
Jump, If, and CallFunction. Branch targets must be instruction boundaries.
A nonzero `FVM_Config.instruction_limit` bounds instruction execution.

Limitations: no AVM1 prototype chain, lexical scope, user-defined functions,
string coercion, double/float Push, ConstantPool, GetMember/SetMember,
DoInitAction, nested sprites, display list, GC, JIT or SWF compression.
There is no rollback on interpreter failure: previously set globals remain.
The interpreter and host C API must not be called concurrently on a context.

See `tests/interpreter.c` and `tests/swf_execute.c` for executable C89
examples demonstrating native callback invocation from AVM1 and SWF.

Security note: this experimental runtime is not yet suitable for executing
untrusted SWF. Set a finite instruction limit; memory and stack quotas are not
yet enforced throughout the interpreter.
