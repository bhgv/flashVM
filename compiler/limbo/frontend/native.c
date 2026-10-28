/* Explicit native scalar ABI emission from authoritative Coco/R catalog nodes. */
#include "native.h"
#include "native_md5.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/* One parsed scalar, with target layout independent of the generator's own pointer width. */
typedef struct
{
    const char *c;  /* Static native C spelling. */
    char tag;       /* Inferno structural type byte. */
    unsigned size;  /* Selected target payload bytes. */
    unsigned align; /* Required target alignment. */
    int pointer;    /* [GC:] One managed pointer slot, for string only. */
} NativeType;

/* Validated parameter owns its C-safe name and exact frame offset. */
typedef struct
{
    NativeType type; /* Resolved direct primitive, never an unresolved alias. */
    char name[64];   /* Bounded copied source name, or generated nil placeholder. */
    unsigned offset; /* Byte offset in the selected frame. */
} NativeParameter;

/* All metadata is validated before output starts; these owners contain no VM pointers. */
typedef struct
{
    char name[64];                  /* Copied export identifier. */
    NativeType result;              /* Void uses tag n and no pointer result. */
    NativeParameter parameters[16]; /* Bounded scalar profile. */
    unsigned count;                 /* Number of initialized parameters. */
    unsigned size;                  /* Complete frame rounded to eight bytes. */
    unsigned map_size;              /* [GC:] Bytes through the last nonzero pointer-map byte. */
    unsigned char map[16];          /* [GC:] Bit order matches Dis descriptor slots. */
    uint32_t signature;             /* Folded structural MD5, not the scaffold FNV snapshot tag. */
} NativeFunction;

/* Copy a useful semantic diagnostic into caller-owned storage and return failure. */
static int fail(char *error, size_t capacity, const char *format, ...)
{
    va_list arguments;
    if (error && capacity)
    {
        va_start(arguments, format);
        vsnprintf(error, capacity, format, arguments);
        va_end(arguments);
    }
    return 0;
}

/* Borrow source/length and copy a C identifier into caller-owned destination[64].
 * Return 1 on success or 0 without changing destination; no locale, allocation or truncation. */
static int identifier(char destination[64], const char *source, size_t length)
{
    size_t index;
    if (!length || length >= 64)
    {
        return 0;
    }
    for (index = 0; index < length; index++)
    {
        unsigned ch = (unsigned char)source[index];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (index && ch >= '0' && ch <= '9')))
        {
            return 0;
        }
    }
    memcpy(destination, source, length);
    destination[length] = 0;
    return 1;
}

/* Borrow catalog/node and resolve a direct primitive for the explicit cell target.
 * Return 1 and fill caller-owned out, or 0 leaving it unchanged. No allocation or VM access. */
static int primitive(const LimboInterface *catalog, size_t node, unsigned cell, NativeType *out)
{
    const char *kind = limbo_interface_type_kind(catalog, node);
    NativeType type = {"void", 'n', 0, 1, 0};
    if (node == SIZE_MAX)
    {
        *out = type;
        return 1;
    }
    if (!kind)
    {
        return 0;
    }
    if (!strcmp(kind, "byte"))
    {
        type = (NativeType){"BYTE", 'b', 1, 1, 0};
    }
    else if (!strcmp(kind, "int"))
    {
        type = (NativeType){"WORD", 'i', cell, cell, 0};
    }
    else if (!strcmp(kind, "big"))
    {
        type = (NativeType){"LONG", 'B', 8, 8, 0};
    }
    else if (!strcmp(kind, "real"))
    {
        type = (NativeType){"REAL", 'r', 8, 8, 0};
    }
    else if (!strcmp(kind, "string"))
    {
        type = (NativeType){"String *", 's', cell, cell, 1};
    }
    else
    {
        return 0;
    }
    *out = type;
    return 1;
}

/* Borrow two validated NativeFunction records for qsort and return their name ordering.
 * Neither record is mutated here; no allocation or parser/VM state is accessed. */
static int compare(const void *left, const void *right)
{
    return strcmp(((const NativeFunction *)left)->name, ((const NativeFunction *)right)->name);
}

