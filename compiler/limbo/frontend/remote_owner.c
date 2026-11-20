/* Copyright (c) 2026 bhgv
 * EN: Checked fn(int):int owner generation; Coco/R supplies all declarations/types.
 * The adapter calls the provider's normal Dis module, never a copied native frame.
 * RU: Проверенная генерация owner fn(int):int; все declarations/types даёт Coco/R.
 * Адаптер вызывает обычный Dis module провайдера, не копирует native frames.
 */
#include "remote_owner.h"
#include "resolve.h"
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


/* EN: Find one module and one function, permitting constants/type aliases only.
 * Borrow catalog; publish declaration indices only on success. No include lookup
 * or constant evaluation; the normal Dis compiler still checks the whole source.
 * RU: Найти один module и одну function, допуская constants/type aliases.
 * Заимствовать catalog, публиковать indices только при успехе. Без include lookup
 * и constant evaluation; весь source проверяет обычный Dis compiler.
 */
static int selection(const LimboInterface *catalog, size_t *module, size_t *function)
{
    size_t m = SIZE_MAX;
    size_t f = SIZE_MAX;
    size_t i;
    LimboDeclaration declaration;
    for (i = 0; i < limbo_interface_count(catalog); i++)
    {
        if (!limbo_interface_declaration(catalog, i, &declaration))
        {
            return 0;
        }
        if (declaration.kind == LIMBO_MODULE && m == SIZE_MAX)
        {
            m = i;
        }
        else if (declaration.kind == LIMBO_FUNCTION && f == SIZE_MAX)
        {
            f = i;
        }
        else if (declaration.kind != LIMBO_CONSTANT && declaration.kind != LIMBO_ALIAS)
        {
            return 0;
        }
    }
    if (m == SIZE_MAX || f == SIZE_MAX ||
        !limbo_interface_declaration(catalog, f, &declaration) || declaration.scope != m)
    {
        return 0;
    }
    *module = m;
    *function = f;
    return 1;
}


/* EN: Resolve a borrowed type in its lexical scope and require scalar int.
 * Return 1/0; propagate no native frame or inferred layout. Resolver owns temps.
 * RU: Разрешить type в lexical scope, требовать scalar int. Вернуть 1/0;
 * native frames и guessed layout не передаются. Temps принадлежат resolver.
 */
static int integer_type(const LimboInterface *catalog, size_t scope, size_t type)
{
    size_t resolved;
    const char *kind;
    if (!limbo_interface_resolve_type(catalog, scope, type, &resolved, NULL))
    {
        return 0;
    }
    kind = limbo_interface_type_kind(catalog, resolved);
    return kind && !strcmp(kind, "int");
}


/* EN/RU: Borrow the parsed ABI and copy the two supported declarations on success.
 * Заимствовать разобранный ABI, скопировать два объявления при успехе. */
static int remote_signature(const LimboInterface *catalog, LimboDeclaration *module,
    LimboDeclaration *function, char *error, size_t capacity)
{
    LimboSignature signature;
    LimboParameter parameter;
    LimboDiagnostic diagnostic;
    size_t module_index;
    size_t function_index;
    if (error && capacity)
    {
        error[0] = 0;
    }
    if (!catalog || !selection(catalog, &module_index, &function_index) ||
        !limbo_interface_declaration(catalog, module_index, module) ||
        !limbo_interface_declaration(catalog, function_index, function) ||
        !limbo_interface_validate_functions(catalog, &diagnostic) ||
        !limbo_interface_signature(catalog, function_index, &signature) ||
        signature.parameter_count != 1 || signature.variadic || signature.polymorphic ||
        !limbo_interface_parameter(catalog, function_index, 0, &parameter) || parameter.self_parameter)
    {
        return refused(error, capacity, "requires one module with one fn(int):int export");
    }
    if (!integer_type(catalog, function->scope, parameter.type_node) ||
        signature.result_node == SIZE_MAX ||
        !integer_type(catalog, function->scope, signature.result_node) ||
        reserved(module->name))
    {
        return refused(error, capacity, "unsupported type or adapter module-name collision");
    }
    return 1;
}


/* EN/RU: See remote_owner.h; preflight the complete supported shape.
 * См. remote_owner.h; проверить поддерживаемую форму до записи. */
