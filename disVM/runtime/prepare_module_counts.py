# Copyright (c) 2026 bhgv
# EN: Adapt hosted Module/Type counts and short native registry transactions.
# RU: Адаптировать счётчики Module/Type и короткие операции native-реестра.
"""Adapt reviewed hosted Module/link count sites without changing pinned source."""


# EN: Replace exact reviewed source occurrences; source/changes are private text
# and replacement triples. Return adapted text or reject source-boundary drift.
# RU: Заменить точные вхождения source по тройкам changes. Вернуть личную строку
# либо отвергнуть расхождение границ исходника.
def replace(source, changes):
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError("hosted module-count boundary drift: " + before)
        source = source.replace(before, after)
    return source


# EN: Convert Type increments/preflights in private hosted link.c source.
# Return adapted text; do not alter allocation or private link construction.
# RU: Преобразовать счётчики/проверки Type в личном link.c source. Вернуть
# адаптацию, не меняя выделения или создание личного link.
def adapt_link(source):
    changes = (
        ("D2H(m->origmp)->ref == 0", "dvm_refcount_load_u(&D2H(m->origmp)->ref) == 0", 1),
        ("Tmodlink.ref++;", "dvm_runtime_type_retain(&Tmodlink.ref);", 1),
        ("t->ref++;", "dvm_runtime_type_retain(&t->ref);", 1),
        ("Tmodlink.ref <= 0", "dvm_refcount_load_i(&Tmodlink.ref) <= 0", 1),
        ("Tmodlink.ref == 0x7fffffff", "dvm_refcount_load_i(&Tmodlink.ref) == 0x7fffffff", 1),
        ("t->ref <= 0", "dvm_refcount_load_i(&t->ref) <= 0", 1),
        ("t->ref > 0x7ffffffe", "dvm_refcount_load_i(&t->ref) > 0x7ffffffe", 1),
    )
    return '#include "type_counts.h"\n#include "disvm/refcount.h"\n' + replace(source, changes)


# EN: Adapt hosted load.c source counts and atomic registry membership. Return
# private text; preserve module construction and destruction outside its mutex.
# RU: Адаптировать счётчики и атомарные операции реестра в личном load.c source.
# Вернуть текст, сохранив создание/уничтожение вне mutex реестра.
def adapt_load(source):
    changes = (
        ("m->ref++;", "dvm_runtime_module_retain(&m->ref);", 1),
        (
            "m->ref--;\n\tif(m->ref > 0)\n\t\treturn;\n\tif(m->ref == -1)\n\t\tabort();",
            "if(!dvm_runtime_module_release(&m->ref))\n\t\treturn;",
            1,
        ),
        ("h->t->ref++;", "dvm_runtime_type_retain(&h->t->ref);", 1),
        ("pt->ref <= 0", "dvm_refcount_load_i(&pt->ref) <= 0", 1),
        ("pt->ref > 0x7ffffffe", "dvm_refcount_load_i(&pt->ref) > 0x7ffffffe", 1),
        ("Tarray.ref <= 0", "dvm_refcount_load_i(&Tarray.ref) <= 0", 1),
        ("Tarray.ref == 0x7fffffff", "dvm_refcount_load_i(&Tarray.ref) == 0x7fffffff", 1),
    )
    source = replace(source, changes)
    lookup = """Module*
lookmod(char *s)
{
	Module *m;

	for(m = modules; m != nil; m = m->link)
		if(strcmp(s, m->path) == 0) {
			dvm_runtime_module_retain(&m->ref);
			return m;
		}
	return nil;
}"""
    release = """	Module **last, *mm;

	if(!dvm_runtime_module_release(&m->ref))
		return;

	last = &modules;
	for(mm = modules; mm != nil; mm = mm->link) {
		if(mm == m) {
			*last = m->link;
			break;
		}
		last = &mm->link;
	}"""
    source = replace(
        source,
        (
            (
                "\tmodule->link = modules;\n\tmodules = module;",
                "\tdvm_module_registry_publish(module);",
                1,
            ),
            ("\tm->link = modules;\n\tmodules = m;", "\tdvm_module_registry_publish(m);", 1),
            (lookup, "Module*\nlookmod(char *s)\n{\n\treturn dvm_module_registry_lookup(s);\n}", 1),
            (release, "\tif(!dvm_module_registry_release(m))\n\t\treturn;", 1),
        ),
    )
    finish = """void
unload(Module *m)
{
\tif(!dvm_module_registry_release(m))
\t\treturn;

"""
    finish_replacement = """/* [GC:] EN: Consume m's native owner; a nonfinal release republishes an
 * independently retained original MP so newly unreachable cycles get another seed.
 * RU: Снять native-владельца m; непоследний release публикует дополнительную
 * удержанную ссылку origmp для повторного поиска ставших недоступными циклов. */
void
unload(Module *m)
{
\tvoid *retry = dvm_gc_module_retry_mp(m);
\tif(!dvm_module_registry_release(m)) {
\t\tdestroy(retry);
\t\treturn;
\t}
\tdestroy(retry);
\tdvm_runtime_module_finish(m);
}


/* [GC:] EN: Finish caller-owned unlinked m outside the registry mutex.
 * Its original MP is either owned here or H after a graph consumed that edge.
 * RU: Завершить собственный удалённый из реестра m вне mutex. Исходный MP
 * принадлежит функции либо равен H после снятия ребра захватом графа. */
void
dvm_runtime_module_finish(Module *m)
{
"""
    source = replace(
        source,
        (
            (finish, finish_replacement, 1),
            (
                '#include "interp.h"',
                '#include "interp.h"\n#include "gc/fast/module_retirement.h"',
                1,
            ),
        ),
    )
    return (
        '#include "module_registry.h"\n#include "module_counts.h"\n#include "type_counts.h"\n#include "disvm/refcount.h"\n'
        + source
    )


# EN: Adapt private hosted xec.c source after opcode count conversion. Return text
# with checked self-import references; registry publication is handled separately.
# RU: Адаптировать личный xec.c source после преобразования счётчиков инструкций.
# Вернуть текст с проверкой self-ссылок; публикация реестра обрабатывается отдельно.
def adapt_xec(source):
    changes = (
        ("m->ref++;", "dvm_runtime_module_retain(&m->ref);", 1),
        ("m->ref <= 0", "dvm_refcount_load_i(&m->ref) <= 0", 1),
        ("m->ref == 0x7fffffff", "dvm_refcount_load_i(&m->ref) == 0x7fffffff", 1),
    )
    return '#include "module_counts.h"\n#include "disvm/refcount.h"\n' + replace(source, changes)


# EN: Publish complete builtin batches under the same short registry mutex.
# source is an owned source-view string; return adapted text or reject drift.
# RU: Публиковать готовые наборы builtin под тем же коротким mutex реестра.
# source — личная строка source-view; вернуть адаптацию либо отвергнуть расхождение.
def adapt_registry_runt(source):
    batch = """	head = modules;
	for(i=0; i<count; i++){
		staged[i]->link = head;
		head = staged[i];
	}
	modules = head;"""
    changes = (
        ("\t\tm->link = modules;\n\t\tmodules = m;", "\t\tdvm_module_registry_publish(m);", 1),
        ("\tModule *head;\n", "", 1),
        (batch, "\tdvm_module_registry_publish_set(staged, count);", 1),
    )
    return '#include "module_registry.h"\n' + replace(source, changes)
