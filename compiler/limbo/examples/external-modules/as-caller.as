// Load the generated arithmetic_dis proxy first. sendToDis is an explicit native
// channel/transport capability supplied by a future checked Dis/SWF bridge.
function useArithmeticDis(sendToDis)
{
    var arithmetic = arithmetic_dis_proxy(sendToDis);

    // The names/types come from arithmetic.m, exactly as for the external AS module.
    // AS receives a callback instead of blocking the VM's event/render owner.
    var callId = arithmetic.call("Arithmetic", "add", [6, 7], function(value, error) {
        if (error != null) {
            trace("Arithmetic.add failed: " + error);
            return;
        }
        trace(value);
    });

    // The host owns pending transport roots until completion/error/host cancellation.
    // This correlation ID is unique only within this proxy instance and never wraps.
    return callId;
}
