# Input API (C89)

`flashvm/input.h` defines `FVM_InputRule`, `FVM_InputEvent`, `fvm_input_set_rule` and `fvm_input_map`. The host detects touches and swipes, passes gesture type, finger count and direction, and receives a mapped mouse button (left, right, middle, wheel up/down). Mapping is per context. Exact direction wins over wildcard. **There is no touchscreen recognizer or event injection into the Flash display list yet.** See `examples/embedding/input`.
