/* Copyright (c) 2026 bhgv
 * EN: Installed native-client C/C++ link probe, without opening a network session.
 * RU: Проверка C/C++ линковки установленного native client без сетевой сессии.
 */
#include <styx/client/client.h>


/* EN: No arguments/resources. Invoke the documented null-safe release and return 0.
 * This requires a C ABI symbol even when this source is compiled as C++98.
 * RU: Аргументов/ресурсов нет. Вызвать null-safe release и вернуть 0.
 * При сборке C++98 всё равно требуется символ с C ABI.
 */
int main(void)
{
    styxclientfree((StyxClient *)0); /* No session is created; the link must still resolve the real C function. */
    return 0; /* Runtime lifecycle/protocol checks belong to the hosted server/client tests. */
}
