/* Copyright (c) 2026 bhgv
 * EN: Original Styx file callbacks for a callback-backed array.
 * RU: Файловые обработчики исходного Styx для массива с обратными вызовами.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "indexed_styx.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Publish the array under parent using five reserved consecutive qids.
 * Опубликовать массив под parent, используя пять зарезервированных qid подряд. */
int styxarrayfilepublish(StyxArrayFiles *files, Styxserver *server, Path parent, Path base,
    const char *name, const char *owner, StyxIntArray *array)
{
    if (!files || !server || !base || base > UINT64_MAX - 5 || !name || !*name || strchr(name, '/') ||
        !strcmp(name, ".") || !strcmp(name, "..") || !owner || !*owner || !array || !array->load)
    {
        return 0;
    }
    for (Path qid = base; qid <= base + 4; qid++)
    {
        if (styxfindfile(server, qid))
        {
            return 0;
        }
    }
    if (!styxadddir(server, parent, base, (char *)name, 0555, (char *)owner) ||
        !styxaddfile(server, base, base + 1, ".idx", 0222, (char *)owner) ||
        !styxaddfile(server, base, base + 2, ".val", array->store ? 0666 : 0444, (char *)owner) ||
        !styxaddfile(server, base, base + 3, ".len", 0444, (char *)owner) ||
        !styxaddfile(server, base, base + 4, ".type", 0444, (char *)owner))
    {
        return 0;
    }
    *files = (StyxArrayFiles){server, base, array};
    return 1;
}


typedef struct Opened
{
    uint64_t generation; /* Selection held by this fid. */
    StyxIntArray *array; /* Borrowed array through fid retirement. */
    int selector; /* Closing .idx retires the selection. */
} Opened;


/* EN/RU: Open a selector or value on the connection that owns the selection.
 * Открыть выбор или значение в соединении, которому принадлежит выбор. */
char *styxarrayfileopen(StyxArrayFiles *files, Qid *qid, int mode)
{
    if (qid->path == files->base || qid->path == files->base + 3 || qid->path == files->base + 4)
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (qid->path != files->base + 1 && qid->path != files->base + 2)
    {
        return Ebadarg;
    }
    int selector = qid->path == files->base + 1;
    if ((selector && mode != OWRITE) || (!selector && mode != OREAD &&
        (!files->array->store || (mode != OWRITE && mode != ORDWR))))
    {
        return Eperm;
    }
    Opened *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    fid->selector = selector;
    fid->array = files->array;
    fid->generation = styxintarrayopen(files->array, styxclient(files->server), selector);
    if (!fid->generation || styxsetaux(files->server, fid) < 0)
    {
        if (selector)
        {
            styxintarrayclose(fid->array, fid->generation);
        }
        free(fid);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Retire selectors on clunk or disconnect; consume the fid context.
 * Отозвать выбор при clunk или отключении и освободить контекст fid. */
void styxarrayfileclose(void *state)
{
    Opened *fid = state;
    if (fid->selector)
    {
        styxintarrayclose(fid->array, fid->generation);
    }
    free(fid);
}


/* EN/RU: Read the element selected by this value fid.
 * Прочитать элемент, выбранный этим fid значения. */
char *styxarrayfileread(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset)
{
    if ((qid.path == files->base + 3 || qid.path == files->base + 4) && offset >= 0)
    {
        if (qid.path == files->base + 4)
        {
            *count = styxreadstr(offset, bytes, *count, "array of int\n");
        }
        else
        {
            char length[16];
            snprintf(length, sizeof(length), "%" PRIu32 "\n", files->array->length);
            *count = styxreadstr(offset, bytes, *count, length);
        }
        return nil;
    }
    Opened *fid = styxgetaux(files->server);
    size_t size = *count;
    if (qid.path != files->base + 2 || !fid || offset < 0 ||
        !styxintarrayread(fid->array, fid->generation, offset, bytes, &size))
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN/RU: Write one index or update the selected value according to the opened file.
 * Записать индекс или заменить выбранное значение в зависимости от открытого файла. */
char *styxarrayfilewrite(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset)
{
    Opened *fid = styxgetaux(files->server);
    if (!fid || offset < 0 || (qid.path != files->base + 1 && qid.path != files->base + 2))
    {
        return Ebadarg;
    }
    int accepted = qid.path == files->base + 1 ? styxintarrayselect(fid->array, fid->generation, offset, bytes, *count) :
        styxintarraywrite(fid->array, fid->generation, offset, bytes, *count);
    return accepted ? nil : Ebadarg;
}
