# Copyright (c) 2026 bhgv
# EN: Adapt native leaf code publication and remove obsolete Loader relocation writes.
# RU: Адаптировать публикацию native leaf-кода и убрать старые relocation-записи Loader.
from prepare_debug_dispatch import adapt as adapt_debug_dispatch


# EN: Replace reviewed borrowed code boundaries using checked replacement callback.
# Return owned adapted source, or propagate drift errors; no file mutation here.
# RU: Заменить проверенные границы code через checked replacement callback.
# Вернуть адаптированный текст либо ошибку дрейфа; файлы здесь не меняются.
def compiler(code, replace):
    for before, after in (
        ('\tif(m->jit != nil){\n\t\tj=m->jit;', '\tj=dvm_jit_load(m);\n\tif(j != nil){'),
        ('\tm->jit=j;', '\tdvm_jit_publish(m,j);'),
        ('\tok=kprotect(leafcompile,&c);', '\tdvm_jit_build_lock();\n\tok=kprotect(leafcompile,&c);'),
        ('\treturn ok;\n}', '\tdvm_jit_build_unlock();\n\treturn ok;\n}'),
    ):
        code = replace(code, before, after, 1)
    return '#include "jit_publication.h"\n' + code


# EN: Adapt borrowed final xec source with an acquire-read of the immutable table.
# replace rejects boundary drift; return owned text without file mutation.
# RU: Адаптировать итоговый xec: acquire-чтение неизменяемой таблицы.
# replace отвергает дрейф границ; вернуть текст без изменения файлов.
def dispatch(code, replace):
    code = replace(code, '\t\tmemmove(&j, (uchar*)fetched.module+O(Module, jit), sizeof(j));',
                   '\t\tj = dvm_jit_select(fetched.module, DVM_R.PC, DVM_R.FP);', 1)
    return '#include "jit_publication.h"\n#include "jit_quarantine.h"\n' + adapt_debug_dispatch(code, replace)


# EN: Remove old relocated-PC backend mutations from borrowed hosted Loader code.
# Native leaf compile uses original bytecode/MP and never changes Modlink layout.
# Return owned text or propagate checked boundary drift.
# RU: Убрать мутации старого relocated-PC backend из hosted Loader.
# Native leaf compile использует исходный bytecode/MP и не меняет Modlink.
# Вернуть текст либо передать ошибку проверенных границ.
def loader(code, replace):
    before = '''\t\tsavedmp=m->origmp;
\t\tm->origmp=f->mp->MP;
\t\tok=kprotect(loadercompile, f);
\t\tm->origmp=savedmp;
\t\tif(ok) {
\t\t\tf->mp->prog=m->prog;
\t\t\tf->mp->compiled=m->compiled;
\t\t} else
\t\t\tloaderword(f->ret, -1);'''
    after = '''        /* Leaf compilation retains original bytecode PCs and module data. */
        ok=kprotect(loadercompile, f);
        if(!ok) {
            loaderword(f->ret, -1);
        }'''
    code = replace(code, before, after, 1)
    return replace(code, '\tuchar *savedmp;\n', '', 1)
