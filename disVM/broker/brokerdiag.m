# Diagnostic-only records. All integers wider than 31 bits stay exact hex strings.
Brokerdiag: module
{
    PATH: con "brokerdiag.dis"; # Relative module selected by the host namespace.
    MaxFrame: con 8192; # Hard diagnostic frame bound, independent of normal RPC payloads.


    Site: adt
    {
        build: string; # Exact originating build identity.
        path: string; # Source file, decoded from canonical UTF-8.
        moduleName: string; # Module/function description.
        line: string; # Eight hex digits, one-based line.
        column: string; # Eight hex digits, one-based UTF-8 byte column.
        offset: string; # Sixteen hex digits, exact byte offset.
        length: string; # Eight hex digits, operation/token byte extent.
        binding: string; # Eight hex digits, translation-local binding identity.
    };


    Event: adt
    {
        kind: int; # 1 slow, 2 timeout, 3 disconnected, 4 debugger pause, 5 owner failure.
        at: string; # Exact sixteen-digit monotonic observation time.
        channel: string; # Opaque eight-digit external channel identity.
        generation: string; # Eight-digit session generation.
        requestId: string; # Sixteen-digit request identity; zero for channel-only failure.
        parentChannel: string; # Calling/dependency channel when available.
        parentGeneration: string; # Calling/dependency session generation.
        parentRequest: string; # Calling/dependency request identity.
        objectId: string; # Opaque proxy identity, never a heap pointer.
        phase: int; # Last explicitly reported owner phase, 0..8.
        flags: int; # Bits 1 timeout, 2 disconnected, 4 owner finished.
        started: string; # Dispatch time, sixteen hex digits.
        updated: string; # Last owner update, distinct from diagnostic observation time.
        progress: string; # Exact owner-defined progress counter.
        transport: int; # 1 in-process, 2 IPC, 3 network metadata.
        logical: string; # Stable logical service name.
        localAddress: string; # Captured local endpoint description.
        peer: string; # Captured remote endpoint, including port when applicable.
        operation: string; # Requested access/call description.
        detail: string; # Last owner-published reason; decoder never invokes the owner.
        origin: ref Site; # Exact initiating variable/operator occurrence.
        creation: ref Site; # Channel creation origin.
        connection: ref Site; # Captured generation's connection origin.
    };


    # Decode borrowed counted ASCII wire bytes. Return (1, independently owned Event),
    # or (0, nil) without a partial record. No Sys, network, getters or channel wait occurs here.
    decode: fn(frame: array of byte): (int, ref Event);
};
