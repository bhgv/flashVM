/* External module scaffolds from the existing Coco/R declaration catalog.
 * This pass never reparses Limbo types or guesses Dis/native frame layouts. */
#include "interface.h"
#include <stdlib.h>
#include <string.h>
#include "bindings_as_runtime.h"

/* Quote original UTF-8 bytes for JSON/Python or portable C string literals. */
static void quoted(FILE *out, const char *text, size_t length, int c_string)
{
    size_t i;
    fputc('"', out);
    for (i = 0; i < length; ++i)
    {
        unsigned ch = (unsigned char)text[i];
        if (!c_string && ch == 0xe2 && i + 2 < length &&
            (unsigned char)text[i + 1] == 0x80 &&
            ((unsigned char)text[i + 2] == 0xa8 || (unsigned char)text[i + 2] == 0xa9))
        {
            fprintf(out, "\\u202%c", (unsigned char)text[i + 2] == 0xa8 ? '8' : '9');
            i += 2;
        }
        else if (ch == '"' || ch == '\\')
            fprintf(out, "\\%c", ch);
        else if (c_string && (ch < 32 || ch >= 127))
            fprintf(out, "\\%03o", ch);
        else if (ch < 32)
            fprintf(out, "\\u%04x", ch);
        else
            fputc(ch, out);
    }
    fputc('"', out);
}

/* Stable quick snapshot tag shared by all generated languages. It is not a
 * cryptographic authenticator or the native Dis structural signature. Hosts must
 * compare/resolve the complete .m descriptor before binding an endpoint. */
static void interface_id(const LimboInterface *catalog, char out[40])
{
    const LimboTree *tree = limbo_interface_tree(catalog);
    const unsigned char *bytes = (const unsigned char *)limbo_tree_source(tree);
    uint64_t hash = UINT64_C(14695981039346656037);
    size_t i;
    for (i = 0; i < limbo_tree_size(tree); ++i)
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
    snprintf(out, 40, "limbo-m1-%08lx%08lx", (unsigned long)(hash >> 32),
             (unsigned long)(hash & UINT64_C(0xffffffff)));
}

/* Resolve one declaration's authored physical line and UTF-8 byte column. */
static void position(const char *source, size_t offset, size_t *cursor, size_t *line, size_t *column)
{
    size_t i;
    if (offset < *cursor)
    {
        *cursor = 0;
        *line = *column = 1;
    }
    for (i = *cursor; i < offset; ++i)
    {
        if (source[i] == '\n')
        {
            ++*line;
            *column = 1;
        }
        else
            ++*column;
    }
    *cursor = offset;
}

/* Emit an ABI-neutral declaration/debug manifest for other language generators.
 * Includes are recorded, never opened; type spelling is deliberately unresolved. */
static void manifest(FILE *out, const LimboInterface *catalog)
{
    const LimboTree *tree = limbo_interface_tree(catalog);
    const char *source = limbo_tree_source(tree);
    size_t i, cursor = 0, line = 1, column = 1;
    LimboDeclaration d;
    char identity[40];
    interface_id(catalog, identity);
    fprintf(out, "{\"interface_id\":\"%s\",", identity);
    fputs("\"schema\":1,\"format\":\"LIMBO-INTERFACE-1\",\"source\":", out);
    quoted(out, source, limbo_tree_size(tree), 0);
    fputs(",\"declarations\":[", out);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        position(source, d.name_begin, &cursor, &line, &column);
        fprintf(out, "%s{\"index\":%zu,\"kind\":", i ? "," : "", i);
        quoted(out, limbo_declaration_kind(d.kind), strlen(limbo_declaration_kind(d.kind)), 0);
        fputs(",\"name\":", out);
        quoted(out, d.name, strlen(d.name), 0);
        fputs(",\"scope\":", out);
        if (d.scope == SIZE_MAX)
            fputs("null", out);
        else
            fprintf(out, "%zu", d.scope);
        fprintf(out, ",\"byte_offset\":%zu,\"byte_length\":%zu,\"line\":%zu,\"column\":%zu,\"type\":",
                d.name_begin, d.name_end - d.name_begin, line, column);
        quoted(out, source + d.type_begin, d.type_end - d.type_begin, 0);
        fputc('}', out);
    }
    fputs("]}\n", out);
}

/* Emit compilable C99 callbacks which explicitly report unimplemented behavior.
 * Opaque call values require a later checked host bridge; no heap/frame ABI is guessed. */
