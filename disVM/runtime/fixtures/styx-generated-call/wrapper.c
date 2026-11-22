/* Copyright (c) 2026 bhgv
 * EN: Execute synthesized Styx call instructions in a normal Dis process.
 * RU: Выполнить инструкции вызова Styx в обычном процессе Dis.
 */
#include "lib9.h"
#include "interp.h"
#include "isa.h"
#include "vfs/scalar_direct.h"
#include <stdlib.h>
#include <string.h>

void __real_xec(Prog *process);


/* EN: Replace the exact fixture function body before the first execution.
 * RU: Заменить тело проверочной функции перед первым исполнением. */
void __wrap_xec(Prog *process)
{
    static int installed;
    if (!installed)
    {
        Module *module = process->R.M->m;
        for (Link *link = module->ext; link && link->name; link++)
        {
            if (!strcmp(link->name, "go"))
            {
                Inst *entry = link->u.pc;
                if (entry < module->prog || entry >= module->prog + module->nprog || module->prog + module->nprog - entry < 8 ||
                    entry[0].op != IMFRAME || entry[6].op != IMCALL || entry[7].op != IRET ||
                    dvm_styx_call_code("(ysa[y]b)", 0, 1, entry, 8) != 8)
                {
                    abort();
                }
                installed = 1;
                fprint(2, "STYX generated call installed\n");
                break;
            }
        }
    }
    __real_xec(process);
}



Module *__real_dvm_namespace_read_imports(char *path, Module *cached, int sync, const Import *imports);


/* EN: Supply exactly the private fixture module; keep normal load/link handling.
 * RU: Подставить только приватный тестовый модуль, сохраняя штатный load/link. */
Module *__wrap_dvm_namespace_read_imports(char *path, Module *cached, int sync, const Import *imports)
{
    int ref = !strcmp(path, "./generated-ref-gateway.dis");
    if (!ref && strcmp(path, "./generated-gateway.dis"))
    {
        return __real_dvm_namespace_read_imports(path, cached, sync, imports);
    }
    if (!imports || !imports[0].name || strcmp(imports[0].name, "invoke") || imports[1].name)
    {
        abort();
    }
    return dvm_styx_call_module(ref ? "(r[ib]r[ib])" : "(ysa[y]b)", ref ? 1 : 0, 1, (uint32_t)imports[0].sig);
}
