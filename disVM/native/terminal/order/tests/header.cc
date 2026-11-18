/* Copyright (c) 2026 bhgv
 * EN: C++98 callers link the portable two-limb codec and native ordering API.
 * RU: C++98-клиенты связываются с переносимым кодеком и native API упорядочивания.
 */
#include "order.h"


/* EN/RU: Encode/decode unsigned64 MAX without native wire layout; return nonzero on contract failure.
 * Кодировать/читать unsigned64 MAX без native wire layout; ненулевой результат при ошибке.
 */
int main()
{
    FVM_SOrdinal number = {0xffffffffUL, 0xffffffffUL}; /* Portable full magnitude. */
    unsigned char bytes[9]; /* Maximum prefix extent. */
    size_t used = 0; /* Exact encoded size. */
    StyxOrder *order = styxordercreate(128, 8); /* Owned native engine. */
    if (!order || fvm_sordinal_write(&number, bytes, sizeof(bytes), &used) != FVM_S_OK || used != 9)
    {
        styxorderfree(order);
        return 1;
    }
    styxorderfree(order);
    return 0;
}
