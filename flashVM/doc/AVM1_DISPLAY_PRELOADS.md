# Function2 display preloads / Клиповые preloads Function2

v0.216 selects `_root` and `_parent` from the activation base clip, using
intrinsic display state rather than replaceable script getters. The base uses
a MovieClip receiver, otherwise the inherited activation base, otherwise an
available global MovieClip root. Without display provenance, the existing
host receiver/global fallback remains. Full function-definition clip provenance
is still open; base selection follows the current target-scope model.

If the parent is absent (null/undefined), its preload does not consume a register.
A following `_global` takes that slot, leaving the later register undefined.
Register-capacity validation remains conservative: it counts selected preload
bits; minimal-register records relying on this skipped slot remain an open edge
case. Parameter/preload overlap and named inherited bindings are not changed.

_root/_parent берутся из внутреннего display-состояния базового клипа, обходя
переопределённые getters. База выбирается по MovieClip this, наследуемой области
активации и доступному глобальному корню. Происхождение функции из исходного
клипа ещё открыто. При отсутствии родителя регистр не расходуется; следующий
_global занимает его место. Проверка вместимости пока считает все preload-биты,
так что минимальное число регистров при пропуске родителя остаётся открытым.

Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/function.rs load_root/load_parent/load_global. Original
`tests/tests/swfs/avm1/define_function2_preload_order/test.swf` has header 6.
Running it unchanged with `flashvm_reference_swf --timeline 1` returns zero,
empty stderr and byte-exact output.txt: Object, undefined, _level0, Object.
No input header or expected output is patched. Its two timeline contexts cover
parentless root and child-parent selection. The arguments/other Function2
fixtures inspected alongside it have header 15 and are excluded from SWF<=8
execution evidence.

`avm1_function2` additionally checks parentless preload/global slot placement
and intrinsic root/parent after installing failing script getters on a child.
These cases do not complete AVM1, MovieClip or any roadmap stage.

Оригинальный SWF6 выполнен без изменения файла/ожиданий: код 0, stderr пуст,
trace совпадает побайтно. Тест Function2 дополнительно проверяет заменённые
getters. Остальные просмотренные upstream Function2/arguments SWF версии 15
не запускались как эталон SWF<=8. Этапы AVM1/MovieClip не закрываются.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 374 and Cairo
Release 373 tests passed (five/five execution-permission retries). Six selected
AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan, with stack/leak
checks disabled. All 17 unchanged selected original SWF<=8 probes returned zero,
empty stderr and byte-exact traces, including the 16 previous probes and the
new SWF6 preload-order case. No non-Linux runtime verification is claimed.
