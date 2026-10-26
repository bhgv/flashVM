# Execute the frontend as a real Dis module.
implement ConsoleEditor;
# Use the supported native I/O interface.
include "sys.m";
# Use the existing document/history channel protocol.
include "idecore.m";
# Load the existing declaration-index protocol.
include "idecomplete.m";
# Use compiler-owned initial AS/Limbo declaration catalogs and call tips.
include "idebuiltins.m";
# Inspect compiled resources through an independent typed Dis file service.
include "idefiles.m";

sys: Sys;                                                # Loaded native I/O module for this frontend.
core: Idecore;                                           # Loaded document service; owns all edits and history.
commands: chan of Idecore->Request;                      # Blocking request endpoint owned by the document process.
completion: Idecomplete;                                 # Loaded channel-owned completion implementation.
builtins: Idebuiltins;                                   # Loaded declaration/call-tip provider.
ascommands: chan of Idecomplete->Request;                # Independent AS catalog owner.
limbocommands: chan of Idecomplete->Request;              # Independent Limbo catalog owner.
files: Idefiles;                                         # Loaded read-only resource inspector.
filecommands: chan of Idefiles->Request;                  # File-window owner request endpoint.
nextid := 1;                                             # Monotonic correlation ID for synchronous requests.
MaxLine: con 4096;                                       # Maximum accepted command extent in UTF-8 bytes.


# Terminal entry: the actual document/history owner remains in the loaded Dis module.
# Supply the documented module arguments or interface definition.
ConsoleEditor: module
# Begin the module scope described above.
{
    init: fn();                                          # Parameterless entry called by the native Dis runner.
# End the current block.
};


# Send one typed request and consume its acknowledgement before reusing caller state.
# operation/expected/range/text are copied values. Returns the owner's full snapshot.
# Supply the documented call arguments or interface definition.
call(operation, expected, first, last: int, text, replacement: string): Idecore->Reply
# Begin the call scope described above.
{
    # Keep this local reply value for the following checked operation.
    reply := chan of Idecore->Reply;
    # Keep this local request value for the following checked operation.
    request := Idecore->Request(nextid, operation, expected, first, last, text, replacement, reply);
    # Advance correlation only after constructing the complete request.
    nextid++;
    # Rendezvous with the separate document owner.
    commands <-= request;
    # Consume its unique acknowledgement before reusing caller state.
    return <-reply;
# End the current block.
}


# Split one command word while preserving interior spaces in the remaining text.
# source is borrowed immutable text. Returns word and its unconsumed suffix.
# Supply the documented splitword arguments or interface definition.
splitword(source: string): (string, string)
# Begin the splitword scope described above.
{
    # Keep this local first value for the following checked operation.
    first := 0;
    # Iterate within the explicit splitword bounds and stop at its stated condition.
    while (first < len source && source[first] == ' ') {
        # Advance the local position while preserving the surrounding bound.
        first++;
    # End the current block.
    }
    # Keep this local last value for the following checked operation.
    last := first;
    # Iterate within the explicit splitword bounds and stop at its stated condition.
    while (last < len source && source[last] != ' ') {
        # Advance the local position while preserving the surrounding bound.
        last++;
    # End the current block.
    }
    # Keep this local word value for the following checked operation.
    word := source[first:last];
    # Iterate within the explicit splitword bounds and stop at its stated condition.
    while (last < len source && source[last] == ' ') {
        # Advance the local position while preserving the surrounding bound.
        last++;
    # End the current block.
    }
    # Return this checked splitword result to the caller.
    return (word, source[last:]);
# End the current block.
}


# Parse a bounded nonnegative character offset without integer overflow.
# token contains ASCII decimal digits. Returns validity and the parsed value.
# Supply the documented number arguments or interface definition.
number(token: string): (int, int)
# Begin the number scope described above.
{
    # Keep this local value value for the following checked operation.
    value := 0;
    # Check this number boundary before consuming or publishing data.
    if (len token == 0) {
        # Return this checked number result to the caller.
        return (0, 0);
    # End the current block.
    }
    # Iterate within the explicit number bounds and stop at its stated condition.
    for (index := 0; index < len token; index++) {
        # Keep this local digit value for the following checked operation.
        digit := token[index] - '0';
        # Check this number boundary before consuming or publishing data.
        if (digit < 0 || digit > 9 || value > (Idecore->MaxCharacters - digit) / 10) {
            # Return this checked number result to the caller.
            return (0, 0);
        # End the current block.
        }
        # Complete this explicit number step; the owner protocol remains synchronous.
        value = value * 10 + digit;
    # End the current block.
    }
    # Return this checked number result to the caller.
    return (1, value);
# End the current block.
}


