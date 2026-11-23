/* Copyright (c) 2026 bhgv
 * [GC:] EN: Script file and directory operations with a common context namespace.
 * RU: Файловые операции сценария и каталоги в общем пространстве контекста.
 */
#include "../core/runtime.hpp"
#include "../core/filesystem_internal.h"
#include "file_port.h"
#include "flashvm/execution.h"
#include "flashvm/gc_roots.h"
#include "flashvm/userdata.h"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define FILE_PAGE 32U     /* Maximum script directory entries per page. */
#define FILE_PATH_BYTES 4096U /* Native path scratch, including NUL. */
#define FILE_BYTES 4096U  /* Maximum script byte array transfer per call. */
#define EXACT_INTEGER 9007199254740991.0 /* Largest uniformly exact AS integer. */

struct FileHandle
{
    FVM_PortFile *file; /* Owned by this wrapper, null after close. */
};

struct FilePage
{
    FVM_Context *context;      /* Borrowed owner through enumeration. */
    FVM_GCRootScope *roots;    /* Keeps entries alive through allocation. */
    FVM_Value rows[FILE_PAGE]; /* Staged, rooted entry Arrays. */
    unsigned count;            /* Number of staged rows. */
};


/* EN: Reject empty, counted-NUL and mistyped path or mode strings.
 * RU: Отклонить пустые строки, встроенные NUL и неверный тип пути либо режима. */
static bool text(const FVM_Value &value, char *bytes, size_t capacity)
{
    const size_t size = fvm_value_string_size(&value);
    if (value.type != FVM_VALUE_STRING || !value.string || !size || size >= capacity ||
        std::memchr(value.string, 0, size))
    {
        return false;
    }
    std::memcpy(bytes, value.string, size);
    bytes[size] = 0;
    return true;
}


/* EN: Convert only an exact finite number within native and script bounds.
 * RU: Преобразовать только конечное точное число в пределах нативного типа и сценария. */
static bool integer(const FVM_Value &value, long minimum, long maximum, long *number)
{
    if (value.type != FVM_VALUE_NUMBER || !(value.number >= minimum && value.number <= maximum) ||
        !(value.number >= -EXACT_INTEGER && value.number <= EXACT_INTEGER))
    {
        return false;
    }
    *number = static_cast<long>(value.number);
    return static_cast<double>(*number) == value.number;
}


/* [GC:] EN: Close the native handle and release its shell at object reclamation.
 * RU: Закрыть нативный дескриптор и освободить оболочку при сборке объекта. */
static void release_file(void *state)
{
    FileHandle *handle = static_cast<FileHandle *>(state);
    fvm_port_file_close(handle->file);
    std::free(handle);
}


/* EN: Borrow a handle of this exact native class in the owner context.
 * RU: Заимствовать дескриптор точного нативного класса у владельца. */
static FileHandle *handle_of(FVM_Context *context, FVM_Object *object)
{
    return object && object->owner == context && object->resource_kind == FVM_RESOURCE_USERDATA &&
        object->release_resource == release_file ? static_cast<FileHandle *>(object->resource) : 0;
}


/* [GC:] EN: Read up to the requested bytes and return a dense byte Array at EOF or success.
 * RU: Прочитать до заданного числа байтов и вернуть плотный Array, включая пустой на EOF. */
static FVM_Result read_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    FileHandle *handle = handle_of(context, self);
    long count;
    if (!handle || !handle->file || argc != 1 || !integer(args[0], 0, FILE_BYTES, &count))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value *items = count ? static_cast<FVM_Value *>(std::malloc(count * sizeof(*items))) : 0;
    if (count && !items)
    {
        return FVM_OUT_OF_MEMORY;
    }
    unsigned char bytes[FILE_BYTES];
    size_t used;
    FVM_Result status = fvm_port_file_read(handle->file, bytes, static_cast<size_t>(count), &used);
    if (status == FVM_OK)
    {
        for (size_t i = 0; i < used; ++i)
        {
            fvm_value_number(items + i, bytes[i]);
        }
        status = fvm_exec_literal_array(context, static_cast<unsigned>(used), items, out);
    }
    std::free(items);
    return status;
}