/* Borrow a terminated, already length-checked parameter name; return 1 allowed/0 reserved.
 * Prevent C/frame-field collisions without renaming ABI inputs or retaining caller storage. */
static int parameter_name(const char *name)
{
    static const char *const reserved[] = {
        "auto",       "break",    "case",     "char",     "const", "continue", "default", "do",     "double",
        "else",       "enum",     "extern",   "float",    "for",   "goto",     "if",      "inline", "int",
        "long",       "register", "restrict", "return",   "short", "signed",   "sizeof",  "static", "struct",
        "switch",     "typedef",  "union",    "unsigned", "void",  "volatile", "while",   "_Bool",  "_Complex",
        "_Imaginary", "regs",     "ret",      "noret",    "temps"};
    size_t index;
    if (!strncmp(name, "_native_", 8))
    {
        return 0;
    }
    for (index = 0; index < sizeof(reserved) / sizeof(reserved[0]); index++)
    {
        if (!strcmp(name, reserved[index]))
        {
            return 0;
        }
    }
    return 1;
}

/* Compare two owned, terminated C spellings for qsort; neither input is changed. */
static int compare_symbol(const void *left, const void *right)
{
    return strcmp((const char *)left, (const char *)right);
}

/* Check every ordinary C identifier emitted for a module before publication.
 * module/functions are borrowed validated names; error/capacity are optional output.
 * Return 1 on success, 0 for allocation failure or a callback/typedef/global collision.
 * Temporary names are freed here; no parser globals, streams or VM state are touched. */
static int
validate_symbols(const char *module, const NativeFunction *functions, unsigned count, char *error, size_t capacity)
{
    char (*names)[160] = calloc(4 + count * 19, sizeof(*names));
    size_t used = 0;
    size_t index;
    int valid = 1;
    if (!names)
    {
        return fail(error, capacity, "native symbol-check allocation failure");
    }
    snprintf(names[used++], sizeof(*names), "%s_native_abi", module);
    snprintf(names[used++], sizeof(*names), "%s_NativeExport", module);
    snprintf(names[used++], sizeof(*names), "%smodtab", module);
    snprintf(names[used++], sizeof(*names), "%smodlen", module);
    for (index = 0; index < count; index++)
    {
        const NativeFunction *function = &functions[index];
        unsigned parameter;
        snprintf(names[used++], sizeof(*names), "%s_%s", module, function->name);
        snprintf(names[used++], sizeof(*names), "F_%s_%s", module, function->name);
        snprintf(names[used++], sizeof(*names), "%s_%s_frame_size", module, function->name);
        for (parameter = 0; parameter < function->count; parameter++)
        {
            snprintf(names[used++], sizeof(*names), "%s_%s_offset_%u", module, function->name, parameter);
        }
    }
    qsort(names, used, sizeof(*names), compare_symbol);
    for (index = 1; index < used; index++)
    {
        if (!strcmp(names[index - 1], names[index]))
        {
            valid = fail(error, capacity, "generated native C symbol collision: %s", names[index]);
            break;
        }
    }
    free(names);
    return valid;
}

