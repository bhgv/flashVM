# Typed application boundary from the common transport to the existing document service.
Idebridge: module
{
    PATH: con "idebridge.dis";  # Relative module selected by the embedding host.
    ReplyMeta: con 56;          # Seven hex words preceding each UTF-8 snapshot fragment.
    ChunkBytes: con 968;        # Maximum fragment bytes after metadata within the 1024-byte payload.


    # Publish a borrowed core reply as bounded UTF-8 fragments; notify selects View.
    # Returns one after rendezvous of every fragment, zero for invalid encoding.
    sendreply: fn(codec: Uibridge, output: chan of array of byte,
                  reply: Idecore->Reply, notify: int): int;

    # codec/core are explicitly loaded modules; input/output are registered byte-array channels.
    # events receives Pointer/Key/Text/Focus records without replacing the input subsystem.
    # Returns zero after acknowledged Shutdown, minus one on malformed framing/admission.
    # Blocks only ordinary Limbo processes; caller must consume replies/events until shutdown.
    serve: fn(codec: Uibridge, core: Idecore, input: chan of array of byte,
              output: chan of array of byte, events: chan of ref Uibridge->Message): int;


    # Own a document and its Dis editing policy, consuming FlashVM input in the same service.
    # codec/core/policy are loaded modules; input/output are ordinary byte-array channels.
    # Publishes View snapshots/selection after input and correlated Reply for explicit commands.
    # Returns zero after acknowledged Shutdown or minus one on admission/protocol failure.
    serveapp: fn(codec: Uibridge, core: Idecore, policy: Ideinput,
                 input: chan of array of byte, output: chan of array of byte): int;
};
