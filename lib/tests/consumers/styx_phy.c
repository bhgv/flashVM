/* Copyright (c) 2026 bhgv
 * EN: Link the installed PHY-server header/target from both C and C++98.
 * RU: Слинковать установленный PHY-server header/target из C и C++98.
 */
#include <styx/server/server.h>


/* EN/RU: No arguments/resources; the documented null-server query must return zero.
 * Аргументов/ресурсов нет; query для null server должен вернуть ноль.
 */
int main(void)
{
    return styxphyspace((struct Styxserver *)0) != 0; /* Require the real C symbol even when compiled as C++98. */
}