/* Emit validated metadata and compile-time C layout checks; stream/catalog remain caller-owned. */
int limbo_native_header(FILE *output, const LimboInterface *catalog, unsigned cell, char *error, size_t capacity)
{
    const LimboTree *tree = limbo_interface_tree(catalog);
    const char *source;
    NativeFunction *functions;
    size_t module = SIZE_MAX;
    char module_name[64];
    size_t index;
    unsigned count = 0;
    int success = 0;
    if (error && capacity)
    {
        error[0] = 0;
    }
    if (!catalog || (cell != 4 && cell != 8))
    {
        return fail(error, capacity, "native target must select classic32 or NC64");
    }
    source = limbo_tree_source(tree);
    functions = calloc(128, sizeof(*functions));
    if (!functions)
    {
        return fail(error, capacity, "native interface allocation failure");
    }
    for (index = 0; index < limbo_interface_count(catalog); index++)
    {
        LimboDeclaration declaration;
        limbo_interface_declaration(catalog, index, &declaration);
        if (declaration.kind == LIMBO_MODULE)
        {
            if (module != SIZE_MAX || declaration.scope != SIZE_MAX ||
                !identifier(module_name, declaration.name, strlen(declaration.name)))
            {
                fail(error, capacity, "native profile requires exactly one C-named top-level module");
                goto done;
            }
            module = index;
        }
    }
    if (module == SIZE_MAX)
    {
        fail(error, capacity, "native interface has no module");
        goto done;
    }
    for (index = 0; index < limbo_interface_count(catalog); index++)
    {
        LimboDeclaration declaration;
        NativeFunction *function;
        LimboSignature signature;
        unsigned position = 8 * cell;
        unsigned parameter;
        unsigned char spelling[256];
        size_t used = 0;
        limbo_interface_declaration(catalog, index, &declaration);
        if (index == module || (declaration.kind == LIMBO_CONSTANT && declaration.scope == module))
        {
            continue;
        }
        if (declaration.kind != LIMBO_FUNCTION || declaration.scope != module || count == 128 ||
            !limbo_interface_signature(catalog, index, &signature) || signature.variadic || signature.polymorphic ||
            signature.parameter_count > 16)
        {
            fail(error, capacity, "unsupported native declaration/profile at %s", declaration.name);
            goto done;
        }
        {
            LimboNode node;
            size_t at;
            for (at = signature.type_node; limbo_tree_node(tree, at, &node); at++)
            {
                if (at != signature.type_node && node.begin >= declaration.type_end)
                {
                    break;
                }
                if (!strcmp(node.kind, "Raises") && node.end > node.begin)
                {
                    fail(error, capacity, "raises-qualified native signatures are not supported");
                    goto done;
                }
            }
        }
        function = &functions[count++];
        if (!identifier(function->name, declaration.name, strlen(declaration.name)) ||
            !primitive(catalog, signature.result_node, cell, &function->result))
        {
            fail(error, capacity, "unsupported native result/name at %s", declaration.name);
            goto done;
        }
        function->count = (unsigned)signature.parameter_count;
        spelling[used++] = 'f';
        spelling[used++] = '(';
        for (parameter = 0; parameter < function->count; parameter++)
        {
            LimboParameter item;
            NativeParameter *p = &function->parameters[parameter];
            unsigned previous;
            if (!limbo_interface_parameter(catalog, index, parameter, &item) || item.self_parameter ||
                !primitive(catalog, item.type_node, cell, &p->type) || p->type.tag == 'n')
            {
                fail(error, capacity, "unsupported native parameter at %s", declaration.name);
                goto done;
            }
            if (item.name_end - item.name_begin == 3 && !memcmp(source + item.name_begin, "nil", 3))
            {
                snprintf(p->name, sizeof(p->name), "arg%u", parameter);
            }
            else if (!identifier(p->name, source + item.name_begin, item.name_end - item.name_begin))
            {
                fail(error, capacity, "non-C native parameter name");
                goto done;
            }
            if (!parameter_name(p->name))
            {
                fail(error, capacity, "reserved C/frame parameter %s", p->name);
                goto done;
            }
            for (previous = 0; previous < parameter; previous++)
            {
                if (!strcmp(p->name, function->parameters[previous].name))
                {
                    fail(error, capacity, "duplicate generated parameter name");
                    goto done;
                }
            }
            position = (position + p->type.align - 1) / p->type.align * p->type.align;
            p->offset = position;
            if (p->type.pointer)
            {
                unsigned slot = position / cell;
                function->map[slot / 8] |= (unsigned char)(0x80u >> (slot % 8));
                function->map_size = slot / 8 + 1;
            }
            position += p->type.size;
            if (parameter)
            {
                spelling[used++] = ',';
            }
            spelling[used++] = (unsigned char)p->type.tag;
        }
        spelling[used++] = ')';
        spelling[used++] = (unsigned char)function->result.tag;
        function->signature = native_signature(spelling, used);
        function->size = (position + 7) / 8 * 8;
    }
    if (!count)
    {
        fail(error, capacity, "native module has no exports");
        goto done;
    }
    qsort(functions, count, sizeof(*functions), compare);
    if (!validate_symbols(module_name, functions, count, error, capacity))
    {
        goto done;
    }
    if (!output)
    {
        success = 1;
        goto done;
    }
    fprintf(output,
            "/* Generated by our Coco/R Limbo compiler; %s target, no hand-maintained offsets. */\n",
            cell == 8 ? "NC64" : "classic32");
    fprintf(output,
            "#ifndef %s_NATIVE_GENERATED_H\n#define %s_NATIVE_GENERATED_H\n#include <stddef.h>\n",
            module_name,
            module_name);
    fprintf(
        output,
        "typedef char %s_native_abi[(NREG == 5 && sizeof(WORD) == %u && sizeof(void *) == %u && sizeof(LONG) == 8 && sizeof(REAL) == 8) ? 1 : -1];\n",
        module_name,
        cell,
        cell);
    for (index = 0; index < count; index++)
    {
        NativeFunction *function = &functions[index];
        unsigned parameter;
        unsigned position = 8 * cell;
        fprintf(
            output,
            "\n/* [GC:] Borrowed owner-thread frame; arguments are rooted by the generated map. */\nvoid %s_%s(void *frame);\ntypedef struct F_%s_%s F_%s_%s;\nstruct F_%s_%s\n{\n    WORD regs[NREG-1]; /* VM frame prefix. */\n",
            module_name,
            function->name,
            module_name,
            function->name,
            module_name,
            function->name,
            module_name,
            function->name);
        if (function->result.tag == 'n')
        {
            fputs("    WORD noret; /* Void result has no return pointer. */\n", output);
        }
        else
        {
            fprintf(output, "    %s *ret; /* Caller-owned result slot. */\n", function->result.c);
        }
        fprintf(output, "    uchar temps[%u]; /* Three target-cell scratch slots. */\n", 3 * cell);
        for (parameter = 0; parameter < function->count; parameter++)
        {
            NativeParameter *p = &function->parameters[parameter];
            if (p->offset > position)
            {
                fprintf(output,
                        "    uchar _native_pad_%u[%u]; /* Explicit ABI alignment. */\n",
                        parameter,
                        p->offset - position);
            }
            fprintf(output, "    %s %s; /* Borrowed argument at byte %u. */\n", p->type.c, p->name, p->offset);
            position = p->offset + p->type.size;
        }
        if (function->size > position)
        {
            fprintf(
                output, "    uchar _native_tail[%u]; /* Complete descriptor extent. */\n", function->size - position);
        }
        fputs("};\n", output);
        fprintf(output,
                "typedef char %s_%s_frame_size[(sizeof(F_%s_%s) == %u) ? 1 : -1];\n",
                module_name,
                function->name,
                module_name,
                function->name,
                function->size);
        for (parameter = 0; parameter < function->count; parameter++)
        {
            fprintf(output,
                    "typedef char %s_%s_offset_%u[(offsetof(F_%s_%s, %s) == %u) ? 1 : -1];\n",
                    module_name,
                    function->name,
                    parameter,
                    module_name,
                    function->name,
                    function->parameters[parameter].name,
                    function->parameters[parameter].offset);
        }
    }
    fprintf(
        output,
        "\n/* Registration record matches builtinmod's native ABI, with a private C type name. */\ntypedef struct { char *name; long sig; void (*fn)(void *); int size; int np; uchar map[16]; } %s_NativeExport;\n",
        module_name);
    fprintf(output, "%s_NativeExport %smodtab[] = {\n", module_name, module_name);
    for (index = 0; index < count; index++)
    {
        NativeFunction *function = &functions[index];
        unsigned byte;
        fprintf(output,
                "    {\"%s\", 0x%08lxUL, %s_%s, %u, %u, {",
                function->name,
                (unsigned long)function->signature,
                module_name,
                function->name,
                function->size,
                function->map_size);
        for (byte = 0; byte < (function->map_size ? function->map_size : 1); byte++)
        {
            fprintf(output, "%s0x%02x", byte ? ", " : "", function->map[byte]);
        }
        fputs("}},\n", output);
    }
    fprintf(output, "    {0, 0, 0, 0, 0, {0}}\n};\n#define %smodlen %u\n#endif\n", module_name, count);
    success = !ferror(output);
    if (!success)
    {
        fail(error, capacity, "native interface output failure");
    }
done:
    free(functions);
    return success;
}
