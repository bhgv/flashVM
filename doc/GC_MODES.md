# GC modes

- `0`: normal allocation threshold, 1000 objects.
- `-100`: threshold 1900; `100`: threshold 100.
- `101` (`FVM_GC_SUSPENDED`): skip automatic steps.
- `102` (`FVM_GC_FULL`): one forced pass, preserving prior mode.

A threshold does not schedule collection on its own: the host must invoke `fvm_gc_step` at safe points. Do not collect inside AVM1 execution until evaluation stack roots are traced.