# Decode the documented text escapes; paths are deliberately passed verbatim.
# source is command text. Returns zero for an unknown/trailing escape, otherwise decoded text.
# Supply the documented unescape arguments or interface definition.
unescape(source: string): (int, string)
# Begin the unescape scope described above.
{
    # Keep this local result value for the following checked operation.
    result := "";
    # Iterate within the explicit unescape bounds and stop at its stated condition.
    for (index := 0; index < len source; index++) {
        # Keep this local character value for the following checked operation.
        character := source[index];
        # Check this unescape boundary before consuming or publishing data.
        if (character == '\\') {
            # Advance the local position while preserving the surrounding bound.
            index++;
            # Check this unescape boundary before consuming or publishing data.
            if (index == len source) {
                # Return this checked unescape result to the caller.
                return (0, "");
            # End the current block.
            }
            # Complete this explicit unescape step; the owner protocol remains synchronous.
            character = source[index];
            # Check this unescape boundary before consuming or publishing data.
            if (character == 'n') {
                # Complete this explicit unescape step; the owner protocol remains synchronous.
                character = '\n';
            # Handle the alternative branch of the current checked operation.
            } else if (character == 't') {
                # Complete this explicit unescape step; the owner protocol remains synchronous.
                character = '\t';
            # Handle the alternative branch of the current checked operation.
            } else if (character == 'r') {
                # Complete this explicit unescape step; the owner protocol remains synchronous.
                character = '\r';
            # Handle the alternative branch of the current checked operation.
            } else if (character == 's') {
                # Complete this explicit unescape step; the owner protocol remains synchronous.
                character = ' ';
            # Handle the alternative branch of the current checked operation.
            } else if (character != '\\') {
                # Return this checked unescape result to the caller.
                return (0, "");
            # End the current block.
            }
        # End the current block.
        }
        # Append one decoded Unicode character to the result.
        result[len result] = character;
    # End the current block.
    }
    # Return this checked unescape result to the caller.
    return (1, result);
# End the current block.
}


# Verify lossless UTF-8 import before any document mutation.
# bytes is borrowed; return validity and decoded text, preserving actual replacement characters.
# Supply the documented decode arguments or interface definition.
decode(bytes: array of byte): (int, string)
# Begin the decode scope described above.
{
    # Keep this local text value for the following checked operation.
    text := string bytes;
    # Re-encode for a byte-exact UTF-8 round-trip check.
    roundtrip := array of byte text;
    # Check this decode boundary before consuming or publishing data.
    if (len bytes != len roundtrip) {
        # Return this checked decode result to the caller.
        return (0, "");
    # End the current block.
    }
    # Iterate within the explicit decode bounds and stop at its stated condition.
    for (index := 0; index < len bytes; index++) {
        # Reject a lossy UTF-8 decoding before modifying the document.
        if (bytes[index] != roundtrip[index]) {
            # Return this checked decode result to the caller.
            return (0, "");
        # End the current block.
        }
    # End the current block.
    }
    # Return this checked decode result to the caller.
    return (1, text);
# End the current block.
}


# Read one bounded terminal line, consuming excess input before reporting an error.
# input borrows stdin. Returns 0 at EOF, 1 for a complete command, -1 for invalid/oversized input.
# Supply the documented readline arguments or interface definition.
readline(input: ref Sys->FD): (int, string)
# Begin the readline scope described above.
{
    # Bound terminal storage before accepting input.
    storage := array[MaxLine] of byte;
    # Read one byte without waiting to fill a large terminal buffer.
    single := array[1] of byte;
    # Keep this local used value for the following checked operation.
    used := 0;
    # Keep this local overflow value for the following checked operation.
    overflow := 0;
    # Iterate within the explicit readline bounds and stop at its stated condition.
    for (;;) {
        # Consume actual native bytes and inspect the returned extent.
        count := sys->read(input, single, 1);
        # Distinguish an actual native read failure from EOF.
        if (count < 0) {
            # Report a real failure to the native runtime instead of claiming success.
            raise "fail: terminal input";
        # End the current block.
        }
        # Check this readline boundary before consuming or publishing data.
        if (count == 0 || single[0] == byte '\n') {
            # Check this readline boundary before consuming or publishing data.
            if (overflow) {
                # Return this checked readline result to the caller.
                return (-1, "");
            # End the current block.
            }
            # Check this readline boundary before consuming or publishing data.
            if (count == 0 && used == 0) {
                # Return this checked readline result to the caller.
                return (0, "");
            # End the current block.
            }
            # Check this readline boundary before consuming or publishing data.
            if (used > 0 && storage[used - 1] == byte '\r') {
                # Advance the local position while preserving the surrounding bound.
                used--;
            # End the current block.
            }
            # Keep this local (valid, text) value for the following checked operation.
            (valid, text) := decode(storage[:used]);
            # Check this readline boundary before consuming or publishing data.
            if (!valid) {
                # Return this checked readline result to the caller.
                return (-1, "");
            # End the current block.
            }
            # Return this checked readline result to the caller.
            return (1, text);
        # End the current block.
        }
        # Check this readline boundary before consuming or publishing data.
        if (used == MaxLine) {
            # Remember overflow while continuing to consume the rejected line.
            overflow = 1;
        # Handle the alternative branch of the current checked operation.
        } else {
            # Store the accepted byte at the validated position.
            storage[used] = single[0];
            # Advance only after storing one byte inside the fixed buffer.
            used++;
        # End the current block.
        }
    # End the current block.
    }
# End the current block.
}


