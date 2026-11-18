/* Copyright (c) 2026 bhgv
 * EN: Public native/client capability APIs link as C++98 without exposing scope internals.
 * RU: Public native/client capability APIs линкуются как C++98, не раскрывая scope internals.
 */
#include "scope.h"
#include <cstdio>


/* EN/RU: Check null/preflight ownership and unchanged outputs, 0 success; no transport/VM fixture.
 * Проверить null/preflight ownership и unchanged outputs, 0 success; без transport/VM fixture.
 */
int main()
{
    unsigned long tag = 99; /* Refusal output sentinel. */
    unsigned long long generation = 99; /* Refusal generation sentinel. */
    size_t size = 99; /* Refusal extent sentinel. */
    if (styxbidirscopes(NULL, 0) || styxscopegroup(NULL) || styxscopeclient(NULL, NULL) ||
        styxscopeassign(NULL, 0, NULL, 0) || styxscopebinding(NULL, 0, &tag, &generation) || tag != 99 || generation != 99 ||
        styxscopeclientnext(NULL, &tag, NULL, 0, &size) != STYX_CONTROL_END || size != 99 ||
        styxscopeclientwait(NULL, 0) || styxscopeservicenext(NULL, NULL, 0, &size) != STYX_CONTROL_END || size != 99 ||
        styxscopeservicewait(NULL, 0) || styxscopeclientsend(NULL, 0, NULL, 0) ||
        styxscopestate(NULL) != STYX_CONTROL_READ_FAILED)
    {
        std::fputs("scoped broker C++98/preflight contract failed\n", stderr);
        return 2;
    }
    styxscopestop(NULL);
    styxscopefree(NULL);
    return 0;
}
