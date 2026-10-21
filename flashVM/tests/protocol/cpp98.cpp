/* C++98 ABI check uses only public protocol headers and the standalone C library. */
#include "flashvm/sbindings.h"
#include "flashvm/sstream.h"
#include "flashvm/revents.h"
#include "flashvm/rheartbeat.h"
#include "flashvm/rdebug.h"
/* Link framing, incremental input and directional metadata without a VM runtime. */
int main()
{
    unsigned char output[64], input[64];
    FVM_SStream stream;
    FVM_SRecord record;
    FVM_SBindings table;
    FVM_SBinding found;
    FVM_SInteger integer = {0, 63, 0};
    size_t size, used;
    if (fvm_sinteger_write(&integer, output, sizeof(output), &size) != FVM_S_OK || size != 1 ||
        output[0] != 0xbf || fvm_sinteger_read(output, size, &integer, &used) != FVM_S_OK ||
        fvm_sbindings_init(&table, 0, 0) != FVM_S_OK ||
        fvm_sbindings_lookup(&table, 3, &found) != FVM_S_UNKNOWN ||
        fvm_srecord_write(3, output, 1, input, sizeof(input), &size) != FVM_S_OK ||
        fvm_sstream_init(&stream, output, sizeof(output), 32) != FVM_S_OK ||
        fvm_sstream_push(&stream, input, size, &record, &used) != FVM_S_OK || record.tag != 3 ||
        record.size != 1 || record.payload[0] != 0xbf || fvm_sstream_reset(&stream) != FVM_S_OK)
        return 1;
    FVM_RHeartbeatPolicy policy = {0};
    FVM_RHeartbeat heartbeat;
    FVM_RDispatch *dispatch = 0;
    FVM_RDebug *debug = 0;
    (void)debug;
    policy.struct_size = sizeof(policy);
    policy.transports = FVM_RH_TRANSPORTS;
    if (fvm_rheartbeat_init(&heartbeat, &policy, 1, 0) != FVM_R_OK ||
        fvm_rdispatch_create(0, &dispatch) != FVM_R_OK ||
        fvm_rdispatch_resources(dispatch, 1, 0, 0, &used) != FVM_R_OK || used != 0 ||
        fvm_rdispatch_destroy(&dispatch) != FVM_R_OK)
        return 1;
    return 0;
}