# Read one bounded UTF-8 source file without touching the document on failure.
# path is a native file path. Returns success and source text, or a diagnostic.
# Supply the documented loadfile arguments or interface definition.
loadfile(path: string): (int, string)
# Begin the loadfile scope described above.
{
    # Keep this local input value for the following checked operation.
    input := sys->open(path, Sys->OREAD);
    # Check this loadfile boundary before consuming or publishing data.
    if (input == nil) {
        # Return this checked loadfile result to the caller.
        return (0, sys->sprint("cannot open: %r"));
    # End the current block.
    }
    # Bound file storage to the largest permitted UTF-8 document.
    bytes := array[4 * Idecore->MaxCharacters] of byte;
    # Keep this local used value for the following checked operation.
    used := 0;
    # Iterate within the explicit loadfile bounds and stop at its stated condition.
    for (;;) {
        # Check this loadfile boundary before consuming or publishing data.
        if (used == len bytes) {
            # Keep this local probe value for the following checked operation.
            probe := array[1] of byte;
            # Consume actual native bytes and inspect the returned extent.
            extra := sys->read(input, probe, 1);
            # Check this loadfile boundary before consuming or publishing data.
            if (extra != 0) {
                # Return this checked loadfile result to the caller.
                return (0, "source exceeds UTF-8 byte limit");
            # End the current block.
            }
            # Finish the current loop only after its condition has been handled.
            break;
        # End the current block.
        }
        # Consume actual native bytes and inspect the returned extent.
        count := sys->read(input, bytes[used:], len bytes - used);
        # Distinguish an actual native read failure from EOF.
        if (count < 0) {
            # Return this checked loadfile result to the caller.
            return (0, "source read failed");
        # End the current block.
        }
        # Check this loadfile boundary before consuming or publishing data.
        if (count == 0) {
            # Finish the current loop only after its condition has been handled.
            break;
        # End the current block.
        }
        # Complete this explicit loadfile step; the owner protocol remains synchronous.
        used += count;
    # End the current block.
    }
    # Release the borrowed language FD owner and close its native file.
    input = nil;
    # Keep this local (valid, text) value for the following checked operation.
    (valid, text) := decode(bytes[:used]);
    # Check this loadfile boundary before consuming or publishing data.
    if (!valid || len text > Idecore->MaxCharacters) {
        # Return this checked loadfile result to the caller.
        return (0, "invalid UTF-8 or document character limit");
    # End the current block.
    }
    # Return this checked loadfile result to the caller.
    return (1, text);
# End the current block.
}


# Write the complete snapshot before asking the owner to acknowledge its saved revision.
# path/text are immutable caller values. Returns success only after every byte was written.
# Supply the documented savefile arguments or interface definition.
savefile(path, text: string): int
# Begin the savefile scope described above.
{
    # Encode the complete snapshot before any destructive create.
    bytes := array of byte text;
    # Keep this local output value for the following checked operation.
    output := sys->create(path, Sys->OWRITE, 8r666);
    # Check this savefile boundary before consuming or publishing data.
    if (output == nil) {
        # Return this checked savefile result to the caller.
        return 0;
    # End the current block.
    }
    # Keep this local written value for the following checked operation.
    written := 0;
    # Iterate within the explicit savefile bounds and stop at its stated condition.
    while (written < len bytes) {
        # Write the unsent snapshot suffix and retain its actual count.
        count := sys->write(output, bytes[written:], len bytes - written);
        # Check this savefile boundary before consuming or publishing data.
        if (count <= 0) {
            # Return this checked savefile result to the caller.
            return 0;
        # End the current block.
        }
        # Complete this explicit savefile step; the owner protocol remains synchronous.
        written += count;
    # End the current block.
    }
    # Close the completed output before acknowledging a saved revision.
    output = nil;
    # Return this checked savefile result to the caller.
    return 1;
# End the current block.
}


