implement GraphicalEditor; # Execute document policy on the real Dis VM.
include "sys.m"; # Use actual native file I/O through the common FlashVM ports.
include "uibridge.m"; # Exchange copied, bounded envelopes with the renderer.
include "idecore.m"; # Keep text and history in their separate channel owner.
include "ideinput.m"; # Reuse Dis keyboard, selection and revision policy.
include "idebridge.m"; # Reuse the bounded snapshot serializer.
include "ideviewport.m"; # Keep viewport decisions and bounds in a separate Dis module.
include "idehost.m"; # Register ordinary channels with the native window owner.

GraphicalEditor: module # Parameterless entry accepted by the experimental host.
{
    init: fn(); # Own the controller until acknowledged clean/forced shutdown.
};

sys: Sys; # Loaded file services, exclusive to this controller's Dis process.
core: Idecore; # Loaded document implementation; no native text authority.
codec: Uibridge; # Shared value codec, never a native pointer transport.
bridge: Idebridge; # Existing ordered Unicode-fragment serializer.


# Send one synchronous operation to the separate document owner and receive its result.
# Arguments are values/borrowed channels; blocks only this Limbo process.
call(document: chan of Idecore->Request, operation: int, revision: int, text: string): Idecore->Reply
{
    answers := chan of Idecore->Reply; # Private rendezvous destination for this call.
    document <-= Idecore->Request(1, operation, revision, 0, 0, text, "", answers); # Copy the request.
    return <-answers; # Return the acknowledged value snapshot.
}


# Read a bounded UTF-8 source from path, returning (success, text); no descriptors escape.
# Errors/oversize/invalid UTF-8 preserve the existing document; reads may block the host owner.
readsource(path: string): (int, string)
{
    if (path == "" || len path != len string array of byte path) { # Reject absent or invalid names.
        return (0, ""); # No file has been opened.
    }
    fd := sys->open(path, Sys->OREAD); # Obtain an actual read-only file reference.
    if (fd == nil) { # Report native open failure without replacing text.
        return (0, ""); # No descriptor is retained by the caller.
    }
    bytes := array[65537] of byte; # One extra byte detects the 64 KiB source limit.
    count := 0; # Accumulate actual reads, including short reads.
    while (count < len bytes) { # Read until EOF or the explicit bound.
        n := sys->read(fd, bytes[count:], len bytes - count); # Block only at supported native I/O.
        if (n < 0) { # An error must not look like an empty file.
            return (0, ""); # The local FD reference is released by Dis ownership.
        }
        if (n == 0) { # EOF completes this bounded source read.
            break; # Decode only received bytes.
        }
        count += n; # Preserve every short-read byte.
    }
    if (count > 65536) { # Never publish a silently truncated document.
        return (0, ""); # The current document stays unchanged.
    }
    text := string bytes[:count]; # Decode using production Dis Unicode conversion.
    encoded := array of byte text; # Detect replacement decoding by exact round trip.
    if (len encoded != count) { # Invalid UTF-8 can change the byte count.
        return (0, ""); # Reject before issuing a document command.
    }
    for (i := 0; i < count; i++) { # Check equally sized replacement sequences too.
        if (encoded[i] != bytes[i]) { # Every original byte must survive decoding.
            return (0, ""); # Report failure without editing the document.
        }
    }
    return (1, text); # Return an owned immutable source value.
}


# Write one acknowledged snapshot completely; return success only after all bytes.
# path/text are values; FD closes on return. This is checked overwrite, not atomic replacement.
writesource(path: string, text: string): int
{
    if (path == "") { # Saving requires an explicit configured destination.
        return 0; # Never guess a filename.
    }
    fd := sys->create(path, Sys->OWRITE, 8r666); # Use supported native overwrite semantics.
    if (fd == nil) { # Permission/open failure leaves the revision dirty.
        return 0; # No false saved acknowledgement is emitted.
    }
    bytes := array of byte text; # Preserve UTF-8, including non-BMP characters.
    count := 0; # Track actual successful writes.
    while (count < len bytes) { # Handle a short write without dropping its suffix.
        n := sys->write(fd, bytes[count:], len bytes - count); # Write the remaining bytes.
        if (n <= 0) { # Reject no-progress/error rather than spinning.
            return 0; # A partial native file is possible; saved revision remains unchanged.
        }
        count += n; # Advance only by actual successful output.
    }
    return 1; # Native Sys write flushes before reporting success.
}