static void c_scaffold(FILE *out, const LimboInterface *catalog, const char *prefix)
{
    const LimboTree *tree = limbo_interface_tree(catalog);
    const char *source = limbo_tree_source(tree);
    size_t i, count = 0;
    LimboDeclaration d, scope;
    char identity[40];
    interface_id(catalog, identity);
    fprintf(out, "const char %s_interface_id[] = \"%s\";\n", prefix, identity);
    fputs("/* Generated from a Coco/R-validated .m snapshot. Implement callbacks before registration. */\n"
          "#include <stddef.h>\n\n"
          "/* Host-owned opaque call, not a Dis frame or channel wire structure. */\n"
          "struct LimboExternalCall;\n", out);
    fprintf(out, "typedef int (*%s_callback)(void *, struct LimboExternalCall *);\n", prefix);
    fprintf(out, "/* Metadata borrows immutable generated strings; calls borrow host-owned arguments. */\n"
            "struct %s_export { const char *module; const char *name; const char *limbo_type; "
            "size_t declaration; size_t byte_offset; %s_callback call; };\n\n", prefix, prefix);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        if (d.kind != LIMBO_FUNCTION)
            continue;
        fprintf(out, "/* TODO: implement declaration %zu; no success is fabricated by this scaffold. */\n"
                "static int %s_call_%zu(void *host, struct LimboExternalCall *call)\n"
                "{\n    (void)host;\n    (void)call;\n    return -1;\n}\n\n", i, prefix, i);
    }
    fprintf(out, "const struct %s_export %s_exports[] = {\n", prefix, prefix);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        if (d.kind != LIMBO_FUNCTION)
            continue;
        fputs("    {", out);
        if (d.scope != SIZE_MAX)
        {
            limbo_interface_declaration(catalog, d.scope, &scope);
            quoted(out, scope.name, strlen(scope.name), 1);
        }
        else
            quoted(out, "", 0, 1);
        fputs(", ", out);
        quoted(out, d.name, strlen(d.name), 1);
        fputs(", ", out);
        quoted(out, source + d.type_begin, d.type_end - d.type_begin, 1);
        fprintf(out, ", %zu, %zu, %s_call_%zu},\n", i, d.name_begin, prefix, i);
        ++count;
    }
    fputs("    {NULL, NULL, NULL, 0, 0, NULL}\n};\n", out);
    fprintf(out, "const size_t %s_export_count = %zu;\n", prefix, count);
}

/* Emit a Python callback registry as another external-language starting point.
 * Python is neither required by the C generator nor added to the deployed IDE. */
static void python_scaffold(FILE *out, const LimboInterface *catalog, const char *prefix)
{
    size_t i;
    LimboDeclaration d;
    char identity[40];
    interface_id(catalog, identity);
    fprintf(out, "%s_interface_id = \"%s\"\n", prefix, identity);
    fputs("# Generated external callbacks; the host bridge owns each call.\n\n", out);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        if (d.kind != LIMBO_FUNCTION)
            continue;
        fprintf(out, "# Implement declaration %zu before registering this callback.\n"
                "def %s_call_%zu(host, call):\n    raise NotImplementedError(", i, prefix, i);
        quoted(out, d.name, strlen(d.name), 0);
        fputs(")\n\n", out);
    }
    fprintf(out, "%s_exports = {\n", prefix);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        if (d.kind == LIMBO_FUNCTION)
            fprintf(out, "    %zu: %s_call_%zu,\n", i, prefix, i);
    }
    fputs("}\n", out);
}

/* Substitute only our fixed template marker; module/source data are separately
 * quoted and never interpreted as a format string or executable identifier. */
static void as_template(FILE *out, const char *text, const char *prefix)
{
    const char *next;
    while ((next = strstr(text, "@P@")) != NULL)
    {
        fwrite(text, 1, (size_t)(next - text), out);
        fputs(prefix, out);
        text = next + 3;
    }
    fputs(text, out);
}

/* Generate both sides of a shared .m contract: non-Dis AS implementation or
 * AS proxy for a Dis endpoint. Transport marshaling/official signature checking
 * remains a host responsibility, never an invented native frame layout. */