int limbo_remote_owner(FILE *out, const LimboInterface *catalog,
    const char *include_name, const char *implementation, char *error, size_t capacity)
{
    LimboDeclaration module, function;
    if (!spelling(include_name, 0) || !spelling(implementation, 1))
    {
        return refused(error, capacity, "invalid include or implementation name");
    }
    if (!remote_signature(catalog, &module, &function, error, capacity))
    {
        return 0;
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


// EN/RU: See remote_owner.h; generate a typed client with explicit binding.
// См. remote_owner.h; сгенерировать typed client с явной привязкой.
int limbo_remote_client(FILE *out, const LimboInterface *catalog, const char *include_name,
    int interface_only, char *error, size_t capacity)
{
    LimboDeclaration module, function;
    if ((!interface_only && !spelling(include_name, 0)) ||
        !remote_signature(catalog, &module, &function, error, capacity))
    {
        return 0;
    }
    if (!strcmp(function.name, "remote_bind") || !strcmp(function.name, "remote_net") ||
        !strcmp(function.name, "remote_endpoint") || !strcmp(function.name, "remote_image") ||
        !strcmp(function.name, "remote_timeout"))
    {
        return refused(error, capacity, "export collides with generated client state");
    }
    if (!out)
    {
        return 1;
    }
    fputs("# Copyright (c) 2026 bhgv\n"
          "# EN: Generated typed remote client.\n"
          "# RU: Сгенерированный типизированный удалённый клиент.\n", out);
    if (interface_only)
    {
        fprintf(out, "%sRemote: module\n{\n"
            "    # Bind once before sharing; timeout is in milliseconds.\n"
            "    # Привязать до передачи другим владельцам; время в мс.\n"
            "    remote_bind: fn(endpoint: ref Proxynet->Endpoint, image: Proxynet->Image, timeout: int);\n"
            "    %s: fn(value: int): int; # Signature resolved from the original interface.\n"
            "};\n", module.name, function.name);
    }
    else
    {
        fprintf(out,
            "implement %sRemote;\n"
            "include \"sys.m\";\ninclude \"proxyhost.m\";\ninclude \"proxyref.m\";\n"
            "include \"proxyhandler.m\";\ninclude \"proxyowner.m\";\ninclude \"proxynet.m\";\n"
            "include \"%s\";\n"
            "remote_net: Proxynet; # Owned implementation / Собственный module.\n"
            "remote_endpoint: ref Proxynet->Endpoint; # Retained endpoint / Удержанный endpoint.\n"
            "remote_image: Proxynet->Image; # Retained target / Удержанный target.\n"
            "remote_timeout: int; # Per-call timeout in ms / Срок вызова в мс.\n\n"
            "# Bind one live target before sharing / Привязать одну цель до передачи.\n"
            "remote_bind(endpoint: ref Proxynet->Endpoint, image: Proxynet->Image, timeout: int)\n{\n"
            "    if (remote_endpoint != nil || endpoint == nil || timeout < 1 || timeout > 60000 ||\n"
            "        image.high != endpoint.high || image.low != endpoint.low) {\n"
            "        raise \"remote client binding refused\";\n    }\n"
            "    net := load Proxynet Proxynet->PATH;\n"
            "    if (net == nil) { raise \"remote client broker unavailable\"; }\n"
            "    net->init();\n"
            "    remote_net = net;\n    remote_image = image;\n    remote_timeout = timeout;\n"
            "    remote_endpoint = endpoint;\n}\n\n"
            "# Invoke the typed export once / Один вызов типизированного экспорта.\n"
            "%s(value: int): int\n{\n"
            "    if (remote_endpoint == nil) { raise \"remote client is unbound\"; }\n"
            "    empty := Proxynet->Image(big 0, big 0, Proxyowner->Handle(big 0, 0, big 0), nil);\n"
            "    (status, result) := remote_net->call(remote_endpoint, Proxyowner->Invoke,\n"
            "        remote_image, empty, empty, 0, value, remote_timeout);\n"
            "    if (status == Proxyowner->OwnerError) { raise \"remote owner error\"; }\n"
            "    if (status == Proxyowner->NotSent) { raise \"remote call not sent\"; }\n"
            "    if (status == Proxyowner->Unknown) { raise \"remote call outcome unknown\"; }\n"
            "    if (status == Proxynet->Disconnected) { raise \"remote client disconnected\"; }\n"
            "    if (status != 0) { raise \"remote call failed\"; }\n"
            "    return result;\n}\n", module.name, include_name, function.name);
    }
    return ferror(out) ? refused(error, capacity, "client output failed") : 1;
}


/* EN: Parse a nonzero decimal wire tag without accepting signs or overflow.
 * RU: Прочитать ненулевой десятичный тег без знака и переполнения. */
static unsigned long route_tag(const char *text)
{
    unsigned long tag = 0;
    for (; *text; text++)
    {
        if (*text < '0' || *text > '9' || tag > (0xffffffffUL - (unsigned)(*text - '0')) / 10)
        {
            return 0;
        }
        tag = tag * 10 + (unsigned)(*text - '0');
    }
    return tag;
}


/* EN: Validate borrowed route spellings and return checked numeric tags.
 * RU: Проверить заимствованные имена и вернуть числовые теги маршрута. */
static int styx_route(const char *include_name, const char *request, const char *reply,
    const char *request_tag, const char *reply_tag, unsigned long *request_id, unsigned long *reply_id)
{
    *request_id = request_tag ? route_tag(request_tag) : 0;
    *reply_id = reply_tag ? route_tag(reply_tag) : 0;
    return spelling(include_name, 0) && spelling(request, 1) && spelling(reply, 1) &&
        request[0] == '/' && reply[0] == '/' && strcmp(request, reply) && *request_id && *reply_id;
}


/* EN/RU: See remote_owner.h. Preflight inputs before any write.
 * См. remote_owner.h. Проверить параметры до записи. */
int limbo_styx_client(FILE *output, const LimboInterface *catalog, const char *include_name,
    const char *request, const char *reply, const char *request_tag, const char *reply_tag,
    char *error, size_t capacity)
{
    LimboDeclaration module, function;
    unsigned long send, receive;
    if (!styx_route(include_name, request, reply, request_tag, reply_tag, &send, &receive))
    {
        return refused(error, capacity, "invalid Styx route paths or tags");
    }
    if (!remote_signature(catalog, &module, &function, error, capacity))
    {
        return 0;
    }
    if (!strcmp(module.name, "Vfsnative") || !strcmp(function.name, "Vfsnative") ||
        !strcmp(function.name, "route") || !strcmp(function.name, "Sys") ||
        !strcmp(include_name, "vfsnative.m") || !strcmp(include_name, "sys.m"))
    {
        return refused(error, capacity, "interface collides with generated transport name");
    }
    if (!output)
    {
        return 1;
    }
    fprintf(output,
        "# Copyright (c) 2026 bhgv\n"
        "# EN: Original module ABI backed by mounted Unibus routes.\n"
        "# RU: Исходный ABI модуля через смонтированные маршруты Unibus.\n"
        "implement %s;\ninclude \"sys.m\";\ninclude \"vfsnative.m\";\ninclude \"%s\";\n\n"
        "# EN/RU: Invoke via the published routes / Вызвать через опубликованные маршруты.\n"
        "%s(value: int): int\n{\n"
        "    route := load Vfsnative Vfsnative->PATH;\n"
        "    if (route == nil) { raise \"scalar transport unavailable\"; }\n"
        "    return route->scalarcall(\"%s\", \"%s\", big %lu, big %lu, value);\n}\n",
        module.name, include_name, function.name, request, reply, send, receive);
    return ferror(output) ? refused(error, capacity, "client output failed") : 1;
}


/* EN/RU: Generate a one-shot owner that loads its original typed implementation.
 * Сгенерировать одноразового владельца, который загружает исходную реализацию. */
int limbo_styx_owner(FILE *output, const LimboInterface *catalog, const char *include_name,
    const char *implementation, const char *request, const char *reply,
    const char *request_tag, const char *reply_tag, char *error, size_t capacity)
{
    LimboDeclaration module, function;
    unsigned long receive, send;
    if (!spelling(implementation, 1))
    {
        return refused(error, capacity, "invalid owner implementation path");
    }
    if (!styx_route(include_name, request, reply, request_tag, reply_tag, &receive, &send))
    {
        return refused(error, capacity, "invalid Styx route paths or tags");
    }
    if (!remote_signature(catalog, &module, &function, error, capacity))
    {
        return 0;
    }
    if (!strcmp(module.name, "StyxScalarOwner") || !strcmp(module.name, "Vfsnative") ||
        !strcmp(module.name, "init") || !strcmp(include_name, "vfsnative.m") ||
        !strcmp(include_name, "sys.m"))
    {
        return refused(error, capacity, "interface collides with generated owner");
    }
    if (!output)
    {
        return 1;
    }
    fprintf(output,
        "# Copyright (c) 2026 bhgv\n"
        "# EN: Serve one call in the installation owning the original Dis module.\n"
        "# RU: Обслужить один вызов там, где опубликован исходный модуль Dis.\n"
        "implement StyxScalarOwner;\ninclude \"sys.m\";\ninclude \"vfsnative.m\";\n"
        "include \"%s\";\nStyxScalarOwner: module { init: fn(); };\n\n"
        "# EN/RU: Serve one granted request / Обслужить один выданный запрос.\n"
        "init()\n{\n"
        "    target := load %s \"%s\";\n"
        "    route := load Vfsnative Vfsnative->PATH;\n"
        "    if (target == nil || route == nil) { raise \"scalar owner load failed\"; }\n"
        "    value := route->scalartake(\"%s\", big %lu);\n"
        "    result := 0;\n"
        "    success := 1;\n"
        "    {\n        result = target->%s(value);\n"
        "    } exception {\n        \"*\" => success = 0; result = 1;\n    }\n"
        "    route->scalarreply(\"%s\", big %lu, success, result);\n"
        "}\n", include_name, module.name, implementation, request,
        receive, function.name, reply, send);
    return ferror(output) ? refused(error, capacity, "owner output failed") : 1;
}
