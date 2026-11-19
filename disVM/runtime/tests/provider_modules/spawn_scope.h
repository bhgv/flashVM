/* Copyright (c) 2026 bhgv
 * [GC:] EN: Observe actual constructor inheritance; isolate allocation refusal and a semantic omission control.
 * RU: Проверять настоящее наследование в конструкторе, отказ выделения памяти и намеренный пропуск привязки.
 */
#ifndef PROVIDER_MODULE_SPAWN_SCOPE_H
#define PROVIDER_MODULE_SPAWN_SCOPE_H
static unsigned namespace_refusals; /* Actual refused private override allocation, before any child publication. */
static unsigned namespace_late_refusals; /* Actual context-retain refusal after the namespace selection was already staged. */
static unsigned namespace_inherits; /* Atomic successful bound-scope constructor records across child/grandchild owners. */


/* [GC:] EN/RU: Production inheritance stages native ownership on an unpublished child; parent stays rooted by its owner.
 * RU: Настоящий конструктор удерживает нативную ссылку до публикации ребёнка; родитель укоренён своим потоком.
 */
extern int __real_dvm_module_environment_inherit(Prog *child, Prog *parent);


/* [GC:] EN/RU: Forward real constructors except the explicit omission control; refuse one real calloc for variant36.
 * No namespace/result is fabricated in positive cases; overflow/rollback are checked before constructor error unwinds.
 * RU: Вызвать настоящий конструктор, кроме контроля с пропуском привязки; вариант36 отклоняет один calloc.
 * Проверить отказ до раскрутки ошибки; успешные результаты и окружения не подменяются.
 */
int __wrap_dvm_module_environment_inherit(Prog *child, Prog *parent)
{
    if (CONTROL == 37)
    {
        return 1; /* Explicit negative control keeps the inherited base group but omits its bound-instance selection. */
    }
    int failing = CONTROL == 36 && !namespace_refusals && parent && parent == currun() &&
        dvm_module_environment_current(parent);
    if (failing)
    {
        binding_allocation = 1;
    }
    int result = __real_dvm_module_environment_inherit(child, parent);
    if (failing)
    {
        launch_check(!binding_allocation && !result && !((DVM_RuntimeProcess *)child)->module_namespaces,
            "native namespace inheritance allocation did not refuse atomically");
        namespace_refusals++;
    }
    if (result && ((DVM_RuntimeProcess *)child)->module_namespaces)
    {
        __atomic_add_fetch(&namespace_inherits, 1, __ATOMIC_RELAXED);
    }
    return result;
}


/* [GC:] EN/RU: Original constructor retains immutable context and optional terminal grants; child is still unpublished.
 * RU: Исходный конструктор удерживает контекст и права терминала; ребёнок ещё не опубликован.
 */
extern int __real_dvm_launch_process_inherit(Prog *child, Prog *parent);


/* [GC:] EN/RU: Exercise real context-refcount overflow after successful scope staging, then permit an ordinary retry.
 * The selected fixture context has no other executing child/borrower yet and is pinned by the active module frame.
 * RU: Проверить переполнение счётчика контекста после удержания namespace, затем разрешить обычную повторную попытку.
 * Контекст закреплён кадром модуля; до публикации ребёнка у него один владелец.
 */
int __wrap_dvm_launch_process_inherit(Prog *child, Prog *parent)
{
    DVM_LaunchContext *context = CONTROL == 38 && parent == currun() ? dvm_module_launch_current(parent) : NULL;
    int failing = context && !namespace_late_refusals;
    if (failing)
    {
        launch_check(((DVM_RuntimeProcess *)child)->module_namespaces && context->references == 1,
            "late constructor failure did not follow private scope staging");
        context->references = UINT_MAX;
    }
    int result = __real_dvm_launch_process_inherit(child, parent);
    if (failing)
    {
        context->references = 1;
        launch_check(!result && !((DVM_RuntimeProcess *)child)->launch_context,
            "overflowed context was partially installed in a private child");
        namespace_late_refusals++;
    }
    return result;
}

#endif
