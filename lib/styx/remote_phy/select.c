/* Copyright (c) 2026 bhgv
 * EN: Explicit host-supplied physical-driver selection, with no global registry or implicit fallback.
 * RU: Явный выбор physical driver из host table без global registry и implicit fallback.
 */
#include "phy.h"
#include <string.h>

/* EN/RU: See phy.h; reject ambiguous names before invoking any factory.
 * См. phy.h; ambiguous names запрещены до вызова factory. */


/* [standalone-contract:physelect]
 * EN: physelect: remote_phy/select implementation and native state.
 * RU: physelect: реализация remote_phy/select и нативное состояние.
 * drivers (const PhyDriver *drivers): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * count (size_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * name (const char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * parameters (const void *parameters): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: RemotePhy *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: RemotePhy *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name, const void *parameters, size_t size)
{
    if ((!drivers && count) || !name || !*name || (!parameters && size))
    {
        return NULL;
    }
    const PhyDriver *found = NULL;
    for (size_t i = 0; i < count; i++)
    {
        if (drivers[i].name && !strcmp(drivers[i].name, name))
        {
            if (found || !drivers[i].open)
            {
                return NULL;
            }
            found = drivers + i;
        }
    }
    return found ? found->open(parameters, size) : NULL;
}
