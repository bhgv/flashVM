implement GraphicalEditor; # Execute document policy on the real Dis VM.
include "sys.m"; # Use actual native file I/O through the common FlashVM ports.
include "uibridge.m"; # Exchange copied, bounded envelopes with the renderer.
include "idecore.m"; # Keep text and history in their separate channel owner.
include "ideinput.m"; # Reuse Dis keyboard, selection and revision policy.
include "idebridge.m"; # Reuse the bounded snapshot serializer.
include "idefileops.m"; # Reuse bounded source I/O independently of any renderer.
include "idesearch.m"; # Reuse literal document operations through typed channels.
include "ideviewport.m"; # Keep viewport decisions and bounds in a separate Dis module.
include "idehost.m"; # Register ordinary channels with the native window owner.

GraphicalEditor: module # Parameterless entry accepted by the experimental host.
{
    init: fn(); # Own the controller until acknowledged clean/forced shutdown.
};

sys: Sys; # Loaded file services, exclusive to this controller's Dis process.
core: Idecore; # Loaded document implementation; no native text authority.
codec: Uibridge; # Shared value codec, never a native pointer transport.
files: Idefileops; # Reusable checked source-file library, called only by Dis policy.
search: Idesearch; # Reusable search/replacement library, using actual document channels.
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
    (status, text) := files->read(sys, path, 65536); # Validate bytes and close the local FD in the library.
    return (status == files->Ok, text); # A failed read never becomes successful empty text.
}


# Write one acknowledged snapshot completely; return success only after all bytes.
# path/text are values; FD closes on return. This is checked overwrite, not atomic replacement.
writesource(path: string, text: string): int
{
    return files->write(sys, path, text) == files->Ok; # The library validates before creating or overwriting.
}


# Find the next literal match and apply only its acknowledged Unicode selection.
# document/state are exclusively coordinated here; query is a value. Returns the real owner reply.
findnext(document: chan of Idecore->Request, state: ref Ideinput->State, policy: Ideinput,
         query: string): Idecore->Reply
{
    reply := search->next(core, document, state.revision, state.caret, query); # Search is read-only at the owner.
    if (reply.status == core->Ok) { # Select only the matched range from this acknowledged revision.
        if (!policy->select(state, reply, reply.offset, reply.offset + len query)) { # Refuse stale range publication.
            reply.status = core->Conflict; # Source/history remain unchanged.
        }
    }
    return reply; # Status NotFound and all errors stay visible to the caller.
}


# Reject multiline/NUL prompt input and bound its Unicode size before publication.
# text is borrowed; returns one for a valid single-line field, zero otherwise.
promptvalid(text: string): int
{
    if (len text > 256) { # Four-byte Unicode scalars still fit the 1024-byte envelope.
        return 0; # Never truncate a path or search query silently.
    }
    for (i := 0; i < len text; i++) { # A file/search field is an explicit single-line UI value.
        if (text[i] == 0 || text[i] == '\n' || text[i] == '\r') { # No accidental newline command or C-string truncation.
            return 0; # The prior field and document remain unchanged.
        }
    }
    return 1; # Unicode is preserved as scalars, not sliced UTF-8 bytes.
}


