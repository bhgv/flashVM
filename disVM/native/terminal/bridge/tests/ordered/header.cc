/* Copyright (c) 2026 bhgv
 * EN: C++98 public launch options and bridge/broker APIs link with real host libraries.
 * RU: Публичные launch-options и API моста/брокера C++98 связываются с настоящими host-библиотеками.
 */
#include "bridge/ordered.h"


/* EN/RU: Invalid/absent display setup creates nothing and preserves native preflight semantics.
 * Неверный/отсутствующий display setup ничего не создаёт и сохраняет native-preflight.
 */
int main()
{
    StyxOrderedTerminalOptions options = StyxOrderedTerminalOptions(); /* Zeroed native-only options. */
    options.struct_size = sizeof(options);
    if (styxbidirorderedterminal(NULL, NULL, &options) || styxbidirordereduibridge(NULL, DVM_UI_UI, 0) ||
        styxbidiruibridgeordered(NULL) || styxorderedclientceiling(NULL) ||
        styxorderedclientsendwait(NULL, NULL, 0, 0) != STYX_ORDERED_END || styxorderedwatch(NULL, NULL, NULL, NULL))
    {
        return 1;
    }
    return 0;
}
