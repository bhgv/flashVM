implement ClipboardDemo;
include "sys.m"; # Use ordinary runtime output; no window or native clipboard is opened.
include "uibridge.m"; # Shared counted-value envelope and canonical transfer identity.
include "ideclipboard.m"; # Reusable Dis-owned fragment staging, also used by the graphical controller.
include "idehost.m"; # Import operation constants only; this example never loads $Idehost.

ClipboardDemo: module # A headless parameterless entry accepted by the installed Dis runtime.
{
    init: fn(); # Demonstrate private prefix ownership and complete Unicode publication.
};


# Stage two independently specified UTF-8 fragments without touching any document or OS clipboard.
# No parameters; loaded modules/private state stay rooted here until the final checked result.
init()
{
    sys := load Sys Sys->PATH; # Link actual Sys printing from the embedded host.
    codec := load Uibridge Uibridge->PATH; # Link the same value codec used by UI channels.
    clipboard := load Ideclipboard Ideclipboard->PATH; # Load the reusable byte-buffer owner.
    if (sys == nil || codec == nil || clipboard == nil) { # Missing modules are a real launch error.
        raise "fail: clipboard tutorial modules"; # Never print a successful demonstration without its implementation.
    }
    state := clipboard->create(); # [GC:] This process exclusively owns the mutable transfer and staging root.
    if (!clipboard->begin(state, clipboard->Paste, 42, 5, 3, 3)) { # Capture ticket/revision/source range as values.
        raise "fail: clipboard tutorial begin"; # No native pointer or descriptor substitutes for identity.
    }
    identity := codec->hexword(42); # Eight canonical ASCII digits precede every fragment body.
    first := ref Uibridge->Message(Uibridge->Request, 1, Idehost->ClipboardData, 5, 0, 9, identity + "Я😀"); # Six bytes.
    (status, ready, text) := clipboard->chunk(codec, state, first); # Stage the prefix in a bounded private byte array.
    if (status != clipboard->Ok || ready || text != "" || state.used != 6) { # A prefix must not become source text.
        raise "fail: clipboard tutorial prefix"; # Atomic publication is checked independently of printed output.
    }
    second := ref Uibridge->Message(Uibridge->Request, 1, Idehost->ClipboardData, 5, 6, 9, identity + "\nAB"); # Three bytes.
    (status, ready, text) = clipboard->chunk(codec, state, second); # Complete exactly nine bytes at the required offset.
    if (status != clipboard->Ok || !ready || text != "Я😀\nAB" || len text != 5) { # Byte offsets differ from scalar counts.
        raise "fail: clipboard tutorial completion"; # No partial or corrupted Unicode result is accepted.
    }
    if (state.action != clipboard->None || state.bytes != nil) { # Completed text owns its value after staging retires.
        raise "fail: clipboard tutorial retirement"; # A mutable native/shared staging buffer must not remain exposed.
    }
    sys->print("complete clipboard value: %s; scalars=%d\n", text, len text); # Print only the fully checked immutable value.
    sys->print("PASS Dis clipboard fragment tutorial\n"); # Actual codec/staging operations have all completed.
}