static void as_scaffold(FILE *out, const LimboInterface *catalog, const char *prefix, int proxy)
{
    const LimboTree *tree = limbo_interface_tree(catalog);
    const char *source = limbo_tree_source(tree);
    size_t i, count = 0;
    char identity[40];
    LimboDeclaration d, scope;
    interface_id(catalog, identity);
    fprintf(out, "// Generated from one Coco/R-validated .m snapshot.\nvar %s_contract = {interface_id:", prefix);
    quoted(out, identity, strlen(identity), 0);
    fputs(", exports:[\n", out);
    for (i = 0; i < limbo_interface_count(catalog); ++i)
    {
        limbo_interface_declaration(catalog, i, &d);
        if (d.kind != LIMBO_FUNCTION)
            continue;
        fprintf(out, "%s    {declaration:%zu, module:", count++ ? ",\n" : "", i);
        if (d.scope != SIZE_MAX)
        {
            limbo_interface_declaration(catalog, d.scope, &scope);
            quoted(out, scope.name, strlen(scope.name), 0);
        }
        else
            quoted(out, "", 0, 0);
        fputs(", name:", out);
        quoted(out, d.name, strlen(d.name), 0);
        fputs(", limbo_type:", out);
        quoted(out, source + d.type_begin, d.type_end - d.type_begin, 0);
        fputc('}', out);
    }
    fputs("\n]};\n\n", out);
    if (!proxy)
    {
        for (i = 0; i < limbo_interface_count(catalog); ++i)
        {
            limbo_interface_declaration(catalog, i, &d);
            if (d.kind != LIMBO_FUNCTION)
                continue;
            fprintf(out, "// Implement .m declaration %zu; args are marshaled by the checked host.\n"
                    "function %s_call_%zu(args, request)\n{\n    throw \"unimplemented external callback: \" + ", i, prefix, i);
            quoted(out, d.name, strlen(d.name), 0);
            fputs(";\n}\n\n", out);
        }
        fprintf(out, "var %s_defaults = new Object();\n", prefix);
        for (i = 0; i < limbo_interface_count(catalog); ++i)
        {
            limbo_interface_declaration(catalog, i, &d);
            if (d.kind == LIMBO_FUNCTION)
                fprintf(out, "%s_defaults[\"d%zu\"] = %s_call_%zu;\n", prefix, i, prefix, i);
        }
    }
    as_template(out, proxy ? as_proxy_template : as_module_template, prefix);
}

/* Keep generated identifiers portable and independent of Unicode/module spelling. */
static int valid_prefix(const char *prefix)
{
    size_t i;
    if (!prefix[0] || strlen(prefix) > 64)
        return 0;
    for (i = 0; prefix[i]; ++i)
    {
        unsigned ch = (unsigned char)prefix[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' ||
              (i && ch >= '0' && ch <= '9')))
            return 0;
    }
    return 1;
}

/* Parse the complete interface before opening output; malformed input preserves
 * existing destinations. Final I/O failure may leave a prefix; streams are local. */
int main(int argc, char **argv)
{
    FILE *input = NULL, *output = NULL;
    unsigned char *data = NULL;
    size_t size;
    LimboInterface *catalog = NULL;
    LimboDiagnostic diagnostic;
    const char *prefix = argc == 7 ? argv[6] : "limbo_generated";
    int result = 2;
    if ((argc != 5 && argc != 7) || strcmp(argv[1], "--format") ||
        (argc == 7 && strcmp(argv[5], "--prefix")) || !valid_prefix(prefix) ||
        (strcmp(argv[2], "json") && strcmp(argv[2], "c") && strcmp(argv[2], "python") &&
         strcmp(argv[2], "as") && strcmp(argv[2], "as1") && strcmp(argv[2], "as2") && strcmp(argv[2], "as3") &&
         strcmp(argv[2], "as-module") && strcmp(argv[2], "as1-module") && strcmp(argv[2], "as2-module") && strcmp(argv[2], "as3-module") &&
         strcmp(argv[2], "as-proxy") && strcmp(argv[2], "as1-proxy") && strcmp(argv[2], "as2-proxy") && strcmp(argv[2], "as3-proxy")) ||
        !strcmp(argv[3], argv[4]))
    {
        fputs("usage: limbo_bindings --format json|c|python|as-module|as-proxy|as1|as2|as3 INPUT.m OUTPUT [--prefix NAME]\n", stderr);
        return 2;
    }
    input = fopen(argv[3], "rb");
    data = (unsigned char *)malloc(2u * 1024u * 1024u + 1);
    if (!input || !data)
        goto done;
    size = fread(data, 1, 2u * 1024u * 1024u + 1, input);
    if (ferror(input))
        goto done;
    result = 1;
    if (!limbo_interface_parse(data, size, &catalog, &diagnostic))
    {
        fprintf(stderr, "interface E%d byte %ld: %s\n", diagnostic.code, diagnostic.offset, diagnostic.message);
        goto done;
    }
    output = fopen(argv[4], "wb");
    if (!output)
        goto done;
    if (!strcmp(argv[2], "json"))
        manifest(output, catalog);
    else if (!strcmp(argv[2], "c"))
        c_scaffold(output, catalog, prefix);
    else if (!strcmp(argv[2], "python"))
        python_scaffold(output, catalog, prefix);
    else
        as_scaffold(output, catalog, prefix, strstr(argv[2], "proxy") != NULL);
    result = ferror(output) ? 1 : 0;
    if (fclose(output))
        result = 1;
    output = NULL;
done:
    if (input)
        fclose(input);
    if (output)
        fclose(output);
    free(data);
    limbo_interface_destroy(catalog);
    return result;
}