/* EN: Stage exact own byte values before opening any write; check for reentrant close.
 * RU: Снять точные собственные байты до записи и проверить закрытие из геттера. */
static FVM_Result write_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    FileHandle *handle = handle_of(context, self);
    FVM_Value item;
    long count;
    if (!handle || !handle->file || argc != 1 || args[0].type != FVM_VALUE_OBJECT)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *array = args[0].object;
    FVM_Result status = fvm_object_get(context, array, "length", &item);
    if (status != FVM_OK || !integer(item, 0, FILE_BYTES, &count))
    {
        return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
    }
    unsigned char bytes[FILE_BYTES];
    for (long i = 0; i < count; ++i)
    {
        char name[sizeof("4096")];
        int own;
        long byte;
        std::sprintf(name, "%ld", i);
        status = fvm_object_has_own(context, array, name, &own);
        if (status != FVM_OK || !own)
        {
            return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
        }
        status = fvm_object_get(context, array, name, &item);
        if (status != FVM_OK || !integer(item, 0, UCHAR_MAX, &byte))
        {
            return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
        }
        bytes[i] = static_cast<unsigned char>(byte);
    }
    status = handle->file ? fvm_port_file_write(handle->file, bytes, static_cast<size_t>(count)) : FVM_ERROR;
    if (status == FVM_OK)
    {
        fvm_value_number(out, count);
    }
    return status;
}


/* EN: Seek on one logical file cursor and return its byte position.
 * RU: Переместить курсор файла и вернуть его позицию в байтах. */
