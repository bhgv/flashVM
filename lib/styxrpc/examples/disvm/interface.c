/* Copyright (c) 2026 bhgv
 * EN: Generate the device's .m from the registered native adjust export.
 * RU: Генерация .m устройства из зарегистрированного экспорта adjust.
 */
#include "../adjust.h"
#include "../graph_device.h"
#include "styxrpc/interface.h"
#include <stdio.h>


/* EN: Write the complete Device interface to stdout, or fail without a partial declaration.
 * RU: Выдать полный интерфейс Device в stdout либо сообщить об отказе. */
int main(void)
{
    static const StyxRpcSchema scalar = {"(i)", "(i)"};
    static const char *const fields[] = {"counter"};
    static const char *const box[] = {"inner"};
    const StyxRpcAdt adts[] = {{"r[i]", "Cell", fields, 1}, {"r[r[i]]", "Box", box, 1}};
    const StyxRpcDeclaration exports[] = {
        {"sample", &scalar}, {adjust_export.name, &adjust_export.schema}, {"bump", &bump_schema}
    };
    char source[512];
    size_t used;
    if (styxrpc_module_source("Device", exports, sizeof(exports) / sizeof(*exports), adts, sizeof(adts) / sizeof(*adts),
        source, sizeof(source), &used) != STYXRPC_OK)
    {
        return 1;
    }
    return fwrite(source, 1, used, stdout) == used ? 0 : 1;
}
