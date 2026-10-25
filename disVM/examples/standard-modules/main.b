# Run retained Inferno libraries as actual Dis modules in the packaged runtime.
implement StandardModules; # Bind this source to the standalone tutorial module.
# Native file/formatting operations are supplied by the shared host.
include "sys.m"; # Declare native I/O and diagnostic output.
# Buffered Unicode readers are ordinary Limbo code.
include "bufio.m"; # Declare retained buffered Unicode readers.
# String parsing and case conversion are ordinary Limbo code.
include "string.m"; # Declare string parsing and case conversion.
# Command-line parsing is independent of a shell executable.
include "arg.m"; # Declare the option parser and consuming argv accessor.
# Numeric name/service resolution uses the retained hosted module.
include "srv.m"; # Declare hosted name and service resolution.
# IEEE scalar/vector functions use the hosted native Math binding.
include "math.m"; # Declare scalar, vector and bit-conversion operations.
# Loader exposes the retained interpreter's real module descriptors/instructions.
include "loader.m"; # Declare real instruction, type and link introspection.
# Styx provides the actual 9P2000 wire codec.
include "styx.m"; # Declare 9P2000 request and response variants.
# Server-side byte-window helpers are independent of mounting a namespace.
include "styxservers.m"; # Declare server byte-window helpers without starting a network service.
# Directory ordering can be tested independently of directory enumeration.
include "readdir.m"; # Declare ordering independently of native directory enumeration.
# Debug interfaces name Draw.Context but no graphics implementation is loaded.
include "draw.m"; # Declare the historical Debug context type; do not load graphics.
# Debug initialization is separate from the still-missing /prog device.
include "debug.m"; # Declare debugger initialization and future /prog-facing operations.
# Dis declares opcode identities used to inspect the actual compiled program.
include "dis.m"; # Declare opcode identities for emitted-bytecode checks.

bufio: Bufio;                   # Loaded buffered-reader implementation.
Iobuf: import bufio;             # Bind reader methods to the actual loaded module.
styx: Styx;                      # Loaded codec module owns message methods.
Tmsg, Rmsg: import styx;               # Bind message constructors/methods to that real module.


# Parameterless standalone entry; no host Python or external emu executes this tutorial.
StandardModules: module # Declare the entry or helper documented above.
{ # Begin the documented module or function body.
    init: fn();                  # Entry invoked by the packaged Dis runner.
}; # Finish this documented scope without extra side effects.


# Exercise a sparse int case table with explicit ranges and a default.
# value is a borrowed scalar; return a stable branch identity.
integercase(value: int): int # Declare the entry or helper documented above.
{ # Begin the documented module or function body.
    case value {                # The NC64 adapter reads native-cell compiler tables.
    -7 to -2 =>                 # First half-open compiled range covers these source labels.
        return 11;              # Distinguish this branch from the others.
    10 to 15 =>                 # A gap prevents treating every integer as a match.
        return 22;              # Return the second range's identity.
    100 to 105 =>               # Keep a third row for binary-search dispatch.
        return 33;              # Return the high range's identity.
    * =>                        # Values in gaps use the emitted default PC.
        return 44;              # Default must not accidentally match a neighboring row.
    }                           # End this checked numeric dispatch.
} # Finish this documented scope without extra side effects.


# Exercise a big case whose endpoints exceed 32 bits.
# value is signed 64-bit; return the selected row identity or zero.
bigcase(value: big): int # Declare the entry or helper documented above.
{ # Begin the documented module or function body.
    case value {                # Big tables have padded native-cell headers and rows.
    big 4294967298 => # Compare full-width endpoints.
        return 1;               # Confirm the upper bits survived serialization and dispatch.
    big -4294967298 => # Also compare negative full-width endpoints.
        return 2;               # Confirm signed ordering rather than low-word matching.
    * =>                        # Other values use a checked default destination.
        return 0;               # Return an unambiguous default identity.
    }                           # End big dispatch.
} # Finish this documented scope without extra side effects.


# Exercise the compiler's dense indexed jump table, including its default branch.
# value is a scalar index; return a non-linear value preventing arithmetic substitution.
densecase(value: int): int # Declare the entry or helper documented above.
{ # Begin the documented module or function body.
    case value {                # Consecutive labels permit an indexed Dis jump table.
    0 =>                        # Each label has its own destination.
        return 2;               # First independently checked result.
    1 =>                        # Second table index.
        return 5;               # Non-linear result.
    2 =>                        # Third table index.
        return 11;              # Distinct result.
    3 =>                        # Fourth table index.
        return 17;              # Distinct result.
    4 =>                        # Fifth table index.
        return 23;              # Distinct result.
    5 =>                        # Sixth table index.
        return 31;              # Distinct result.
    6 =>                        # Seventh table index.
        return 41;              # Distinct result.
    7 =>                        # Eighth table index.
        return 47;              # Final table result.
    * =>                        # Negative/out-of-range values must use the default.
        return 97;              # Default destination identity.
    }                           # End the dense dispatch.
} # Finish this documented scope without extra side effects.