# Own graphical editor policy with no Draw/Tk/AS-side document mutation.
# No parameters; channels remain rooted on this stack through the final reply.
init()
{
    sys = load Sys Sys->PATH; # Link the actual native file services.
    core = load Idecore Idecore->PATH; # Link the document/history module.
    codec = load Uibridge Uibridge->PATH; # Link the common channel codec.
    files = load Idefileops Idefileops->PATH; # Link the shared checked file operations.
    search = load Idesearch Idesearch->PATH; # Link renderer-independent document actions.
    bridge = load Idebridge Idebridge->PATH; # Link the existing snapshot encoder.
    policy := load Ideinput Ideinput->PATH; # Link Dis-owned input policy.
    viewport := load Ideviewport Ideviewport->PATH; # Link the bounded logical-line viewport.
    host := load Idehost Idehost->PATH; # Link only the native channel registration boundary.
    if (sys == nil || core == nil || codec == nil || bridge == nil || policy == nil || host == nil || viewport == nil || files == nil || search == nil) { # Require all real modules.
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
    fontsize := 11; # Default size belongs to Dis policy, not native document logic.
    window.rows = host->rows(); # Admit actual font geometry for the initial viewport.
    state.page = window.rows; # Page navigation and visible logical lines use the same units.
    reveal := 1; # Initial presentation follows the acknowledged caret.
    ticket := 0; # Monotonic view identity also distinguishes scrolling within one revision.
    mode := 0; # 0 document, 1 Find, 2 Open, 3 Save As, 4 Replace needle, 5 replacement.
    prompt := ""; # Exclusive single-line field value; never a native document buffer.
    selected := 0; # Ctrl+A marks the complete prompt for replacement by the next text input.
    query := ""; # Last literal query, reused by F3 and replacement.
    dragging := 0; # Only an admitted pointer begin starts a Dis selection gesture.
    for (;;) { # Serialize every input, file operation and document acknowledgement.
        if (ticket == 2147483647) { # Never wrap a view identity onto a stale input.
            raise "fail: graphical editor view ticket exhausted"; # Stop with explicit exhaustion.
        }
        style := ref Uibridge->Message(codec->View, 0, host->Style, reply.revision, fontsize, window.rows, ""); # Publish Dis-owned font/row policy.
        (styleok, stylewire) := codec->encode(style); # Style is a value, never a TextField pointer.
        if (!styleok) { # Invalid style must fail visibly before the following snapshot.
            raise "fail: graphical editor style"; # Never continue with mismatched hit geometry.
        }
        output <-= stylewire; # Native geometry is committed before the matching viewport/snapshot.
        paneltext := prompt; # Active input fields are Dis-owned values.
        if (mode == 0) { # An inactive panel reports the selected document path.
            paneltext = path; # Save As only changes this path after successful output.
            if (len paneltext > 200) { # Bound the label without changing the actual file destination.
                paneltext = paneltext[:200]; # Scalar slicing cannot split UTF-8.
            }
        }
        panel := ref Uibridge->Message(codec->View, 0, host->Panel, reply.revision, mode, selected, paneltext); # Publish prompt state by channel.
        (panelok, panelwire) := codec->encode(panel); # Validate the counted Unicode field.
        if (!panelok) { # Invalid UI state must fail visibly, not mutate native widgets directly.
            raise "fail: graphical editor panel"; # No successful dummy panel response is substituted.
        }
        output <-= panelwire; # Panel and viewport are committed together at the final selection record.
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
        if (event.kind == codec->Request && event.code == host->Zoom) { # Wheel/pinch changes are still requests to Dis.
            expected := array of byte event.text; # Bind geometry to the exact completed view.
            (known, seen) := codec->parseword(expected, 0); # Ticket changes even without a content edit.
            if (!known || len expected != 8 || seen != ticket || event.flags != reply.revision) { # Reject stale gesture geometry.
                reply.status = core->Conflict; # Text, history and current size remain unchanged.
            } else if ((event.first != -1 && event.first != 1) || event.second < 1 || event.second > 128) { # Bound delta and measured rows.
                reply.status = core->RangeError; # Do not accept unbounded native geometry proposals.
            } else { # Apply explicit zoom policy on Dis.
                candidate := fontsize + event.first; # One bounded wheel/pinch step per admitted event.
                if (candidate < 6) { # Keep the minimum usable size explicit.
                    candidate = 6; # Clamp without wrapping a view or font identity.
                }
                if (candidate > 48) { # Avoid unbounded font/layout allocation.
                    candidate = 48; # A gesture at the maximum remains a no-op content-wise.
                }
                fontsize = candidate; # The renderer receives this size only through Style.
                window.rows = event.second; # Native metrics are admitted as copied bounded values.
                state.page = window.rows; # Keep page movement consistent after scaling.
                reply.status = core->Ok; # Zoom never changes content revision or saved state.
                reveal = 1; # Reveal the current caret in the resized logical viewport.
            }
            continue; # A zoom event is never also a text selection or ordinary scroll.
        }
        if (mode != 0) { # Modal field policy executes entirely on Dis.
            if (event.kind == codec->Focus || (event.kind == codec->Key && (event.code == 16 || event.code == 17))) { # Keep modifier/focus state accurate.
                (admitted, observed) := policy->apply(core, document, state, event); # Reuse the production modifier controller.
                if (!admitted) { # Never use malformed input to alter field policy.
                    raise "fail: graphical editor prompt modifier"; # Report a protocol failure.
                }
                reply = observed.document; # Preserve current document metadata.
                if (event.kind == codec->Focus && event.flags == 0) { # Focus loss retires any gesture.
                    dragging = 0; # Returning focus cannot resume a hidden drag.
                }
            } else if (event.kind == codec->Text) { # Committed text edits the field, not the source.
                candidate := prompt + event.text; # Append unless the entire field was selected.
                if (selected) { # Ctrl+A is field-local while a prompt is active.
                    candidate = event.text; # Replace the selected field value as one UI action.
                }
                if (!promptvalid(candidate)) { # Validate before publishing a changed field.
                    reply.status = core->TooLarge; # Preserve prompt, source, history and path.
                } else { # The bounded field is now owned by this controller.
                    prompt = candidate; # No direct TextField mutation occurs here.
                    selected = 0; # Consume the field selection exactly once.
                    reply.status = core->Ok; # Acknowledge accepted field input.
                }
            } else if (event.kind == codec->Key && event.flags != 0) { # Only key-down performs a field action.
                if (event.code == 27) { # Escape cancels without touching document/path/history.
                    mode = 0; # Return to document input policy.
                    prompt = ""; # Release the private field text.
                    selected = 0; # Release any field-local selection.
                    reply.status = core->Ok; # Explicit successful cancellation.
                } else if (event.code == 65 && state.control) { # Ctrl+A selects the complete field.
                    selected = 1; # Document selection remains unchanged.
                } else if (event.code == 8) { # Backspace uses Unicode field coordinates.
                    if (selected) { # Deleting the selected field clears it.
                        prompt = ""; # No source edit or Undo entry is made.
                    } else if (len prompt > 0) { # An empty field cannot underflow.
                        prompt = prompt[:len prompt - 1]; # Remove exactly one scalar.
                    }
                    selected = 0; # The remaining field is no longer selected.
                } else if (event.code == 13) { # Enter executes an explicitly accepted field operation.
                    if (mode == 1) { # Literal Find selects the real acknowledged match.
                        query = prompt; # Retain the user's query for F3.
                        reply = findnext(document, state, policy, query); # Search through the document channel.
                        mode = 0; # Errors/NotFound are visible in the normal status view.
                        reveal = 1; # Reveal a successful match's caret.
                    } else if (mode == 4) { # Replacement is a two-field operation, without delimiter ambiguity.
                        if (prompt == "") { # An empty needle is invalid before any replacement stage.
                            reply.status = core->Invalid; # Keep this field open for correction.
                        } else { # Preserve the exact literal needle.
                            query = prompt; # Unicode/non-BMP text stays unchanged.
                            prompt = ""; # Begin a separately counted replacement field.
                            mode = 5; # The next Enter confirms replacement text, including empty text.
                            selected = 0; # No accidental selection carries into the second field.
                        }
                    } else if (mode == 5) { # ReplaceAll remains one real document-owner operation.
                        reply = search->replace(core, document, reply.revision, query, prompt); # Bounds/history are checked by Idecore.
                        policy->observe(state, reply); # Synchronize selection after an acknowledged edit.
                        mode = 0; # Publish actual match count/status with the resulting snapshot.
                        reveal = 1; # Show the post-operation cursor.
                    } else if (mode == 3) { # Save As adopts its destination only after a successful write.
                        if (writesource(prompt, reply.text)) { # Preserve exact current snapshot bytes.
                            path = prompt; # Subsequent Ctrl+S and reload now use this successful destination.
                            reply = call(document, core->MarkSaved, reply.revision, ""); # Acknowledge that exact revision.
                            mode = 0; # Close the field only after success.
                        } else { # A failed candidate does not replace the current path.
                            reply.status = core->Invalid; # Keep the field open and preserve dirty state.
                        }
                    } else if (mode == 2) { # Open never discards unsaved edits.
                        if (reply.revision != reply.saved) { # Check before touching the candidate file.
                            reply.status = core->Conflict; # Keep the original document and destination.
                        } else { # Read and validate before retiring the current session.
                            (loaded, source) := readsource(prompt); # I/O policy stays on Dis.
                            if (loaded) { # Only validated bytes can replace the document.
                                reply = call(document, core->Close, reply.revision, ""); # Retire old history.
                                reply = call(document, core->Open, reply.revision, source); # Establish a fresh saved revision.
                                path = prompt; # Adopt the successfully loaded path.
                                policy->observe(state, reply); # Reset old source positions.
                                window = viewport->create(); # Retire old scroll coordinates too.
                                window.rows = state.page; # Preserve admitted font geometry.
                                dragging = 0; # Retire old gesture state.
                                mode = 0; # Return to the new document.
                                reveal = 1; # Show its initial caret.
                            } else { # Failed UTF-8/limit/open admission changes no document state.
                                reply.status = core->Invalid; # Leave the field open for correction.
                            }
                        }
                    }
                }
            }
            continue; # A field event is never also applied to the source document.
        }
        if (event.kind == codec->Key && event.flags != 0 && state.control) { # Application shortcuts remain Dis-owned.
            if (event.code == 70 || event.code == 72 || event.code == 79 || (event.code == 83 && state.shift)) { # F/H/O/Shift+S open a field.
                mode = 1; # Ctrl+F starts literal Find.
                prompt = query; # Reuse the previous literal query as editable field text.
                if (event.code == 72) { # Ctrl+H begins the needle stage of Replace All.
                    mode = 4; # Replacement text is accepted in a separate second field.
                }
                if (event.code == 79) { # Ctrl+O selects a source file.
                    mode = 2; # Open retains the current document until successful admission.
                    prompt = path; # Prefill only the editable field, not a native file dialog.
                }
                if (event.code == 83) { # Ctrl+Shift+S chooses a save destination.
                    mode = 3; # Destination adoption is conditional on successful I/O.
                    prompt = path; # Preserve the original path until that acknowledgement.
                }
                selected = 1; # The next committed text replaces the prefilled field.
                dragging = 0; # A modal action retires any document drag gesture.
                reply.status = core->Ok; # Opening a field does not edit source/history.
                continue; # The shortcut must not reach document insertion/navigation.
            }
        }
        if (event.kind == codec->Key && event.flags != 0 && event.code == 114) { # F3 repeats the current literal query.
            reply = findnext(document, state, policy, query); # At most one wrap, always revision checked.
            reveal = 1; # Reveal the acknowledged match when found.
            continue; # Never interpret F3 as text or a second document operation.
        }
        if (event.kind == codec->Request && (event.code == host->Find || event.code == host->SaveAs || event.code == host->Open)) { # Callable actions share the same sequential controller.
            if (event.flags != reply.revision) { # External callers must identify their source snapshot.
                reply.status = core->Conflict; # A late command cannot search/save/open against another revision.
                continue; # Publish the unchanged source and path with a new view identity.
            }
            if (event.code == host->Find) { # Embedding clients can issue the same literal FindNext action.
                query = event.text; # Retain the exact counted query for F3.
                reply = findnext(document, state, policy, query); # No direct selection mutation bypasses the policy.
                reveal = 1; # Follow a successful match.
            } else if (event.code == host->SaveAs) { # An explicit path remains a copied value.
                if (writesource(event.text, reply.text)) { # Invalid/NUL paths fail before native create.
                    path = event.text; # Adopt only the successfully written destination.
                    reply = call(document, core->MarkSaved, reply.revision, ""); # Acknowledge the written snapshot.
                } else { # File errors never mark the document saved.
                    reply.status = core->Invalid; # Preserve old path and dirty state.
                }
            } else if (reply.revision != reply.saved) { # Open must not bypass the dirty guard.
                reply.status = core->Conflict; # Neither source nor destination changes.
            } else { # A clean document may be replaced only after strict source admission.
                (loaded, source) := readsource(event.text); # Same callable file library as the UI field.
                if (loaded) { # A rejected candidate never retires the current document/history.
                    reply = call(document, core->Close, reply.revision, ""); # Stop the old session.
                    reply = call(document, core->Open, reply.revision, source); # Establish the validated source.
                    path = event.text; # Adopt the loaded source path.
                    policy->observe(state, reply); # Retire stale selections and goals.
                    window = viewport->create(); # Retire old source viewport coordinates.
                    window.rows = state.page; # Preserve admitted font geometry.
                    dragging = 0; # Retire old drag state.
                    reveal = 1; # Display the new document start.
                } else { # Read failure is visible and nondestructive.
                    reply.status = core->Invalid; # Keep the original source and path.
                }
            }
            continue; # Do not also dispatch this application action as raw input.
        }
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
