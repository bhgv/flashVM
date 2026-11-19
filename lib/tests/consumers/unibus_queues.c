/* Copyright (c) 2026 bhgv
 * EN: Installed native queue link/lifecycle probe for C and C++98 consumers.
 * RU: Проверка линковки/жизненного цикла native queues для C и C++98 SDK callers.
 */
#include <unibus/order.h>


/* EN: No arguments. Own one engine through create/free; return 1 on allocation
 * failure, 0 after cleanup. No consumers or carrier can race with disposal.
 * RU: Аргументов нет. Владеть engine от create до free; вернуть 1 при ошибке
 * allocation, 0 после cleanup. Нет конкурирующих consumers/carrier.
 */
int main(void)
{
    StyxOrder *queue = styxordercreate(256, 16); /* Real native allocation and initialization, not just header parsing. */
    if (!queue)
    {
        return 1; /* A failed constructor transfers no ownership. */
    }
    styxorderfree(queue); /* No concurrent callers exist; release native synchronization and storage. */
    return 0; /* Both constructor and destructor resolved through the exported SDK target. */
}
