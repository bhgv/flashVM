# Input API (C89)

`flashvm/input.h` defines [`FVM_InputRule`](../include/flashvm/input.h#L33), [`FVM_InputEvent`](../include/flashvm/input.h#L42), [`fvm_input_set_rule`](../include/flashvm/input.h#L52) and [`fvm_input_map`](../include/flashvm/input.h#L54). The host detects touches and swipes, passes gesture type, finger count and direction, and receives a mapped mouse button (left, right, middle, wheel up/down). Mapping is per context. Exact direction wins over wildcard. **There is no touchscreen recognizer or event injection into the Flash display list yet.** See `examples/embedding/input`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_map` | function | [include/flashvm/input.h:54](../include/flashvm/input.h#L54) |
| `fvm_input_set_rule` | function | [include/flashvm/input.h:52](../include/flashvm/input.h#L52) |
| `FVM_InputEvent` | struct | [include/flashvm/input.h:42](../include/flashvm/input.h#L42) |
| `FVM_InputRule` | struct | [include/flashvm/input.h:33](../include/flashvm/input.h#L33) |
<!-- source-links:end -->
