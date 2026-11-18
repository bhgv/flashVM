/* Copyright (c) 2026 bhgv
 * EN: Public record-file and cancellable-wait APIs link from C++98 with ordinary C ownership.
 * RU: Публичные record-file и cancellable-wait API линкуются из C++98 с обычным C-владением.
 */
#include "files.h"


/* EN/RU: Invalid/empty native capability checks do no I/O, allocation or guest execution.
 * Проверки неверных/пустых capabilities без I/O, allocation и guest-кода.
 */
int main()
{
    if (styxorderedfiles(NULL) || !styxorderedfilesfree(NULL) || styxorderedfile(NULL, 0, FVM_ACCESS_READ) ||
        styxorderedclientwaitcancel(NULL, 0, NULL) ||
        styxorderedclientsendwaitcancel(NULL, NULL, 0, 0, NULL) != STYX_ORDERED_END ||
        styxorderedclientnextwaitcancel(NULL, NULL, NULL, 0, NULL, 0, NULL) != STYX_ORDER_END)
    {
        return 1;
    }
    styxorderedfilesstop(NULL);
    return 0;
}
