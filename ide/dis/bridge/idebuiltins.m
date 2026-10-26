# Initial builtin declaration catalogs and structured parameter help for AS and Limbo.
Idebuiltins: module
{
    PATH: con "idebuiltins.dis";  # Relative declaration-provider module resource.
    Keyword,                      # Source-language keyword/construction.
    TypeName,                     # Builtin type or constructor.
    Function,                     # Global function.
    ModuleName,                   # Limbo module interface.
    ObjectName,                   # AS system object/namespace.
    Method,                       # Qualified method/function inside an object/module.
    Property,                     # Qualified property or constant.
    Operator: con iota;           # Source-language operator with short semantic help.


    # One overload's call tip. The selected parameter is independently copied.
    Calltip: adt
    {
        name: string;                           # Exact qualified callable name.
        signature: string;                      # Full displayed signature, including optional/rest syntax.
        summary: string;                        # Brief plain-text call documentation.
        active: int;                            # Selected parameter index, or -1 beyond admitted arguments.
        parameter: ref Idecomplete->Parameter;  # Active argument metadata, nil when active is -1.
    };


    # Return fresh initial AS declarations: keywords, operators, system objects and methods.
    # Entries document the selected FlashVM subset; availability still depends on runtime/profile.
    actionscript: fn(): array of ref Idecomplete->Item;


    # Return fresh Limbo language and selected Sys interface declarations.
    # Sys presence describes the module contract, not implementation inside the incomplete host.
    limbo: fn(): array of ref Idecomplete->Item;


    # Return optional Projection.* library call tips from the loaded Idevalue type catalog.
    # No arguments. Returns nil if the module is unavailable; never advertises new native AS globals.
    projection: fn(): array of ref Idecomplete->Item;


    # Select exact-name overload help from a bounded completion query result.
    # candidates are borrowed; name is qualified; argument is the zero-based active argument.
    # Returns copied call tips. Negative/out-of-bound input returns nil; no source parsing occurs.
    help: fn(candidates: array of ref Idecomplete->Item, name: string,
             argument: int): array of ref Calltip;
};