# Execute real library calls and fail visibly on any wrong value or missing binding.
# No parameters/result; all input is in memory and no user file is overwritten.
init() # Declare the entry or helper documented above.
{ # Begin the documented module or function body.
    sys := load Sys Sys->PATH;   # Resolve the native module registered by the host.
    bufio = load Bufio Bufio->PATH; # Canonical /dis/lib paths map to bundled module resources.
    strings := load String String->PATH; # Load the retained Limbo string implementation.
    args := load Arg Arg->PATH;  # Load the retained option parser.
    srv := load Srv Srv->PATH;   # Load actual native resolver bindings.
    math := load Math Math->PATH; # Load checked native math wrappers.
    loader := load Loader Loader->PATH; # Load actual module introspection.
    styx = load Styx Styx->PATH; # Bind constructors to the real wire-codec module.
    servers := load Styxservers Styxservers->PATH; # Load server helpers, not a replacement OS.
    readdir := load Readdir Readdir->PATH; # Load retained directory ordering logic.
    debug := load Debug Debug->PATH; # Load the debugger library without Draw/Tk services.
    if (sys == nil || bufio == nil || strings == nil || args == nil || srv == nil || math == nil || # Reject a missing dependency or incorrect result before continuing.
        loader == nil || styx == nil || servers == nil || readdir == nil || debug == nil) { # Complete the checked condition before entering its failure branch.
        raise "fail: standard module load"; # Missing libraries must not masquerade as working modules.
    }                           # Every module above must actually link.
    if (integercase(-4) != 11 || integercase(12) != 22 || integercase(102) != 33 || integercase(9) != 44 || # Reject a missing dependency or incorrect result before continuing.
        bigcase(big 4294967298) != 1 || bigcase(big -4294967298) != 2 || bigcase(big 7) != 0) { # Complete the checked condition before entering its failure branch.
        raise "fail: native-cell numeric dispatch"; # Detect case-table ABI mismatches immediately.
    }                           # Both interpreter and JIT runs execute these same source decisions.
    index := 0;                 # Reuse one function-scoped loop counter in Limbo.
    dense := array[] of {2, 5, 11, 17, 23, 31, 41, 47}; # Independent dense-case answers.
    for (index = 0; index < len dense; index++) { # Execute every emitted table destination.
        if (densecase(index) != dense[index]) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: dense numeric dispatch"; # Reject wrong entry stride or target-PC interpretation.
        }                       # A branch cannot borrow another label's result.
    }                           # Also test both sides outside the table below.
    if (densecase(-1) != 97 || densecase(8) != 97) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: dense numeric default"; # Require correct bounds/default routing.
    }                           # The current PC is changed only by validated destinations.
    reader := bufio->sopen("α🙂\nβ"); # Own one in-memory UTF-8 buffer with no native file handle.
    if (reader.getc() != 'α' || reader.getc() != 16r1f642 || reader.ungetc() < 0 || # Reject a missing dependency or incorrect result before continuing.
        reader.getc() != 16r1f642 || reader.getc() != '\n' || reader.getc() != 'β' || reader.getc() != Bufio->EOF) { # Complete the checked condition before entering its failure branch.
        raise "fail: Bufio Unicode"; # Check non-BMP decoding, pushback, newline and EOF.
    }                           # No GUI or external reader implements this behavior.
    reader.close();             # Release the buffer's filler ownership.
    if (strings->toupper("Abc") != "ABC" || !strings->prefix("α", "αβ")) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: String";   # Compare actual retained string-library results.
    }                           # Continue only after both operations succeed.
    (number, rest) := strings->toint("123tail", 10); # Parse a number and preserve its unconsumed suffix.
    if (number != 123 || rest != "tail") { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: String.toint"; # Detect incorrect tuple or string ownership.
    }                           # Numeric parser results are ordinary Dis values.
    args->init("tool" :: "-n" :: "7" :: "source.b" :: nil); # Supply a complete private argv list.
    if (args->progname() != "tool" || args->opt() != 'n' || args->arg() != "7" || args->opt() != 0) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Arg";      # Check option consumption and retained positional arguments.
    }                           # No command-line shell is involved.
    remaining := args->argv();   # Transfer the positional list exactly once; argv consumes its owner.
    if (remaining == nil || hd remaining != "source.b") { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Arg.argv"; # Never call the consuming accessor twice in one check.
    }                           # Retain the returned list through the following comparison.
    srv->init();                # Initialize the real hosted interface.
    for (index = 0; index < 100; index++) { # Exercise repeated native/VM result ownership.
        addresses := srv->iph2a("127.0.0.1"); # Numeric loopback requires no external DNS server.
        if (addresses == nil || hd addresses != "127.0.0.1" || srv->ipn2p("tcp", "80") != "80") { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: Srv";  # Verify both resolver results are genuine and usable.
        }                       # Each iteration releases the preceding result owners.
    }                           # Exceed the ordinary FD quota without consuming any descriptors.
    if (srv->ipn2p("tcp", "65536") != nil || srv->ipn2p("invalid", "80") != nil || # Reject a missing dependency or incorrect result before continuing.
        srv->ipa2h("not-an-address") != nil || srv->ipn2p("tcp", "+65536") != nil) { # Complete the checked condition before entering its failure branch.
        raise "fail: resolver error guards"; # Invalid inputs must not wrap or create successful answers.
    }                           # Preserve failure as nil results, matching Srv's interface.
    if (math->sqrt(9.0) != 3.0 || math->bits64real(big 16r3ff0000000000000) != 1.0 || # Reject a missing dependency or incorrect result before continuing.
        math->dot(array[] of {2.0, 3.0}, array[] of {4.0, 5.0}) != 23.0) { # Complete the checked condition before entering its failure branch.
        raise "fail: Math scalars/vector"; # Exercise actual float slots, bit conversion and local array descriptors.
    }                           # Native libm supplies scalar IEEE operations in this hosted build.
    values := array[] of {3.0, 1.0, 2.0}; # Keep source values independently of the sorting permutation.
    permutation := array[] of {0, 1, 2}; # Own the mutable permutation output.
    math->sort(values, permutation); # Sort indices using the retained checked Math implementation.
    if (permutation[0] != 1 || permutation[1] != 2 || permutation[2] != 0) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Math.sort"; # Observe the exact native-word output, not a host-side model.
    }                           # The original real array remains untouched.
    decoded := array[1] of int;  # Allocate a module-local native integer array.
    math->import_int(array[] of {byte 255, byte 255, byte 255, byte 255}, decoded); # Decode signed wire int32.
    if (decoded[0] != -1) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Math.import_int"; # Ensure external int32 is sign-extended into a complete NC64 cell.
    }                           # No stale upper bytes may remain.
    styx->init();                # Initialize the retained codec's Sys reference.
    servers->init(styx);         # Share this codec with the retained server helpers.
    request := ref Tmsg.Read(23, 7, big 4294967300, 16); # Preserve tag, fid and a full-width file offset.
    wire := request.pack();     # Produce an actual little-endian 9P2000 request.
    (used, unpacked) := styx->Tmsg.unpack(wire); # Decode using the same public protocol interface.
    if (used != len wire || unpacked == nil || tagof unpacked != tagof request) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Styx round trip"; # Reject incomplete or misclassified packets.
    }                           # Variant inspection follows only after a valid decode.
    pick message := unpacked {  # Read variant fields through Limbo's checked type refinement.
    Read =>                     # Only the original read operation is admitted here.
        if (message.tag != 23 || message.fid != 7 || message.offset != big 4294967300 || message.count != 16) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: Styx fields"; # Verify protocol data, including offset high bits.
        }                       # End explicit field checks.
    * =>                        # Unexpected variants are visible failures.
        raise "fail: Styx variant"; # Never treat a different decoded request as equivalent.
    }                           # End variant refinement.
    attach := ref Tmsg.Attach(24, 3, Styx->NOFID, "user", ""); # NOFID keeps its classic signed int32 representation.
    (attachused, attachmessage) := styx->Tmsg.unpack(attach.pack()); # Decode the wire sentinel on the wider host.
    if (attachused <= 0 || attachmessage == nil) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Styx attach"; # Require a real decoded authentication-free attachment.
    }                           # Refine the decoded variant before reading its fields.
    pick attached := attachmessage { # The wire may not silently change message variants.
    Attach =>                   # Check the inherited tag and no-authentication fid.
        if (attached.afid != Styx->NOFID || attached.tag != 24) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: Styx NOFID width"; # Reject unsigned extension of a classic int32 sentinel.
        }                       # Native int width must not change the wire protocol contract.
    * =>                        # Other decoded variants are wrong.
        raise "fail: Styx attach variant"; # Fail explicitly rather than trusting a cast.
    }                           # End attach refinement.
    opened := ref Rmsg.Open(25, Sys->Qid(big 16r1234567890abcdef, -1, 0), 4096); # Exercise Qid version high bits.
    (openused, openmessage) := styx->Rmsg.unpack(opened.pack()); # Decode with the actual response codec.
    if (openused <= 0 || openmessage == nil) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Styx open reply"; # No partial response may pass as complete.
    }                           # Inspect only the decoded Open variant.
    pick openreply := openmessage { # Preserve its tagged type during inspection.
    Open =>                     # Qid path is big; version retains signed int32 semantics.
        if (openreply.qid.vers != -1 || openreply.qid.path != big 16r1234567890abcdef) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: Styx Qid widths"; # Both halves of the resource identity must survive.
        }                       # The prepared codec applies this adaptation only to NC64.
    * =>                        # A different response type is invalid evidence.
        raise "fail: Styx open variant"; # Surface the mismatch in the actual Dis execution.
    }                           # End response refinement.
    for (length := 0; length < len wire; length++) { # A truncated request must never publish a partial message.
        (shortused, partial) := styx->Tmsg.unpack(wire[:length]); # Keep the original advertised size.
        if (shortused != 0 || partial != nil) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: truncated Styx frame"; # Require a clean need-more-data result.
        }                       # No parser exception is an acceptable substitute.
    }                           # The complete packet already passed the round-trip check above.
    malformed := array[] of {byte 0, byte 0, byte 0, byte 0, byte 0, byte 0, byte 0}; # Own a complete fixed header for impossible-size tests.
    for (length = 0; length < 7; length++) { # Every declared size below the header is invalid.
        malformed[0] = byte length; # Other length bytes remain zero.
        (badused, badrequest) := styx->Tmsg.unpack(malformed); # Test the request decoder.
        (badreplyused, badreply) := styx->Rmsg.unpack(malformed); # Test the response decoder independently.
        if (badused != -1 || badrequest != nil || badreplyused != -1 || badreply != nil) { # Reject a missing dependency or incorrect result before continuing.
            raise "fail: impossible Styx header size"; # Prevent slicing away the header and indexing outside it.
        }                       # No response object escapes a rejected frame.
    }                           # Both decoders remain usable after every error result.
    response := servers->readbytes(request, array of byte "abcd"); # An offset beyond this data must yield EOF.
    if (len response.data != 0) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Styx server EOF"; # This helper executes without mounting a namespace.
    }                           # Live network serving remains a separate integration step.
    (empty, count) := readdir->sortdir(array[0] of ref Sys->Dir, Readdir->NAME); # Empty ordering needs no Sys.dirread.
    if (count != 0 || len empty != 0 || !debug->init()) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Readdir/Debug initialization"; # Do not equate these checks with a working /prog debugger.
    }                           # The reference libraries are loaded and executing their admitted operations.
    inspected := load Nilmod "libraries-check.dis"; # Inspect this actual compiled module without invoking another init.
    if (inspected == nil || len loader->ifetch(inspected) == 0 || len loader->tdesc(inspected) == 0 || # Reject a missing dependency or incorrect result before continuing.
        len loader->link(inspected) == 0) { # Complete the checked condition before entering its failure branch.
        raise "fail: Loader introspection"; # Require real instructions, descriptors and exported links.
    }                           # No synthetic module metadata is returned by the host.
    instructions := loader->ifetch(inspected); # Fetch actual compiler-produced instructions.
    gotos := 0;                 # Count indexed computed jumps independently.
    cases := 0;                 # Count sparse integer-case instructions.
    bigcases := 0;              # Count full-width big-case instructions.
    for (index = 0; index < len instructions; index++) { # Inspect every opcode without executing a second init.
        if (int instructions[index].op == Dis->IGOTO) { # Identify indexed computed jumps.
            gotos++;            # Record real emitted coverage.
        }                       # Other instructions do not contribute to this count.
        if (int instructions[index].op == Dis->ICASE) { # Identify sparse integer cases.
            cases++;            # Record real emitted coverage.
        }                       # Preserve independent evidence for each table format.
        if (int instructions[index].op == Dis->ICASEL) { # Identify full-width big cases.
            bigcases++;         # Record real emitted coverage.
        }                       # This does not imply NC64 native table-leaf support.
    }                           # All counted instructions came from this compiled module.
    if (gotos == 0 || cases == 0 || bigcases == 0) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: missing emitted numeric table coverage"; # Do not claim unexercised table forms.
    }                           # Runtime mode counters separately distinguish JIT and interpretation.
    if (sys->fprint(sys->fildes(1), "PASS standard modules and native-cell tables\n") <= 0) { # Reject a missing dependency or incorrect result before continuing.
        raise "fail: Sys.fprint"; # Verify the formatted FD return value as well as its output.
    }                           # Exit only after all retained modules have executed successfully.
}                               # Dis process/heap cleanup follows the normal runtime lifecycle.