# Print plain metadata; failed owner operations preserve their returned document snapshot.
# state is copied from a real acknowledgement. Returns no value.
# Supply the documented report arguments or interface definition.
report(state: Idecore->Reply)
# Begin the report scope described above.
{
    # Show this bounded result or prompt on real standard output.
    sys->print("STATUS %d revision=%d saved=%d chars=%d\n", state.status,
               # Complete this explicit report step; the owner protocol remains synchronous.
               state.revision, state.saved, len state.text);
# End the current block.
}


# Send one read-only resource request and consume its private acknowledgement.
# operation/path are copied inputs. Returns the correlated file-window response.
filecall(operation: int, path: string): Idefiles->Reply
{
    # Isolate the result from concurrent document/completion response channels.
    reply := chan of Idefiles->Reply;
    # Read only the first sixteen bytes; Shutdown ignores these window fields.
    request := Idefiles->Request(nextid, operation, path, big 0, Sys->SEEKSTART, 16, reply);
    # Reserve a distinct correlation identity before rendezvous.
    nextid++;
    # The Dis owner performs all file positioning and reading.
    filecommands <-= request;
    # Consume the result before reusing caller-owned values.
    return <-reply;
}


# Display a read-only resource size and first sixteen bytes in hexadecimal.
# path is a literal native path with spaces preserved. No document state is changed.
inspect(path: string)
{
    # Request a bounded binary window instead of decoding arbitrary bytes as source.
    result := filecall(files->Read, path);
    # Keep file errors separate from editor/history status.
    if (result.status != files->Ok) {
        # Report plain fixed diagnostic text supplied by the file owner.
        sys->print("ERROR inspect: %s\n", result.message);
        # Preserve the current document and its saved revision.
        return;
    }
    # big offsets preserve the file's measured extent without narrowing to int.
    sys->print("RESOURCE size=%bd offset=%bd bytes=%d\n", result.size, result.position, len result.bytes);
    # Emit bounded byte values, including embedded NUL, without terminal control effects.
    for (index := 0; index < len result.bytes; index++) {
        # Convert the byte to its unsigned hexadecimal representation.
        sys->print("%2.2x ", int result.bytes[index]);
    }
    # Terminate the binary preview without changing its underlying bytes.
    sys->print("\n");
}


# Display complete source using bounded writes rather than the native formatter's message limit.
# text is immutable document data. Returns after the terminal has received every chunk.
# Supply the documented display arguments or interface definition.
display(text: string)
# Begin the display scope described above.
{
    # Show this bounded result or prompt on real standard output.
    sys->print("TEXT %d\n", len text);
    # Iterate within the explicit display bounds and stop at its stated condition.
    for (first := 0; first < len text; first += 512) {
        # Keep this local last value for the following checked operation.
        last := first + 512;
        # Check this display boundary before consuming or publishing data.
        if (last > len text) {
            # Complete this explicit display step; the owner protocol remains synchronous.
            last = len text;
        # End the current block.
        }
        # Show this bounded result or prompt on real standard output.
        # Render controls visibly while preserving the immutable source snapshot.
        shown := text[first:last];
        # Keep line breaks and tabs meaningful; other controls become one visible Unicode character.
        for (index := 0; index < len shown; index++) {
            # Read one Unicode value, not a UTF-8 byte.
            character := shown[index];
            # Do not pass hidden C0 controls to the terminal renderer.
            if (character < 32 && character != '\n' && character != '\t') {
                # A copy-on-write string update leaves the document snapshot unchanged.
                shown[index] = 16r2400 + character;
            # DEL has its own conventional control-picture codepoint.
            } else if (character == 127) {
                # Preserve one displayed character for the original source character.
                shown[index] = 16r2421;
            # End the control-character mapping.
            }
        # End this bounded display chunk.
        }
        # Output the sanitized display without modifying source bytes or history.
        sys->print("%s", shown);
    # End the current block.
    }
    # Show this bounded result or prompt on real standard output.
    sys->print("\nEND\n");
# End the current block.
}


