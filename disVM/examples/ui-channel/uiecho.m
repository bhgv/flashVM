# Teaching service: no Sys/Draw/Tk, native window or second input subsystem.
Uiecho: module
{
    PATH: con "uiecho.dis"; # Explicit module resource for the future embedding host.


    # u is the loaded Uibridge codec; input/output are typed Dis byte-array channels.
    # Returns zero after acknowledged operation 2 (shutdown), minus one for bad input.
    # Operation 1 echoes text. This teaching service is not the IDE document engine.
    serve: fn(u: Uibridge, input: chan of array of byte,
              output: chan of array of byte): int;
};