# Own graphical editor policy with no Draw/Tk/AS-side document mutation.
# No parameters; channels remain rooted on this stack through the final reply.
init()
{
    sys = load Sys Sys->PATH; # Link the actual native file services.
    core = load Idecore Idecore->PATH; # Link the document/history module.
    codec = load Uibridge Uibridge->PATH; # Link the common channel codec.
    bridge = load Idebridge Idebridge->PATH; # Link the existing snapshot encoder.
    policy := load Ideinput Ideinput->PATH; # Link Dis-owned input policy.
    viewport := load Ideviewport Ideviewport->PATH; # Link the bounded logical-line viewport.
    host := load Idehost Idehost->PATH; # Link only the native channel registration boundary.
    if (sys == nil || core == nil || codec == nil || bridge == nil || policy == nil || host == nil || viewport == nil) { # Require all real modules.
        raise "fail: graphical editor modules"; # Stop visibly on an unsupported host.
    }
    input := chan of array of byte; # The native owner offers copied events here.
    output := chan of array of byte; # The native owner receives Dis-owned snapshots here.
    document := core->start(); # Spawn the existing separate document owner.
    state := policy->create(); # Keep cursor/control/history policy in Dis.
    path := host->path(); # Copy the explicit host-selected destination.
    text := ""; # A new document starts empty.
    if (path != "" && !host->fresh()) { # Loading is explicit and may fail without creating a file.
        (loaded, initial) := readsource(path); # Read only through Sys.
        if (!loaded) { # Initial load must not silently discard an existing file.
            raise "fail: graphical editor initial source"; # Native process reports the error.
        }
        text = initial; # Publish only validated UTF-8.
    }
    reply := call(document, core->Open, 0, text); # Establish the first saved revision.
    policy->observe(state, reply); # Synchronize the Dis selection with that revision.
    if (host->attach(input, output) != 0) { # Borrow these live stack-rooted ports.
        raise "fail: graphical editor channel registration"; # Never continue without a receiver.
    }
    window := viewport->create(); # The Dis controller exclusively owns its viewport.
    reveal := 1; # Initial presentation follows the acknowledged caret.
    ticket := 0; # Monotonic view identity also distinguishes scrolling within one revision.
    dragging := 0; # Only an admitted pointer begin starts a Dis selection gesture.
    for (;;) { # Serialize every input, file operation and document acknowledgement.
        if (ticket == 2147483647) { # Never wrap a view identity onto a stale input.
            raise "fail: graphical editor view ticket exhausted"; # Stop with explicit exhaustion.
        }
        ticket++; # Give every published view a distinct scalar identity.
        (first, last, top) := viewport->window(window, reply.text, state.caret, reveal); # Bound the display window.
        metadata := codec->hexword(top) + codec->hexword(len reply.text) + codec->hexword(ticket); # No heap addresses.
        placement := ref Uibridge->Message(codec->View, 0, host->Viewport, reply.revision, first, last, metadata); # Dis owns visible bounds.
        (placed, geometry) := codec->encode(placement); # Validate the copied viewport record.
        if (!placed) { # Reject malformed metadata before any snapshot fragment.
            raise "fail: graphical editor viewport"; # Never fabricate a valid view.
        }
        output <-= geometry; # Publish viewport identity before its document fragments.
        if (!bridge->sendreply(codec, output, reply, 1)) { # Send a complete Unicode-aligned snapshot.
            raise "fail: graphical editor snapshot"; # A framing failure is terminal.
        }
        view := ref Uibridge->Message(codec->View, 0, 65536, reply.revision, state.caret, state.anchor, ""); # Selection is Dis authority.
        (encoded, selection) := codec->encode(view); # Encode only copied scalar coordinates.
        if (!encoded) { # Refuse invalid coordinates/format before sending.
            raise "fail: graphical editor selection"; # Surface the protocol failure.
        }
        output <-= selection; # Complete the view before receiving another event.
        frame := <-input; # Block this process while native rendering/events continue.
        (valid, event) := codec->decode(frame); # Reject malformed transport input.
        if (!valid) { # Do not execute unvalidated event fields.
            raise "fail: graphical editor input"; # The native host reports failure.
        }
        reveal = 0; # Wheel/modifier events must not snap the viewport back to the caret.
        if (event.kind == codec->Request && (event.code == host->Scroll || event.code == host->Place)) { # Admit native geometry only as a request.
            expected := array of byte event.text; # The ticket is an exact eight-byte hexadecimal word.
            (known, seen) := codec->parseword(expected, 0); # Decode without trusting a native pointer.
            if (!known || len expected != 8 || seen != ticket || event.flags != reply.revision) { # Reject old revision or viewport geometry.
                reply.status = core->Conflict; # Preserve text, selection and history on stale input.
                continue; # Publish a fresh view identity with the failure status.
            }
            if (event.code == host->Scroll) { # Scroll never edits or moves the caret.
                viewport->scroll(window, reply.text, event.first); # Clamp the logical-line delta in Dis.
                reply.status = core->Ok; # Acknowledge the bounded viewport operation.
            } else { # The renderer only proposed a source scalar from its actual glyph layout.
                position := event.first; # Coordinates remain source Unicode scalars.
                phase := event.second; # Begin/update/end are 1/2/3, not native button structs.
                if (position < 0 || position > len reply.text || phase < 1 || phase > 3) { # Validate before changing selection state.
                    reply.status = core->RangeError; # Invalid geometry changes nothing.
                } else if (phase == 1 || dragging) { # An update alone cannot invent a drag gesture.
                    anchor := state.anchor; # Preserve the original gesture or Shift anchor.
                    if (phase == 1 && !state.shift) { # Ordinary click collapses selection.
                        anchor = position; # Only Dis makes this selection decision.
                    }
                    if (!policy->select(state, reply, anchor, position)) { # Recheck current document bounds/revision.
                        reply.status = core->Conflict; # No guessed stale coordinate is applied.
                    } else { # The acknowledged selection now belongs to this controller.
                        dragging = phase != 3; # End releases the gesture after its last selection.
                        reveal = 1; # Keep an admitted pointer selection in the displayed interval.
                        reply.status = core->Ok; # Acknowledge without changing document history.
                    }
                }
            }
            continue; # The ordinary input path must not apply the same event a second time.
        }
        if (event.kind == codec->Focus && event.flags == 0) { # Losing focus ends any pending drag.
            dragging = 0; # A later update cannot extend an old gesture.
        }
        if (event.kind == codec->Text || (event.kind == codec->Key && event.flags != 0
            && event.code != 16 && event.code != 17)) { # Navigation/editing reveal their caret; releases do not.
            reveal = 1; # Viewport policy remains in Dis.
        }
        operation := event.code; # Explicit request operation or virtual-key value.
        if (event.kind == codec->Key && event.flags != 0 && state.control) { # Shortcuts remain Dis policy.
            if (event.code == 83) { # Ctrl+S selects the configured destination.
                operation = host->Save; # Save without native document authority.
            } else if (event.code == 79) { # Ctrl+O reloads that same path.
                operation = host->Reload; # Dirty-state checks follow below.
            }
        }
        if ((event.kind == codec->Request || event.kind == codec->Key) && operation == host->Save) { # Serialize save with editing.
            reply = call(document, core->Snapshot, 0, ""); # Capture the exact current revision.
            if (writesource(path, reply.text)) { # Acknowledge only actual successful output.
                reply = call(document, core->MarkSaved, reply.revision, ""); # Mark that revision saved.
            } else { # Keep the original revision/saved metadata on failure.
                reply.status = core->Invalid; # Present a visible save error.
            }
        } else if (event.kind == codec->Request && operation == host->Quit) { # Window close is also a request.
            if (reply.revision == reply.saved || event.flags == 1) { # Forced discard must be explicit.
                call(document, core->Shutdown, reply.revision, ""); # Stop the real owner before returning.
                host->detach(); # Stop native borrowing before this stack releases the channel roots.
                return; # Remaining acknowledged process exits can now be reaped safely.
            }
            reply.status = core->Conflict; # Dirty close is refused and visibly reported.
        } else if ((event.kind == codec->Request || event.kind == codec->Key) && operation == host->Reload) { # Reload through Dis I/O.
            if (reply.revision != reply.saved) { # Preserve unsaved changes.
                reply.status = core->Conflict; # Signal the refused reload.
            } else { # Only a clean document can be replaced.
                (loaded, source) := readsource(path); # Validate before changing the owner.
                if (loaded) { # Close/open establishes a fresh saved document session.
                    reply = call(document, core->Close, reply.revision, ""); # Retire old history first.
                    reply = call(document, core->Open, reply.revision, source); # Install validated text.
                    policy->observe(state, reply); # Reset selection to the new document.
                } else { # Failed reads must preserve text/history.
                    reply.status = core->Invalid; # Publish the failure.
                }
            }
        } else { # Ordinary keys/text/focus use the existing production Dis controller.
            (admitted, result) := policy->apply(core, document, state, event); # Execute through document channels.
            if (!admitted) { # Reject unknown/malformed input explicitly.
                raise "fail: graphical editor input policy"; # Native host reports the error.
            }
            reply = result.document; # Publish only the owner's acknowledged snapshot.
        }
    }
}