# Exchange one declaration-index message and consume its copied response.
# owner/operation/prefix/items borrow caller values. Returns the acknowledged generation result.
catalog(owner: chan of Idecomplete->Request, operation: int, prefix: string,
        items: array of ref Idecomplete->Item): Idecomplete->Reply
{ # Keep index ownership and publication entirely in its Dis process.
    reply := chan of Idecomplete->Reply; # Retain one private response rendezvous.
    request := Idecomplete->Request(nextid, operation, 1, prefix, Idecomplete->MaxResults, items, reply); # Fully initialize the value.
    nextid++; # Advance correlation without sharing a mutable request.
    owner <-= request; # Send the request through the normal blocking channel.
    return <-reply; # Consume the acknowledgement before dropping the rendezvous.
} # The returned candidates are independent copies owned by the caller.


# Show a bounded builtin prefix list or exact-name parameter help.
# language selects AS/Limbo; rest holds prefix, or symbol plus zero-based argument. Returns nothing.
complete(language, rest: string, hint: int)
{ # This frontend formats metadata; it does not parse source or invent declarations.
    owner: chan of Idecomplete->Request; # Select exactly one language namespace.
    if (language == "as") { # AS system objects, methods, keywords and operators use their catalog.
        owner = ascommands; # Retain the real AS service endpoint.
    } else if (language == "limbo") { # Limbo declarations use a separate admitted generation.
        owner = limbocommands; # Keep prefixes from different languages independent.
    } else { # Unknown names must not silently select a different language.
        sys->print("ERROR language must be as or limbo\n"); # Explain the rejected command.
        return; # No owner request was issued.
    } # The selected owner now determines all returned records.
    symbol := rest; # A completion prefix may contain qualified-name separators.
    argument := 0; # Call tips use a zero-based active argument.
    if (hint) { # Exact-name help additionally requires its argument position.
        (name, suffix) := splitword(rest); # Preserve the exact qualified symbol.
        (indextext, extra) := splitword(suffix); # Reject trailing command data.
        (valid, index) := number(indextext); # Parse without overflowing a VM integer.
        if (!valid || extra != "") { # Invalid input cannot generate a misleading call tip.
            sys->print("ERROR hint LANGUAGE NAME ARGUMENT\n"); # State the accepted shape.
            return; # Preserve both catalog generations.
        } # The argument index is now a bounded integer.
        symbol = name; # Query the exact-name prefix before overload selection.
        argument = index; # Select the requested parameter in every exact overload.
    } # Prefix queries do not need an argument index.
    response := catalog(owner, completion->Query, symbol, nil); # Receive bounded copied metadata.
    if (response.status != completion->Ok) { # A rejected query must not be shown as successful completion.
        sys->print("ERROR completion status=%d\n", response.status); # Report the real owner status.
        return; # Never inspect rejected candidate data.
    } # Candidate records now belong exclusively to this response.
    if (!hint) { # Plain completion reports the ordered prefix candidates.
        sys->print("COMPLETIONS %d\n", len response.items); # Make empty results explicit.
        for (index := 0; index < len response.items; index++) { # Respect the owner's bounded result count.
            item := response.items[index]; # Borrow one copied declaration for formatting.
            sys->print("CANDIDATE %s %s\n", item.name, item.signature); # Show insertion spelling and signature.
            sys->print("  %s\n", item.documentation); # Show plain short help without markup execution.
        } # No catalog object is mutated by displaying it.
    } else { # Exact overload help uses the declaration provider's existing policy.
        tips := builtins->help(response.items, symbol, argument); # Return copied active-parameter help.
        sys->print("HINTS %d\n", len tips); # Distinguish absent symbols from an admitted overload.
        for (index := 0; index < len tips; index++) { # Handle every matching overload independently.
            tip := tips[index]; # Borrow the returned immutable display value.
            sys->print("SIGNATURE %s\n", tip.signature); # Display its actual callable signature.
            sys->print("  %s\n", tip.summary); # Display the callable's short documentation.
            if (tip.parameter != nil) { # Out-of-range arguments have no active parameter.
                parameter := tip.parameter; # Borrow that copied argument descriptor.
                sys->print("PARAMETER %d %s:%s %s\n", tip.active, parameter.name, parameter.type_name, parameter.summary); # Show names/types/help.
            } # Never dereference a missing active-parameter descriptor.
        } # Every displayed overload still comes from admitted declaration records.
    } # The owner will remain alive until explicit terminal shutdown.
} # Document text and its saved revision are unchanged by completion.


