implement GraphicalEditor; # Execute document policy on the real Dis VM.
include "sys.m"; # Use actual native file I/O through the common FlashVM ports.
include "uibridge.m"; # Exchange copied, bounded envelopes with the renderer.
include "idecore.m"; # Keep text and history in their separate channel owner.
include "ideinput.m"; # Reuse Dis keyboard, selection and revision policy.
include "idebridge.m"; # Reuse the bounded snapshot serializer.
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
    host := load Idehost Idehost->PATH; # Link only the native channel registration boundary.
    if (sys == nil || core == nil || codec == nil || bridge == nil || policy == nil || host == nil) { # Require all real modules.
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
    for (;;) { # Serialize every input, file operation and document acknowledgement.
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
