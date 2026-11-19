/* Copyright (c) 2026 bhgv
 * EN: Checked fn(int):int owner generation; Coco/R supplies all declarations/types.
 * The adapter calls the provider's normal Dis module, never a copied native frame.
 * RU: Проверенная генерация owner fn(int):int; все declarations/types даёт Coco/R.
 * Адаптер вызывает обычный Dis module провайдера, не копирует native frames.
 */
#include "remote_owner.h"
#include <string.h>


/* EN: Validate borrowed include/path spelling before interpolating a Limbo string.
 * slash permits provider paths; includes remain simple basenames. Return 1/0;
 * no output, ownership or filesystem effects. Limit both to 255 bytes.
 * RU: Проверить include/path до вставки в Limbo string. slash разрешает пути
 * провайдера; includes остаются basenames. Вернуть 1/0 без output/ownership/I/O.
 * Оба значения ограничены 255 bytes.
 */
static int spelling(const char *text, int slash)
{
    size_t i;
    if (!text || !*text || strlen(text) > 255)
    {
        return 0;
    }
    for (i = 0; text[i]; i++)
    {
        unsigned char ch = (unsigned char)text[i];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
              (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.' ||
              (slash && ch == '/')))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Report borrowed message in optional caller buffer, return failure=0.
 * No retained pointers; capacity includes NUL and may be zero.
 * RU: Записать message в optional caller buffer, вернуть отказ=0. Указатели
 * не сохраняются; capacity включает NUL и может быть нулём.
 */
static int refused(char *error, size_t capacity, const char *message)
{
    if (error && capacity)
    {
        snprintf(error, capacity, "%s", message);
    }
    return 0;
}


/* EN: Reject module names shadowing imported modules or generated locals/exports.
 * Borrow name, return 1/0 without allocation or parser state access.
 * RU: Отвергнуть module names, скрывающие imports/locals/exports адаптера.
 * Заимствовать name, вернуть 1/0 без allocation и parser state.
 */
static int reserved(const char *name)
{
    static const char *const names[] = {
        "Sys", "Processctl", "Proxyhost", "Proxyref", "Proxygc", "Proxyhandler",
        "remote_target", "control", "capture", "left", "right", "value", "kind",
        "refs", "deadline", "start", "invoke", "result", "routed", "routedresult"
    }; /* Adapter-owned names / Имена адаптера. */
    size_t i;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    {
        if (!strcmp(name, names[i]))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See remote_owner.h. Preflight the complete supported shape, then emit.
 * См. remote_owner.h. Сначала проверить весь supported shape, затем генерировать.
 */
int limbo_remote_owner(FILE *out, const LimboInterface *catalog,
                       const char *include_name, const char *implementation,
                       char *error, size_t capacity)
{
    LimboDeclaration module, function;
    LimboSignature signature;
    LimboParameter parameter;
    LimboDiagnostic diagnostic;
    const char *kind;
    if (error && capacity)
    {
        error[0] = 0;
    }
    if (!catalog || !spelling(include_name, 0) || !spelling(implementation, 1) ||
        limbo_interface_count(catalog) != 2 ||
        !limbo_interface_declaration(catalog, 0, &module) || module.kind != LIMBO_MODULE ||
        !limbo_interface_declaration(catalog, 1, &function) ||
        function.kind != LIMBO_FUNCTION || function.scope != 0 ||
        !limbo_interface_validate_functions(catalog, &diagnostic) ||
        !limbo_interface_signature(catalog, 1, &signature) ||
        signature.parameter_count != 1 || signature.variadic || signature.polymorphic ||
        !limbo_interface_parameter(catalog, 1, 0, &parameter) || parameter.self_parameter)
    {
        return refused(error, capacity, "requires one module with one fn(int):int export");
    }
    kind = limbo_interface_type_kind(catalog, parameter.type_node);
    if (!kind || strcmp(kind, "int") || signature.result_node == SIZE_MAX ||
        !(kind = limbo_interface_type_kind(catalog, signature.result_node)) || strcmp(kind, "int") ||
        reserved(module.name))
    {
        return refused(error, capacity, "unsupported type or adapter module-name collision");
    }
    if (!out)
    {
        return 1;
    }
    fprintf(out,
        "# Copyright (c) 2026 bhgv\n"
        "# EN: Generated owner adapter; implementation stays in the provider VM.\n"
        "# RU: Generated owner adapter; реализация остаётся в VM провайдера.\n"
        "implement Proxyhandler;\n"
        "include \"sys.m\";\ninclude \"processctl.m\";\n"
        "include \"proxyhost.m\";\ninclude \"proxyref.m\";\n"
        "include \"proxyhandler.m\";\ninclude \"%s\";\n"
        "remote_target: %s; # Strong module owner / Владеющая ссылка на module.\n\n"
        "# EN: Load the original implementation on this permanent owner.\n"
        "# RU: Загрузить реализацию на этом постоянном owner.\n"
        "start(capture: ref Proxyhost->Cell)\n{\n"
        "    remote_target = load %s \"%s\";\n"
        "    if (remote_target == nil) { raise \"remote implementation link failed\"; }\n"
        "    control := load Processctl Processctl->PATH;\n"
        "    capture.value = control->pid();\n}\n\n"
        "# EN: Copy one scalar into the real Dis call; refs are not in this profile.\n"
        "# RU: Передать scalar настоящему Dis call; refs не входят в профиль.\n"
        "invoke(capture, left, right: ref Proxyhost->Cell, value: int): int\n{\n"
        "    control := load Processctl Processctl->PATH;\n"
        "    if (capture.value != control->pid() || left != nil || right != nil) {\n"
        "        raise \"remote owner or argument profile mismatch\";\n    }\n"
        "    return remote_target->%s(value);\n}\n\n",
        include_name, module.name, module.name, implementation, function.name);
    fputs(
        "# EN/RU: Unsupported result/reference operations fail explicitly.\n"
        "# Неподдержанные result/reference operations явно отказывают.\n"
        "result(capture, left, right: ref Proxyhost->Cell, value, kind: int): ref Proxyhost->Cell\n"
        "{\n    raise \"scalar owner has no proxy results\";\n}\n\n"
        "# EN/RU: Routed references are outside this scalar profile.\n"
        "# Routed references не входят в scalar profile.\n"
        "routed(capture: ref Proxyhost->Cell, left, right: ref Proxyref->Ref,\n"
        "       refs, value, deadline: int): int\n"
        "{\n    raise \"scalar owner has no routed arguments\";\n}\n\n"
        "# EN/RU: No routed proxy result is created.\n"
        "# Routed proxy result не создаётся.\n"
        "routedresult(capture: ref Proxyhost->Cell, left, right: ref Proxyref->Ref,\n"
        "             refs, value, deadline: int): ref Proxyref->Ref\n"
        "{\n    raise \"scalar owner has no routed results\";\n}\n", out);
    return ferror(out) ? refused(error, capacity, "owner output failed") : 1;
}
