/* Copyright (c) 2026 bhgv
 * EN: Describe resolved export and import types using the compiler linkage order.
 * RU: Описание типов экспортов и импортов в порядке таблицы связей компилятора.
 */
#include "limbo.h"
#include "styx_export.h"

#define SHAPE_MAX 48U /* Shape bytes excluding NUL, consistent with hosted Dis. */
#define TABLE_MAX 65536U /* Limit for each section, including its final marker. */
#define DEPTH_MAX 6U /* Per-value bound on nested compiler types. */


typedef struct Shape
{
    char text[SHAPE_MAX]; /* Current shape bytes, counted without NUL. */
    size_t size; /* Bytes written into text. */
} Shape;


/* EN: Append ch to shape and return zero on exhaustion.
 * RU: Добавить ch к shape и вернуть ноль при исчерпании места. */
static int token(Shape *shape, char ch)
{
    if (shape->size == sizeof(shape->text))
    {
        return 0;
    }
    shape->text[shape->size++] = ch;
    return 1;
}


/* EN: Serialize a supported resolved type with bounded recursion.
 * RU: Записать поддерживаемый разрешённый тип с ограниченной вложенностью. */
static int shape_type(Shape *shape, const Type *type, unsigned depth)
{
    if (!type || depth == DEPTH_MAX)
    {
        return 0;
    }
    switch (type->kind)
    {
    case Tint:
        return token(shape, 'i');
    case Tbig:
        return token(shape, 'b');
    case Tbyte:
        return token(shape, 'y');
    case Treal:
        return token(shape, 'f');
    case Tstring:
        /* Keep ordinary images readable by loaders with the scalar grammar. */
        return debug['G'] && token(shape, 's');
    case Tarray:
        return debug['G'] && token(shape, 'a') && token(shape, '[') &&
            shape_type(shape, type->tof, depth + 1) && token(shape, ']');
    case Ttuple:
        if (!token(shape, '('))
        {
            return 0;
        }
        for (const Decl *field = type->ids; field; field = field->next)
        {
            if (!shape_type(shape, field->ty, depth + 1))
            {
                return 0;
            }
        }
        return token(shape, ')');
    case Tref:
        type = type->tof;
        if (!type || type->kind != Tadt || type->tags || type->polys ||
            !token(shape, 'r') || !token(shape, '['))
        {
            return 0;
        }
        for (const Decl *field = type->ids; field; field = field->next)
        {
            if (field->store == Dfield &&
                ((!debug['G'] && field->ty->kind != Tint && field->ty->kind != Tbig) ||
                    !shape_type(shape, field->ty, depth + 1)))
            {
                return 0;
            }
        }
        return token(shape, ']');
    default:
        return 0;
    }
}


/* EN: Fill args and result from a checked function, or return zero for omission.
 * RU: Заполнить args и result из проверенной функции либо вернуть ноль. */
static int shape_function(const Type *type, Shape *args, Shape *result)
{
    if (type->kind != Tfn || type->varargs || type->polys || !token(args, '(') || !token(result, '('))
    {
        return 0;
    }
    for (const Decl *arg = type->ids; arg; arg = arg->next)
    {
        if (arg->implicit || !shape_type(args, arg->ty, 0))
        {
            return 0;
        }
    }
    return token(args, ')') &&
        (type->tof->kind == Tnone || shape_type(result, type->tof, 0)) && token(result, ')');
}


/* EN: Write the shared signature and counted type fields of one record.
 * RU: Записать общую сигнатуру и типы с длинами в одной записи. */
static void write_shape(Decl *entry, const Shape *args, const Shape *result)
{
    disword(sign(entry));
    if (Bputc(bout, args->size) < 0 || Bputc(bout, result->size) < 0 ||
        Bwrite(bout, args->text, args->size) != args->size ||
        Bwrite(bout, result->text, result->size) != result->size)
    {
        fatal("cannot write compiled Styx types");
    }
}


/* EN: Write the resolved exported types in the exact dismod order.
 * RU: Записать разрешённые типы экспортов в порядке dismod. */
static void write_exports(Decl *module)
{
    unsigned long index = 0;
    size_t bytes = 4; /* Reserve the final record terminator. */
    for (Decl *entry = module->ty->tof->ids; entry; entry = entry->next, index++)
    {
        Shape args = {{0}, 0}, result = {{0}, 0};
        if (entry->store != Dfn || !shape_function(entry->ty, &args, &result))
        {
            continue;
        }
        size_t record = 10 + args.size + result.size;
        if (index >= 0xffffffffUL || record > TABLE_MAX - bytes)
        {
            break;
        }
        bytes += record;
        disword(index);
        write_shape(entry, &args, &result);
    }
    disword(0xffffffffUL);
}


/* EN: Follow disldt exactly, including the positions of omitted entries.
 * RU: Следовать порядку disldt, учитывая позиции пропущенных записей. */
static void write_imports(Decl *ldts)
{
    size_t bytes = 4; /* Reserve the final group terminator. */
    unsigned long group = 0;
    for (Decl *table = ldts; table; table = table->next)
    {
        if (table->store != Dglobal || !table->init)
        {
            continue;
        }
        unsigned long index = 0;
        for (Decl *entry = table->init->decl->ty->ids; entry; entry = entry->next, index++)
        {
            Shape args = {{0}, 0}, result = {{0}, 0};
            if (!shape_function(entry->ty, &args, &result))
            {
                continue;
            }
            size_t record = 14 + args.size + result.size;
            if (group >= 0xffffffffUL || index >= 0xffffffffUL || record > TABLE_MAX - bytes)
            {
                disword(0xffffffffUL);
                return;
            }
            bytes += record;
            disword(group);
            disword(index);
            write_shape(entry, &args, &result);
        }
        group++;
    }
    disword(0xffffffffUL);
}


/* EN/RU: See styx_export.h. / См. styx_export.h. */
void limbo_styx_export(Decl *module, Decl *ldts)
{
    if (IBY2WD != 8)
    {
        return;
    }
    if (Bwrite(bout, ldts ? "STYXCS2!" : "STYXCS1!", 8) != 8)
    {
        fatal("cannot write Styx type table");
    }
    write_exports(module);
    if (ldts)
    {
        write_imports(ldts);
    }
}