static FVM_Result seek_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    FileHandle *handle = handle_of(context, self);
    long offset, origin = FVM_FILE_SEEK_START, position;
    if (!handle || !handle->file || (argc != 1 && argc != 2) || !integer(args[0], LONG_MIN, LONG_MAX, &offset) ||
        (argc == 2 && !integer(args[1], FVM_FILE_SEEK_START, FVM_FILE_SEEK_END, &origin)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    const FVM_Result status = fvm_port_file_seek(handle->file, offset, static_cast<int>(origin), &position);
    if (status == FVM_OK)
    {
        fvm_value_number(out, static_cast<double>(position));
    }
    return status;
}


/* EN: Consume the open descriptor exactly once, keeping the shell usable for repeated close.
 * RU: Освободить открытый дескриптор ровно один раз, сохранив оболочку для повторного close. */
static FVM_Result close_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *, FVM_Value *out, void *)
{
    FileHandle *handle = handle_of(context, self);
    if (!handle || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_PortFile *file = handle->file;
    handle->file = 0;
    fvm_port_file_close(file);
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN: Root one new native file shell before installing its methods and opening the path.
 * RU: Удержать оболочку файла до установки методов и открытия пути. */
static FVM_Result open_file(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    char path[FILE_PATH_BYTES], mode[sizeof("rwo")] = "r";
    if ((argc != 1 && argc != 2) || !text(args[0], path, sizeof(path)) ||
        (argc == 2 && !text(args[1], mode, sizeof(mode))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    status = fvm_object_create(context, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &value);
    }
    const char *names[] = {"read", "write", "seek", "close"};
    const FVM_NativeFunction callbacks[] = {read_file, write_file, seek_file, close_file};
    for (unsigned i = 0; status == FVM_OK && i < sizeof(names) / sizeof(*names); ++i)
    {
        status = fvm_object_define_method_rooted(context, value.object, names[i], callbacks[i], 0, 0);
    }
    if (status == FVM_OK)
    {
        FileHandle *handle = static_cast<FileHandle *>(std::calloc(1, sizeof(*handle)));
        status = handle ? fvm_userdata_attach(context, value.object, handle, release_file) : FVM_OUT_OF_MEMORY;
        if (status == FVM_OK)
        {
            status = fvm_filesystem_open(context, path, mode, &handle->file);
        }
        else
        {
            std::free(handle);
        }
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN: Materialize the common stat tuple, optionally preceded by a directory name.
 * RU: Создать общий кортеж stat, при необходимости с именем записи каталога. */
static FVM_Result info_array(FVM_Context *context, const FVM_FsInfo &info, const char *name, FVM_Value *out)
{
    FVM_Value fields[5];
    FVM_Value *data = name ? fields + 1 : fields;
    if (name && fvm_value_string(fields, name) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(data, info.length_high);
    fvm_value_number(data + 1, info.length_low);
    fvm_value_number(data + 2, info.mode);
    fvm_value_boolean(data + 3, info.directory != 0);
    return fvm_exec_literal_array(context, name ? 5 : 4, fields, out);
}


/* [GC:] EN: Stat the path and return the same metadata fields from either backend.
 * RU: Выполнить stat пути и вернуть одинаковые поля из любого backend. */
static FVM_Result stat_path(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    char path[FILE_PATH_BYTES];
    FVM_FsInfo info;
    const FVM_Result status = argc == 1 && text(args[0], path, sizeof(path)) ?
        fvm_filesystem_stat(context, path, &info) : FVM_INVALID_ARGUMENT;
    return status == FVM_OK ? info_array(context, info, 0, out) : status;
}


/* [GC:] EN: Copy one borrowed entry and root it before the next native callback.
 * RU: Скопировать заимствованную запись и удержать её до следующего колбэка. */
static FVM_Result visit_entry(void *state, const char *name, const FVM_FsInfo *info)
{
    FilePage *page = static_cast<FilePage *>(state);
    if (page->count == FILE_PAGE)
    {
        return FVM_ERROR;
    }
    FVM_Value *entry = page->rows + page->count;
    FVM_Result status = info_array(page->context, *info, name, entry);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(page->roots, entry);
    }
    if (status == FVM_OK)
    {
        ++page->count;
    }
    return status;
}


/* [GC:] EN: Read a page after a zero-based skip and publish it only on success.
 * RU: Прочитать страницу после пропуска записей и выдать её только при успехе. */
static FVM_Result list_path(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    char path[FILE_PATH_BYTES];
    long skip = 0, limit = FILE_PAGE;
    if ((argc != 1 && argc != 3) || !text(args[0], path, sizeof(path)) ||
        (argc == 3 && (!integer(args[1], 0, LONG_MAX, &skip) || !integer(args[2], 1, FILE_PAGE, &limit))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FilePage page;
    page.context = context;
    page.count = 0;
    FVM_Result status = fvm_gc_roots_begin(context, &page.roots);
    if (status == FVM_OK)
    {
        status = fvm_filesystem_list(context, path, static_cast<size_t>(skip), static_cast<size_t>(limit), visit_entry, &page);
        if (status == FVM_OK)
        {
            status = fvm_exec_literal_array(context, page.count, page.rows, out);
        }
        fvm_gc_roots_end(page.roots);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/filesystem.h. / См. flashvm/filesystem.h. */
FVM_Result fvm_filesystem_install(FVM_Context *context, const char *global)
{
    FVM_Value value;
    int own;
    if (!context || context->destroying || !global || !*global ||
        fvm_object_has_own(context, context->global, global, &own) != FVM_OK || own ||
        fvm_get_global(context, global, &value) != FVM_OK || value.type != FVM_VALUE_UNDEFINED)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&value);
    status = fvm_object_create(context, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &value);
    }
    const char *names[] = {"open", "stat", "list"};
    const FVM_NativeFunction callbacks[] = {open_file, stat_path, list_path};
    for (unsigned i = 0; status == FVM_OK && i < sizeof(names) / sizeof(*names); ++i)
    {
        status = fvm_object_define_method_rooted(context, value.object, names[i], callbacks[i], 0, 0);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, global, &value);
    }
    fvm_gc_roots_end(roots);
    return status;
}
