/* Copyright (c) 2026 bhgv
 * EN: Private admission, branch relocation and SWO serialization for the AVM1 emitter.
 * RU: Внутренняя проверка, замыкание переходов и запись SWO для генератора AVM1.
 */
#ifndef FIR_OBJECT_FORMAT_H
#define FIR_OBJECT_FORMAT_H
#include "fir.h"

#define FIR_OBJECT_LIMIT (2u * 1024u * 1024u) /* Bound a complete binary SWO record. */


/* EN: One unresolved AVM1 branch / RU: Незамкнутый переход AVM1. */
typedef struct
{
    size_t at;  /* First byte of the little-endian signed displacement. */
    int target; /* Block index, or -1 for the current action block's End. */
} FirAvm1Fixup;


/* EN/RU: Validate target requirements / Проверить ограничения целевого формата. */
int fir_object_admit(FirModule *module, char *error, size_t capacity);
/* EN/RU: Resolve staged branches / Заполнить смещения переходов. */
int fir_object_relocate(unsigned char *actions, const FirAvm1Fixup *fixups, int count,
    const size_t *labels, size_t end, char *error, size_t capacity);
/* EN/RU: Wrap complete code bytes in SWO / Записать подготовленные команды в SWO. */
int fir_object_write(FILE *output, const FirModule *module, const unsigned char *actions,
    size_t size, char *error, size_t capacity);

#endif