# List commands and units for this runnable console frontend.
# No parameters or return value; document/history logic stays in idecore.dis.
# Supply the documented help arguments or interface definition.
help()
# Begin the help scope described above.
{
    # Show this bounded result or prompt on real standard output.
    sys->print("help; show; status; set TEXT; append TEXT; insert OFFSET TEXT; delete START END\n");
    # Show this bounded result or prompt on real standard output.
    sys->print("find TEXT; replaceall OLD|NEW; undo; redo; load PATH; load! PATH; save PATH\n");
    # Show this bounded result or prompt on real standard output.
    sys->print("inspect PATH (read-only binary resource header)\n");
    # List language-aware catalog and parameter help.
    sys->print("complete LANGUAGE PREFIX; hint LANGUAGE NAME ARGUMENT (LANGUAGE: as or limbo)\n");
    # Explain dirty-state and offset semantics before edits.
    sys->print("quit (clean only); quit! (discard). Offsets count Unicode characters.\n");
    # Show this bounded result or prompt on real standard output.
    sys->print("Text escapes: \\n \\t \\r \\s \\\\. Paths are literal. Save replaces the named file.\n");
# End the current block.
}


# Own terminal input while a separate Dis process owns all document state.
# No parameters. Shutdown is acknowledged before normal return or unsaved-EOF failure.
# Supply the documented init arguments or interface definition.
init()
# Begin the init scope described above.
{
    # Resolve the actual module; no native replacement of document behavior is used.
    sys = load Sys Sys->PATH;
    # Resolve the actual module; no native replacement of document behavior is used.
    core = load Idecore Idecore->PATH;
    # Check this init boundary before consuming or publishing data.
    if (sys == nil || core == nil) {
        # Report a real failure to the native runtime instead of claiming success.
        raise "fail: console editor modules";
    # End the current block.
    }
    # Keep this local input value for the following checked operation.
    # Load actual completion and initial declaration providers.
    completion = load Idecomplete Idecomplete->PATH;
    # The provider describes language APIs, not unconditional native service availability.
    builtins = load Idebuiltins Idebuiltins->PATH;
    # Fail before publishing any catalog if its implementation cannot be linked.
    if (completion == nil || builtins == nil) {
        # A native replacement of this service is not accepted.
        raise "fail: completion modules";
    }
    # Give AS its independent channel owner.
    ascommands = completion->start();
    # Give Limbo a separate generation and namespace.
    limbocommands = completion->start();
    # Publish genuine initial AS declarations by value.
    asitems := builtins->actionscript();
    projectionitems := builtins->projection();
    combined := array[len asitems + len projectionitems] of ref Idecomplete->Item;
    combined[0:] = asitems;
    combined[len asitems:] = projectionitems;
    # Optional Projection.* names describe the explicitly loaded AS client library, not native globals.
    asready := catalog(ascommands, completion->Publish, "", combined);
    # Publish genuine language/Sys declaration metadata separately.
    limboready := catalog(limbocommands, completion->Publish, "", builtins->limbo());
    # Keep an incomplete index from masquerading as working completion.
    if (asready.status != completion->Ok || limboready.status != completion->Ok) {
        # The runtime will visibly fail the incomplete startup.
        raise "fail: completion catalogs";
    }
    # Resolve the actual read-only resource inspection implementation.
    files = load Idefiles Idefiles->PATH;
    # A missing implementation must fail before publishing its endpoint.
    if (files == nil) {
        # Do not substitute native editor policy.
        raise "fail: file-window module";
    }
    # Start a separate Dis process owning bounded file-window requests.
    filecommands = files->start(sys);
    # Keep the borrowed standard-input descriptor alive throughout the command loop.
    input := sys->fildes(0);
    # Check this init boundary before consuming or publishing data.
    if (input == nil) {
        # Report a real failure to the native runtime instead of claiming success.
        raise "fail: console stdin";
    # End the current block.
    }
    # Complete this explicit init step; the owner protocol remains synchronous.
    commands = core->start();
    # Send core->Open and retain the returned complete snapshot.
    state := call(core->Open, 0, 0, 0, "", "");
    # Show this bounded result or prompt on real standard output.
    sys->print("Dis IDE console: real document/history service; type help.\n");
    # Record unsaved EOF separately from an explicit forced quit.
    eofdirty := 0;
    # Iterate within the explicit init bounds and stop at its stated condition.
    for (;;) {
        # Show this bounded result or prompt on real standard output.
        sys->print("IDE> ");
        # Keep this local (ready, line) value for the following checked operation.
        (ready, line) := readline(input);
        # Treat terminal EOF separately from a valid empty command.
        if (ready == 0) {
            # Detect whether EOF would discard unacknowledged edits.
            eofdirty = state.revision != state.saved;
            # Finish the current loop only after its condition has been handled.
            break;
        # End the current block.
        }
        # Check this init boundary before consuming or publishing data.
        if (ready < 0) {
            # Show this bounded result or prompt on real standard output.
            sys->print("ERROR command exceeds limit or contains invalid UTF-8\n");
            # Keep the current document and accept the next command.
            continue;
        # End the current block.
        }
        # Keep this local (verb, rest) value for the following checked operation.
        (verb, rest) := splitword(line);
        # Select the requested command; state changes still go through the owner.
        if (verb == "") {
            # Keep the current document and accept the next command.
            continue;
        # End the current block.
        }
        # Select the requested command; state changes still go through the owner.
        if (verb == "quit" || verb == "quit!") {
            # Select the requested command; state changes still go through the owner.
            if (verb == "quit" && state.revision != state.saved) {
                # Show this bounded result or prompt on real standard output.
                sys->print("DIRTY: save or use quit! to discard\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Finish the current loop only after its condition has been handled.
            break;
        # End the current block.
        }
        # Select the requested command; state changes still go through the owner.
        if (verb == "help") {
            # Complete this explicit init step; the owner protocol remains synchronous.
            help();
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "inspect") {
            # Inspect literal bytes without loading them into the editable source document.
            inspect(rest);
        } else if (verb == "complete" || verb == "hint") {
            # Parse language separately from its case-sensitive qualified prefix.
            (language, query) := splitword(rest);
            # Query the real Dis completion owner without editing the document.
            complete(language, query, verb == "hint");
        } else if (verb == "show") {
            # Send core->Snapshot and retain the returned complete snapshot.
            state = call(core->Snapshot, 0, 0, 0, "", "");
            # Complete this explicit init step; the owner protocol remains synchronous.
            display(state.text);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "status") {
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "undo" || verb == "redo") {
            # Keep this local operation value for the following checked operation.
            operation := core->Undo;
            # Select the requested command; state changes still go through the owner.
            if (verb == "redo") {
                # Complete this explicit init step; the owner protocol remains synchronous.
                operation = core->Redo;
            # End the current block.
            }
            # Send operation and retain the returned complete snapshot.
            state = call(operation, state.revision, 0, 0, "", "");
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "load" || verb == "load!") {
            # Select the requested command; state changes still go through the owner.
            if (verb == "load" && state.revision != state.saved) {
                # Show this bounded result or prompt on real standard output.
                sys->print("DIRTY: save first or use load! to replace\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Keep this local (ok, text) value for the following checked operation.
            (ok, text) := loadfile(rest);
            # Check this init boundary before consuming or publishing data.
            if (!ok) {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR %s\n", text);
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Send core->Replace and retain the returned complete snapshot.
            state = call(core->Replace, state.revision, 0, len state.text, text, "");
            # Check this init boundary before consuming or publishing data.
            if (state.status == core->Ok) {
                # Send core->MarkSaved and retain the returned complete snapshot.
                state = call(core->MarkSaved, state.revision, 0, 0, "", "");
            # End the current block.
            }
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "save") {
            # Retain the exact immutable source revision being saved.
            snapshot := state;
            # Check this init boundary before consuming or publishing data.
            if (!savefile(rest, snapshot.text)) {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR save failed; document stays dirty\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Send core->MarkSaved and retain the returned complete snapshot.
            state = call(core->MarkSaved, snapshot.revision, 0, 0, "", "");
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "delete") {
            # Keep this local (starttext, suffix) value for the following checked operation.
            (starttext, suffix) := splitword(rest);
            # Keep this local (endtext, extra) value for the following checked operation.
            (endtext, extra) := splitword(suffix);
            # Keep this local (startok, start) value for the following checked operation.
            (startok, start) := number(starttext);
            # Keep this local (endok, end) value for the following checked operation.
            (endok, end) := number(endtext);
            # Check this init boundary before consuming or publishing data.
            if (!startok || !endok || extra != "") {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR delete START END\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Send core->Replace and retain the returned complete snapshot.
            state = call(core->Replace, state.revision, start, end, "", "");
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else if (verb == "replaceall") {
            # Keep this local separator value for the following checked operation.
            separator := 0;
            # Iterate within the explicit init bounds and stop at its stated condition.
            while (separator < len rest && rest[separator] != '|') {
                # Advance the local position while preserving the surrounding bound.
                separator++;
            # End the current block.
            }
            # Check this init boundary before consuming or publishing data.
            if (separator == len rest) {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR replaceall OLD|NEW\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Keep this local (leftok, needle) value for the following checked operation.
            (leftok, needle) := unescape(rest[:separator]);
            # Keep this local (rightok, replacement) value for the following checked operation.
            (rightok, replacement) := unescape(rest[separator + 1:]);
            # Check this init boundary before consuming or publishing data.
            if (!leftok || !rightok) {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR invalid text escape\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Send core->ReplaceAll and retain the returned complete snapshot.
            state = call(core->ReplaceAll, state.revision, 0, len state.text, needle, replacement);
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # Handle the alternative branch of the current checked operation.
        } else {
            # Keep this local offset value for the following checked operation.
            offset := 0;
            # Select the requested command; state changes still go through the owner.
            if (verb == "insert") {
                # Keep this local (position, suffix) value for the following checked operation.
                (position, suffix) := splitword(rest);
                # Keep this local (valid, parsed) value for the following checked operation.
                (valid, parsed) := number(position);
                # Check this init boundary before consuming or publishing data.
                if (!valid) {
                    # Show this bounded result or prompt on real standard output.
                    sys->print("ERROR insert OFFSET TEXT\n");
                    # Keep the current document and accept the next command.
                    continue;
                # End the current block.
                }
                # Complete this explicit init step; the owner protocol remains synchronous.
                offset = parsed;
                # Complete this explicit init step; the owner protocol remains synchronous.
                rest = suffix;
            # End the current block.
            }
            # Keep this local (valid, text) value for the following checked operation.
            (valid, text) := unescape(rest);
            # Check this init boundary before consuming or publishing data.
            if (!valid) {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR invalid text escape\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Select the requested command; state changes still go through the owner.
            if (verb == "set") {
                # Send core->Replace and retain the returned complete snapshot.
                state = call(core->Replace, state.revision, 0, len state.text, text, "");
            # Handle the alternative branch of the current checked operation.
            } else if (verb == "append") {
                # Send core->Replace and retain the returned complete snapshot.
                state = call(core->Replace, state.revision, len state.text, len state.text, text + "\n", "");
            # Handle the alternative branch of the current checked operation.
            } else if (verb == "insert") {
                # Send core->Replace and retain the returned complete snapshot.
                state = call(core->Replace, state.revision, offset, offset, text, "");
            # Handle the alternative branch of the current checked operation.
            } else if (verb == "find") {
                # Send core->Find and retain the returned complete snapshot.
                state = call(core->Find, state.revision, 0, len state.text, text, "");
                # Check this init boundary before consuming or publishing data.
                if (state.status == core->Ok) {
                    # Show this bounded result or prompt on real standard output.
                    sys->print("FOUND offset=%d line=%d column=%d\n", state.offset, state.line, state.column);
                # Handle the alternative branch of the current checked operation.
                } else {
                    # Show this bounded result or prompt on real standard output.
                    sys->print("NOT_FOUND\n");
                # End the current block.
                }
            # Handle the alternative branch of the current checked operation.
            } else {
                # Show this bounded result or prompt on real standard output.
                sys->print("ERROR unknown command; type help\n");
                # Keep the current document and accept the next command.
                continue;
            # End the current block.
            }
            # Complete this explicit init step; the owner protocol remains synchronous.
            report(state);
        # End the current block.
        }
    # End the current block.
    }
    # Send core->Shutdown and retain the returned complete snapshot.
    # Acknowledge both index owners before ending the application.
    asstopped := catalog(ascommands, completion->Shutdown, "", nil);
    # The second owner has its own reply rendezvous and lifecycle.
    limbostopped := catalog(limbocommands, completion->Shutdown, "", nil);
    # Do not claim clean exit if an index refused terminal shutdown.
    if (asstopped.status != completion->Ok || limbostopped.status != completion->Ok) {
        # Expose lifecycle errors to the native application caller.
        raise "fail: completion shutdown";
    }
    # Drain the file owner acknowledgement before terminating the application.
    filestopped := filecall(files->Shutdown, "");
    # A terminal request must not leave a blocked file-service process behind.
    if (filestopped.status != files->Ok) {
        # Expose a real lifecycle failure instead of claiming a completed shutdown.
        raise "fail: file-window shutdown";
    }
    state = call(core->Shutdown, state.revision, 0, 0, "", "");
    # Check this init boundary before consuming or publishing data.
    if (eofdirty) {
        # Report a real failure to the native runtime instead of claiming success.
        raise "fail: EOF with unsaved document";
    # End the current block.
    }
    # Show this bounded result or prompt on real standard output.
    sys->print("BYE document owner stopped\n");
# End the current block.
}
