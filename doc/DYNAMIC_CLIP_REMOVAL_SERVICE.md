# Dynamic clip removal service / Удаление динамического клипа — v0.313

The C89 fvm_exec_remove_dynamic_clip(context, clip) applies an intrinsic native
eligibility policy before the existing atomic detach transaction. It accepts a
native instance, reads its private public depth, and leaves negative-depth or
parentless instances unchanged. It also walks the native parent chain: a detached
instance or detached ancestor suppresses removal. A bounded ancestry walk rejects
cycles/excessive nesting. No replaceable getDepth/removeMovieClip method is read.

The native model currently limits public depths to [-16384,1048575]. All of its
nonnegative supported depths lie inside the pinned reference's removable range;
this service does not widen native depth storage to the much larger historical
AVM range. Separate uppercase macros document minimum dynamic depth, ancestry
bound and the service's GC root layout. Forced fvm_exec_detach_clip retains its
broader host policy, including authored negative-depth detachment.

[GC:] The explicit clip root retains native state and ancestry through eligibility
checks and copied-map preparation. Successful removal uses the existing atomic
parent/playback/detachment-marker publication; failures preserve ownership and
marker state. Eligibility preparation can allocate and report FVM_OUT_OF_MEMORY,
even when the policy would ultimately ignore removal.

execution_remove_dynamic_clip is a C89 host/SWF4–8 fixture covering native minimum
negative depth, -1, zero, native maximum positive depth, roots, repeated removal,
script overrides and a child under a detached ancestor. The allocation fixture
verifies every positive-removal preparation failure, root restoration and GC
between attempts. This is a common explicit-object service for future Action/JIT
callers; it does not inspect interpreter state, bytecode or operand stacks.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals.rs remove_display_object: nonnegative public depths, parent
requirement and removed-state exclusion. FlashVM's ancestor check uses the native
detachment marker introduced in v0.312. Complete historical removed state is not
claimed. RemoveSprite dispatch, path coercion, target fallback, unload events and
broader display-object support remain open; the source Action audit remains 94/100.

Общая C-служба удаляет только прикреплённые нативные клипы с неотрицательной
глубиной. Корень, отрицательная глубина и уже отсоединённое дерево не меняются.
Проверки обходят переопределённые скриптовые методы и используют явный объект;
после них выполняется существующая атомарная операция. Проверены host/SWF4–8,
границы нативных глубин, повторное удаление, отсоединённый предок, GC и отказы
памяти. Сама инструкция RemoveSprite, разрешение путей и события ещё не подключены;
все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 605 tests directly. Both targeted
heap/global ASan+UBSan fixtures passed (no stack/leak instrumentation). All 26
accepted original SWFs matched the pinned oracle in each profile. The source
Action-reference audit remains 94/100. No roadmap stage is marked complete.
