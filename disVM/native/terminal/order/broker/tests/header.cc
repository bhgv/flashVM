/* Copyright (c) 2026 bhgv
 * EN: C++98 links portable service codec and explicit native bidirectional ordered broker API.
 * RU: C++98 связывает переносимый кодек службы и явный native API двунаправленного брокера.
 */
#include "broker.h"
#include "attach.h"
#include "flashvm/sordctl.h"


/* EN/RU: Check scalar ABI and invalid native preflight, no transport or guest execution.
 * Проверить скалярный ABI и native preflight, без транспорта и исполнения гостя.
 */
int main()
{
    FVM_SOrderControl message = FVM_SOrderControl(); /* Zeroed portable values, no C99 initialization. */
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* Private wire buffer. */
    size_t used = 123; /* Failure preservation sentinel. */
    if (fvm_sorder_control_write(&message, bytes, sizeof(bytes), &used) != FVM_S_INVALID || used != 123 ||
        styxorderedoffer(NULL, 1, 1) || styxorderedofferwait(NULL, 1, 0) || styxorderedclient(NULL, 0) ||
        styxorderedready(NULL, STYX_ORDER_TX) || styxorderedagreed(NULL, STYX_ORDER_TX) || styxorderedwait(NULL, STYX_ORDER_TX, 0) ||
        styxorderedrenew(NULL, 0) || styxorderedservicewait(NULL, 0) || styxorderedclientsend(NULL, NULL, 0))
    {
        return 1;
    }
    StyxOrderedOptions options = StyxOrderedOptions(); /* C++98 native descriptor preflight. */
    if (styxorderedoptionsvalid(&options) || styxbidirorderedchannels(NULL, NULL, &options) ||
        styxbidirorderedlaunch(NULL, NULL, &options))
    {
        return 2;
    }
    styxorderedfree(NULL);
    return 0;
}
