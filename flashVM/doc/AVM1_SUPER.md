# Superclass dispatch checkpoint / Вызовы super — v0.23

## English

Function activations now create named `super` references and support Function2
`PreloadSuper`. `SuppressSuper` keeps the named binding undefined. A function
without a constructor or method owner receives undefined rather than a guessed
superclass object. Constructor invocations and named property calls record their
selected owner in a temporary invocation scope.

A superclass facade retains its actual receiver and prototype depth. Parent lookup
uses the live chain at that depth, so an inherited method skips its defining class,
not merely the receiver's immediate prototype. Escaped facades observe later
prototype changes. Constructor calls use the current `__constructor__` reference
and the existing receiver. Getter/setter and named method calls retain that receiver.
Named property writes search ancestor setters; canonical numeric property writes
currently return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) rather than silently writing an inaccessible
facade field. Native boxed superclass initialization and exact SWF-version behavior
remain open.

[GC:] Invocation scopes retain old/new constructor/function/owner identities and
restore the root chain and metadata after errors. Facade state retains its receiver;
prototype depth is scalar metadata. The native callback retains the facade itself.
Wire version 7 adds facade kind 37 and callback identifier 808. Metadata is checked
as a private receiver/depth record before VM allocation. Versions 1–6 remain readable;
public ABI 3 is unchanged. File and network framing accept the new graph version.

`tests/avm1_super.cpp` covers constructor chaining, actual-this method dispatch,
Function2 preload/suppression, inherited owner selection with a shadowing method,
exception propagation and subsequent calls, escaped facade GC roots, live prototype
mutation, queue transport to a context without standard objects, and native ancestor
getter/setter receiver binding. The previously unsupported no-owner preload case
now returns undefined. Existing legacy wire fixtures also exercise version 6.

Linux checks: headless 170, AGG 180, Cairo 180, minimal 161, strict C89/C++98 with
heap/global ASan and UBSan 167 checks. Managed-workspace executable permission failures
are repaired and retried. ASan stack checks and LeakSanitizer remain disabled by the
environment; the container allocation-failure case uses UBSan only and SQLite is
excluded from that sanitizer profile.

This checkpoint does not close Function2, AVM1 or roadmap stage 2. Remaining work
includes numeric super properties, full native superclass payload semantics,
SWF-version suppression/coercion rules, comprehensive proxy reflection/metamethod
behavior and independent-process facade transfer tests. The live receiver/depth
model was compared with the primary Ruffle superclass implementation
([source](https://github.com/ruffle-rs/ruffle/blob/master/core/src/avm1/object/super_object.rs));
this is not a Flash Player conformance claim.

## Русский

Добавлены именованный `super` и `PreloadSuper` Function2. `SuppressSuper` оставляет
именованное значение undefined. Вызов без определённого владельца метода или
конструктора получает undefined, без угадывания суперкласса.

Фасад удерживает настоящий получатель и глубину прототипа. Поиск выполняется
в текущей цепочке на этой глубине: унаследованный метод пропускает класс, где он
определён, даже если экземпляр принадлежит более глубокому подклассу. Возвращённый
фасад учитывает последующие изменения цепочки. `super()` использует текущий
`__constructor__` и тот же экземпляр. Методы и аксессоры получают исходный `this`.
Именованные записи используют setter предка; числовые записи пока явно возвращают
[`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48). Полная инициализация нативных boxed-суперклассов ещё открыта.

[GC:] Временные кадры удерживают старые и новые сведения о вызове и восстанавливают
их при ошибках. Фасад удерживает получатель и собственный нативный callback.
Формат графов 7 переносит вид 37 и callback 808; приватные данные проверяются до
выделения объектов VM. Чтение 1–6 и публичный ABI 3 сохранены.

Перечень тестов и результаты приведены выше. Ограничения санитайзеров, проверки
только Linux и повторные запуски при утрате прав исполнения указаны явно.
Полная семантика Function2/AVM1 не завершена: остаются числовые super-свойства,
нативные суперклассы, правила версий SWF, отражение/метаметоды фасада и его
межпроцессная передача. Этап 2 плана остаётся открытым.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
