"""Generate native-cell Sys declarations; unsupported host calls fail explicitly."""

import argparse
from pathlib import Path
import re
import subprocess
import sys
from prepare_type_counts import adapt as adapt_type_counts
from prepare_heap_counts import adapt as adapt_heap_counts
from prepare_opcode_counts import adapt as adapt_opcode_counts
from prepare_module_counts import adapt_link, adapt_load, adapt_xec
from prepare_snapshots import adapt_string, adapt_xec as adapt_xec_snapshots
from prepare_observations import adapt_runt, adapt_xec as adapt_xec_observations
from prepare_jit_counts import adapt as adapt_jit_counts
from prepare_record_new import adapt as adapt_record_new, adapt_leaf as adapt_record_leaf
from prepare_channel_new import adapt as adapt_channel_new, adapt_wait_completion, adapt_alt_completion, adapt_alt_preflight, adapt_alt_reservations, adapt_channel_finalizer, adapt_queue_holds, adapt_buffer_reservation, adapt_channel_operand_replay, adapt_alt_result_guards, adapt_alt_header_replay, adapt_alt_selection_capture
from prepare_mapped_cons import adapt as adapt_mapped_cons
from prepare_raw_cons import adapt as adapt_raw_cons
from prepare_scalar_cons import adapt_xec as adapt_scalar_xec, adapt_emitter as adapt_scalar_emitter
from prepare_pointer_builder import adapt as adapt_pointer_builder
from prepare_pointer_tail import adapt as adapt_pointer_tail
from prepare_owner_tail import adapt as adapt_owner_tail, adapt_arm64 as adapt_arm64_owner_tail
from prepare_aggregate_tail import adapt as adapt_aggregate_tail
from prepare_roots import adapt as adapt_roots
from prepare_boundary_counts import adapt as adapt_boundary_counts
from prepare_stack_check import adapt as adapt_stack_check
from prepare_shared_pointer import adapt as adapt_shared_pointer
from prepare_shared_bytes import adapt as adapt_shared_bytes
from prepare_shared_mapped import adapt as adapt_shared_mapped
from prepare_shared_head import adapt as adapt_shared_head
from prepare_shared_list_pointer import adapt as adapt_shared_list_pointer
from prepare_shared_shift import adapt as adapt_shared_shift
from prepare_shared_branch import adapt as adapt_shared_branch
from prepare_shared_array_length import adapt as adapt_shared_array_length
from prepare_shared_real_conversion import adapt as adapt_shared_real_conversion
from prepare_shared_real_arithmetic import adapt as adapt_shared_real_arithmetic
from prepare_shared_real_branch import adapt as adapt_shared_real_branch
from prepare_shared_string_branch import adapt as adapt_shared_string_branch
from prepare_shared_string_length import adapt as adapt_shared_string_length
from prepare_shared_list_length import adapt as adapt_shared_list_length
from prepare_shared_arithmetic import adapt as adapt_shared_arithmetic
from prepare_shared_integer import adapt as adapt_shared_integer
from prepare_shared_list_bytes import adapt as adapt_shared_list_bytes
from prepare_bytes_tail import adapt as adapt_bytes_tail
from prepare_gc_fields import adapt as adapt_gc_fields, validate_header as validate_gc_header


# Apply exactly one reviewed relative-sequence source adaptation; reject drift.
# source/before/after are owned strings, count is the required single occurrence.
# Return replaced text or raise before publishing an incomplete generated view.
def adapt_array_slice_captures(code):
    """Keep the nil-source shortcut and guard only pre-effect slice inputs."""
    start = code.index('OP(slicea)')
    end = code.index('OP(slicela)', start)
    body = code[start:end]
    capture = ('\tmemmove(&v, DVM_R.m, sizeof(v));\n'
               '\tmemmove(&start, DVM_R.s, sizeof(start));\n'
               '\tmemmove(&ds, slot, sizeof(ds));')
    body = relative_replace(body, capture, """    if(dvm_runtime_guarded_copies) {
        int replay;
        const char *problem = dvm_runtime_bytes_snapshot_operands(ISLICEA, DVM_R.m, DVM_R.s,
            sizeof(v), slot, &v, &start, &ds, &replay);
        if(replay) {
            return;
        }
        if(problem) {
            error((char *)problem);
        }
    } else {
"""+capture+"""
    }""", 1)
    code = code[:start] + body + code[end:]
    start = code.index('OP(slicela)')
    end = code.index('\n}', start)
    body = code[start:end]
    capture = '\tmemmove(&ss, DVM_R.s, sizeof(ss));'
    body = relative_replace(body, capture, """    if(dvm_runtime_guarded_copies) {
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_attempt(DVM_R.s, &ss, sizeof(ss), &claim);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ISLICELA)) {
            return;
        }
        if(problem) {
            error((char *)problem);
        }
    } else {
"""+capture+"""
    }""", 1)
    capture = '\tmemmove(&dl, DVM_R.m, sizeof(dl));\n\tmemmove(&ds, DVM_R.d, sizeof(ds));'
    body = relative_replace(body, capture, """    if(dvm_runtime_guarded_copies) {
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_pair_attempt(DVM_R.m, sizeof(dl), &dl,
            DVM_R.d, sizeof(ds), &ds, &claim);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ISLICELA)) {
            return;
        }
        if(problem) {
            error((char *)problem);
        }
    } else {
"""+capture+"""
    }""", 1)
    return code[:start] + body + code[end:]


def adapt_array_slice_publication(code):
    """Guard narrowing a view and commit a slice assignment as one transaction."""
    start = code.index('OP(slicea)')
    end = code.index('OP(slicela)', start)
    body = code[start:end]
    update = '\t\tds->len = n;\n\t\tds->data = data;'
    guarded = """        if(dvm_runtime_guarded_copies && !relative) {
            DVM_RangeAccess access = {0};
            DVM_CellResult claim = dvm_managed_ranges_begin(ds, sizeof(*ds), slot, sizeof(ds), &access);
            Array *observed;
            if(claim == DVM_CELL_ACQUIRED) {
                memmove(&observed, slot, sizeof(observed));
                if(observed == ds) {
                    ds->len = n;
                    ds->data = data;
                } else {
                    claim = DVM_CELL_BUSY;
                }
                if(!dvm_range_access_end(&access)) {
                    abort();
                }
            }
            if(claim == DVM_CELL_ACQUIRED ||
               (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ISLICEA))) {
                return;
            }
            error(exHeap);
        }
"""
    body = relative_replace(body, update, guarded+update, 1)
    prepare = """typedef struct DvmRelativeSlice {
    Array *source; /* Borrowed original array under NativeWorld. */
    Array *result; /* Owned private copy or final view until publication. */
    WORD start; /* Validated normalized lower bound. */
    WORD count; /* Validated result length. */
    int rooted; /* Temporary root protecting the copy during view allocation. */
} DvmRelativeSlice;


/* Prepare a relative slice using caller-owned request storage. Return one on
 * success. The enclosing kprotect handler owns partial cleanup on VM errors. */
static int dvm_prepare_relative_slice(void *argument)
{
    DvmRelativeSlice *request = argument;
    Array *copy = arraycpy(request->source);
    Array *view;
    Type *type = copy->t;
    uchar *data;
    request->result = copy;
    data = arrayspan(copy, type, request->start, request->count);
    if(type->np == 0) {
        copy->len = request->count;
        copy->data = data;
        return 1;
    }
    if(dvm_refcount_load_i(&type->ref) > 0x7ffffffe - (type == &Tarray) ||
       dvm_refcount_load_i(&Tarray.ref) <= 0 || dvm_refcount_load_i(&Tarray.ref) == 0x7fffffff) {
        error(exHeap);
    }
    ptradd(D2H(copy));
    request->rooted = 1;
    view = H2D(Array*, heap(&Tarray));
    view->len = request->count;
    view->data = data;
    view->t = type;
    view->root = copy;
    dvm_runtime_type_retain(&type->ref);
    ptrdel(D2H(copy));
    request->rooted = 0;
    request->result = view;
    return 1;
}


/* Build a private result for the live old owner, then replace its slot once.
 * Errors and finalization occur outside range guards. BUSY replays before commit. */
static void dvm_publish_relative_slice(Array *old, void *slot, WORD start, WORD count)
{
    DvmRelativeSlice request = {old, H, start, count, 0};
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    Array *observed;
    if(!kprotect(dvm_prepare_relative_slice, &request)) {
        if(request.rooted) {
            ptrdel(D2H(request.result));
        }
        destroy(request.result);
        krethrow();
        return;
    }
    claim = dvm_managed_ranges_begin(NULL, 0, slot, sizeof(request.result), &access);
    if(claim == DVM_CELL_ACQUIRED) {
        memmove(&observed, slot, sizeof(observed));
        if(observed == old) {
            memmove(slot, &request.result, sizeof(request.result));
        } else {
            claim = DVM_CELL_BUSY;
        }
        if(!dvm_range_access_end(&access)) {
            abort();
        }
    }
    if(claim != DVM_CELL_ACQUIRED) {
        destroy(request.result);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ISLICEA)) {
            return;
        }
        error(exHeap);
    }
    if(request.result->root != H) {
        Setmark(D2H(request.result->root));
    }
    destroy(old);
}


"""
    body = relative_replace(body, '\tif(relative){\n\t\tcopy = arraycpy(ds);',
        """    if(relative && dvm_runtime_guarded_copies) {
        dvm_publish_relative_slice(ds, slot, start, n);
        return;
    }
\tif(relative){
\t\tcopy = arraycpy(ds);""", 1)
    initialize = '\tss = H2D(Array*, h);'
    body = relative_replace(body, initialize, initialize+"""
    if(dvm_runtime_guarded_copies && !relative) {
        DVM_RangeAccess access = {0};
        DVM_CellResult claim;
        Array *observed;
        int backed = ds->root != H;

        /* A refused view owns its type but has no elements or backing owner. */
        ss->len = 0;
        ss->data = nil;
        ss->t = t;
        ss->root = H;
        dvm_runtime_type_retain(&t->ref);
        claim = dvm_managed_ranges_begin(NULL, 0, slot, sizeof(ss), &access);
        if(claim == DVM_CELL_ACQUIRED) {
            memmove(&observed, slot, sizeof(observed));
            if(observed == ds) {
                h = D2H(root);
                if(backed) {
                    dvm_runtime_heap_retain(&h->ref);
                }
                ss->len = n;
                ss->data = data;
                ss->root = root;
                memmove(slot, &ss, sizeof(ss));
            } else {
                claim = DVM_CELL_BUSY;
            }
            if(!dvm_range_access_end(&access)) {
                abort();
            }
        }
        if(claim != DVM_CELL_ACQUIRED) {
            destroy(ss);
            if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ISLICEA)) {
                return;
            }
            error(exHeap);
        }
        if(backed) {
            destroy(ds);
        }
        Setmark(h);
        return;
    }""", 1)
    code = code[:start]+prepare+body+code[end:]
    start = code.index('OP(slicela)')
    end = code.index('\n}', start)
    body = code[start:end]
    anchor = '\tif(t->np == 0) {'
    body = relative_replace(body, anchor, """    if(dvm_runtime_guarded_copies) {
        dvm_runtime_array_slice_copy(source, destination, t, (size_t)ss->len, ss, ds);
        return;
    }
"""+anchor, 1)
    return code[:start]+body+code[end:]


def adapt_string_conversion_captures(code, conversions):
    """Replace pre-effect operand reads in the selected conversion bodies.

    The source and conversion descriptions are borrowed. Return adapted source
    or fail generation if the pinned reads change. NativeWorld still owns the
    captured objects and the effects of formatting, parsing and publication.
    """
    for opcode, operand, paired in conversions:
        start = code.index('OP('+opcode.lower()+')')
        end = code.index('\n}', start)
        body = code[start:end]
        read = f'\tmemmove(&{operand}, DVM_R.s, sizeof({operand}));'
        old_read = '\tmemmove(&old, slot, sizeof(old));'
        bit_copy = '\tmemmove(&bits, &value, sizeof(bits));'
        if paired:
            capture = read+'\n'
            if opcode == 'CVTFC':
                capture += bit_copy+'\n'
            capture += old_read
            replacement = f"""    if(dvm_runtime_guarded_copies) {{
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_pair_attempt(DVM_R.s, sizeof({operand}), &{operand},
            slot, sizeof(old), &old, &claim);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
            return;
        }}
        if(problem) {{
            error((char *)problem);
        }}
    }} else {{
{capture}
    }}"""
            if opcode == 'CVTFC':
                replacement = replacement.replace('    } else {', bit_copy+'\n    } else {', 1)
        else:
            # One union covers aliasing source and destination slots from
            # capture through publication. NativeWorld still owns the string.
            parser = {'CVTCW': 'strtol(view, nil, 10)', 'CVTCL': 'strtoll(view, nil, 10)',
                      'CVTCF': 'strtod(view, nil)'}[opcode]
            capture = read+'\n\tvalue = '+parser.replace('view', 'string2c(s)')+';\n'
            capture += '\tmemmove(DVM_R.d, &value, sizeof(value));'
            replacement = f"""    if(dvm_runtime_guarded_copies) {{
        DVM_RangeAccess access = {{0}};
        DVM_CellResult claim = dvm_managed_ranges_begin(DVM_R.s, sizeof(s), DVM_R.d, sizeof(value), &access);
        const char *problem = NULL;
        char *view;
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
            return;
        }}
        if(claim != DVM_CELL_ACQUIRED) {{
            error(exHeap);
        }}
{read}
        view = dvm_runtime_string_view(s, &problem);
        if(!problem) {{
            value = {parser};
            memmove(DVM_R.d, &value, sizeof(value));
        }}
        if(!dvm_range_access_end(&access)) {{
            abort();
        }}
        if(problem) {{
            error((char *)problem);
        }}
        return;
    }}
{capture}"""
        body = relative_replace(body, capture, replacement, 1)
        code = code[:start] + body + code[end:]
    return code


def adapt_numeric_string_publication(code):
    """Guard the formatted bytes and owner slot before either can change.

    NativeWorld keeps the borrowed old string and private allocation alive.
    A failed claim discards only a fresh unpublished object before retry.
    """
    for opcode in ('CVTWC', 'CVTLC', 'CVTFC'):
        start = code.index('OP('+opcode.lower()+')')
        end = code.index('\n}', start)
        body = code[start:end]
        mutation = body[body.index('\tif(ds == old && old->len < 0)'):]
        guarded = f"""    if(dvm_runtime_guarded_copies) {{
        size_t span = offsetof(String, data)+(size_t)length+1;
        DVM_CellResult claim = dvm_managed_ranges_begin(ds, span, slot, sizeof(ds), &access);
        String *observed;
        if(claim != DVM_CELL_ACQUIRED) {{
            if(ds != old) {{
                destroy(ds);
            }}
            if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
        memmove(&observed, slot, sizeof(observed));
        if(observed != old) {{
            if(!dvm_range_access_end(&access)) {{
                abort();
            }}
            if(ds != old) {{
                destroy(ds);
            }}
            if(dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
    }}
"""
        # One scope owns the claim while the shared body updates bytes and slot.
        release = """    if(dvm_runtime_guarded_copies && !dvm_range_access_end(&access)) {
        abort();
    }
"""
        mutation = relative_replace(mutation, '\tif(ds != old)\n\t\tdestroy(old);',
                                    release+'\tif(ds != old)\n\t\tdestroy(old);', 1)
        body = body[:body.index('\tif(ds == old && old->len < 0)')] + guarded + mutation
        locals = '\tvoid *slot;\n    DVM_RangeAccess access = {0};'
        if opcode == 'CVTFC':
            locals += '\n    fenv_t format_environment;'
            capture = """    if(dvm_runtime_guarded_copies && fegetenv(&format_environment)) {
        abort();
    }
"""
            body = relative_replace(body, '\texponent = (bits>>52)&2047;', capture+'\texponent = (bits>>52)&2047;', 1)
            for condition in ('if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ICVTFC))',
                              'if(dvm_runtime_scalar_retry(ICVTFC))'):
                # Restore rounding and flags even when the retry limit is exhausted.
                body = relative_replace(body, '        '+condition, """        if(fesetenv(&format_environment)) {
            abort();
        }
        """+condition, 1)
        body = relative_replace(body, '\tvoid *slot;', locals, 1)
        code = code[:start]+body+code[end:]
    return code


def adapt_byte_string_publication(code):
    """Protect byte/string destination effects and discard only unpublished owners."""
    for opcode, result, width, reused in (
        ('CVTCA', 'a', 'l', 'l ? target.data : NULL'),
        ('CVTAC', 'ds', 'n', 'n ? (void *)(address+offsetof(String, data)) : NULL'),
    ):
        start = code.index('OP('+opcode.lower()+')')
        end = code.index('\n}', start)
        body = code[start:end]
        slot = '\tmemmove(slot, &'+result+', sizeof('+result+'));'
        publish = f"""    if(dvm_runtime_guarded_copies) {{
        DVM_RangeAccess access = {{0}};
        DVM_CellResult claim = dvm_managed_ranges_begin(NULL, 0, slot, sizeof({result}), &access);
        void *observed;
        if(claim == DVM_CELL_ACQUIRED) {{
            memmove(&observed, slot, sizeof(observed));
            if(observed == old) {{
                memmove(slot, &{result}, sizeof({result}));
            }} else {{
                claim = DVM_CELL_BUSY;
            }}
            if(!dvm_range_access_end(&access)) {{
                abort();
            }}
        }}
        if(claim != DVM_CELL_ACQUIRED) {{
            destroy({result});
            if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
    }} else {{
{slot}
    }}"""
        body = relative_replace(body, slot, publish, 1)
        if opcode == 'CVTCA':
            before = '\t\t\t\t\tmemmove(target.data, bytes, l);\n\t\t\t\t}'
            body = relative_replace(body, before, '\t\t\t\t}', 1)
            entry = '\t\t\t\ttarget.len = l;'
            header = '(uchar*)old+offsetof(Array, len)'
            header_width = 'sizeof(target.len)'
            copy = 'memmove(target.data, bytes, l);'
        else:
            entry = '\t\t\t\t\t\tif(n != 0)'
            header = '(uchar*)old+offsetof(String, len)'
            header_width = 'sizeof(n)'
            copy = None
        claim = f"""    DVM_RangeAccess reuse_access = {{0}};
    if(dvm_runtime_guarded_copies) {{
        DVM_CellResult result = dvm_managed_three_begin({reused}, {width},
            {header}, {header_width}, slot, sizeof(old), &reuse_access);
        void *observed;
        if(result == DVM_CELL_ACQUIRED) {{
            memmove(&observed, slot, sizeof(observed));
            if(observed != old) {{
                if(!dvm_range_access_end(&reuse_access)) {{
                    abort();
                }}
                result = DVM_CELL_BUSY;
            }}
        }}
        if(result != DVM_CELL_ACQUIRED) {{
            if(result == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
    }}
"""
        if copy:
            claim += f'    if({width} != 0) {{\n        {copy}\n    }}\n'
        body = relative_replace(body, entry, claim+entry, 1)
        reuse_slot = '\tmemmove(slot, &old, sizeof(old));'
        body = relative_replace(body, reuse_slot, reuse_slot+"""
    if(dvm_runtime_guarded_copies && !dvm_range_access_end(&reuse_access)) {
        abort();
    }""", 1)
        code = code[:start]+body+code[end:]
    return code


def adapt_string_edit_publication(code):
    """Guard fresh string publication and in-place character and slice edits."""
    for opcode in ('INSC', 'SLICEC', 'ADDC'):
        start = code.index('OP('+opcode.lower()+')')
        end = code.index('\n}', start)
        body = code[start:end]
        publish = '\tmemmove(slot, &result, sizeof(result));'
        guarded = f"""    if(dvm_runtime_guarded_copies) {{
        DVM_RangeAccess access = {{0}};
        DVM_CellResult claim = dvm_managed_ranges_begin(NULL, 0, slot, sizeof(result), &access);
        String *observed;
        if(claim == DVM_CELL_ACQUIRED) {{
            memmove(&observed, slot, sizeof(observed));
            if(observed == old) {{
                memmove(slot, &result, sizeof(result));
            }} else {{
                claim = DVM_CELL_BUSY;
            }}
            if(!dvm_range_access_end(&access)) {{
                abort();
            }}
        }}
        if(claim != DVM_CELL_ACQUIRED) {{
            if(result != old) {{
                destroy(result);
            }}
            if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
    }} else {{
{publish}
    }}"""
        body = relative_replace(body, publish, guarded, 1)
        if opcode != 'ADDC':
            if opcode == 'INSC':
                entry = '\t\tif(wide)\n\t\t\told->Srune[index] = rune;'
                payload = '(void *)(address+bytes-width)'
                size = 'width'
                prefix = '    size_t width = wide ? sizeof(Rune) : 1;\n'
            else:
                entry = '\t\t\tif(old->len < 0){\n\t\t\t\tif(start != 0)'
                payload = 'start != 0 ? (void *)address : NULL'
                size = 'start != 0 ? (size_t)bytes : 0'
                prefix = ''
            reuse = prefix+f"""    DVM_RangeAccess edit = {{0}};
    if(dvm_runtime_guarded_copies) {{
        DVM_CellResult claim = dvm_managed_three_begin({payload}, {size},
            &old->len, sizeof(old->len), slot, sizeof(old), &edit);
        String *observed;
        if(claim == DVM_CELL_ACQUIRED) {{
            memmove(&observed, slot, sizeof(observed));
            if(observed != old) {{
                if(!dvm_range_access_end(&edit)) {{
                    abort();
                }}
                claim = DVM_CELL_BUSY;
            }}
        }}
        if(claim != DVM_CELL_ACQUIRED) {{
            if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(I{opcode})) {{
                return;
            }}
            error(exHeap);
        }}
    }}
"""
            release = """    if(dvm_runtime_guarded_copies && !dvm_range_access_end(&edit)) {
        abort();
    }
"""
            pos = body.index(entry)
            done = body.index('return;', pos)
            line = body.rindex('\n', pos, done)+1
            body = body[:pos]+reuse+body[pos:line]+release+body[line:]
        code = code[:start]+body+code[end:]
    return code


def adapt_string_append_reuse(code):
    """Keep the public addstring helper and guard opcode-only in-place append."""
    start = code.index('String*\naddstring(')
    end = code.index('\n}', start)+2
    body = code[start:end]
    body = relative_replace(body, 'String*\naddstring(String *s1, String *s2, int append)',
        'static String*\ndvm_string_append(String *s1, String *s2, int append, void *slot)', 1)
    body = relative_replace(body, '\tRune *r;', '\tRune *r;\n    DVM_RangeAccess access = {0};\n    int claimed = 0;', 1)
    anchor = '\n\tif(s1->len < 0) {'
    claim = """
    if(slot && append && l <= s1->max && !(s1->len >= 0 && s2->len < 0)) {
        size_t width = s1->len < 0 ? sizeof(Rune) : 1;
        DVM_CellResult result = dvm_managed_three_begin((void *)address, (size_t)l*width,
            &s1->len, sizeof(s1->len), slot, sizeof(s1), &access);
        String *observed;
        if(result == DVM_CELL_ACQUIRED) {
            memmove(&observed, slot, sizeof(observed));
            if(observed != s1) {
                if(!dvm_range_access_end(&access)) {
                    abort();
                }
                result = DVM_CELL_BUSY;
            }
        }
        if(result != DVM_CELL_ACQUIRED) {
            if(result == DVM_CELL_BUSY && dvm_runtime_scalar_retry(IADDC)) {
                return nil;
            }
            error(exHeap);
        }
        claimed = 1;
    }
"""
    body = relative_replace(body, anchor, claim+anchor, 1)
    for length, tabs in (('-l', '\t\t'), ('l', '\t')):
        line = 'ns->len = '+length+';'
        tail = '\n\n'+tabs+'return ns;'
        body = relative_replace(body, line+tail, line+"""
        if(claimed && !dvm_range_access_end(&access)) {
            abort();
        }"""+tail, 1)
    wrapper = """

String*
addstring(String *s1, String *s2, int append)
{
    return dvm_string_append(s1, s2, append, NULL);
}"""
    code = code[:start]+body+wrapper+code[end:]
    call = 'result = addstring(left, right, DVM_R.m == DVM_R.d);'
    return relative_replace(code, call, """result = dvm_string_append(left, right, DVM_R.m == DVM_R.d,
        dvm_runtime_guarded_copies ? slot : NULL);
    if(result == nil) {
        return;
    }""", 1)


def adapt_string_view(code):
    """Share the pinned UTF-8 view body between throwing and guarded callers."""
    start = code.index('char*\nstring2c(String *s)')
    end = code.index('\n}', start)+2
    body = code[start:end]
    body = relative_replace(body, 'string2c(String *s)', 'dvm_runtime_string_view(String *s, const char **problem)', 1)
    body, count = re.subn(r'^(\t+)if([^\n]*?)\n\1\terror\((ex\w+)\);',
                         r'\1if\2 {\n\1\t*problem = \3;\n\1\treturn nil;\n\1}', body, flags=re.M)
    if count != 8 or 'error(' in body:
        raise ValueError('UTF-8 view error boundaries changed')
    wrapper = """

char*
string2c(String *s)
{
    const char *problem = NULL;
    char *view = dvm_runtime_string_view(s, &problem);
    if(problem) {
        error((char *)problem);
    }
    return view;
}"""
    return code[:start] + body + wrapper + code[end:]


def adapt_string_operand_captures(code):
    """Preserve decoded preflight and retry only before string mutation."""
    for opcode, first, second, first_cell, second_cell in (
        ('INSC', 'value', 'index', 'DVM_R.s', 'DVM_R.m'),
        ('SLICEC', 'start', 'end', 'DVM_R.s', 'DVM_R.m'),
        ('ADDC', 'left', 'right', 'DVM_R.m', 'DVM_R.s'),
    ):
        start = code.index('OP('+opcode.lower()+')')
        end = code.index('\n}', start)
        body = code[start:end]
        reads = (
            f'\tmemmove(&{first}, {first_cell}, sizeof({first}));\n'
            f'\tmemmove(&{second}, {second_cell}, sizeof({second}));\n'
            '\tmemmove(&old, slot, sizeof(old));'
        )
        if opcode == 'INSC':
            old = reads.replace('\n\tmemmove(&index', '\n\trune = value;\n\tmemmove(&index')
            old = old.replace('\n\tmemmove(&old', '\n\tslot = DVM_R.d;\n\tmemmove(&old')
            before = '    slot = DVM_R.d;\n'
            after = '\n    rune = value;'
        else:
            old, before, after = reads, '', ''
        replacement = before + f"""    if(dvm_runtime_guarded_copies) {{
        int replay;
        const char *problem = dvm_runtime_bytes_snapshot_operands(I{opcode}, {first_cell}, {second_cell},
            sizeof({first}), slot, &{first}, &{second}, &old, &replay);
        if(replay) {{
            return;
        }}
        if(problem) {{
            error((char *)problem);
        }}
    }} else {{
{reads}
    }}""" + after
        code = code[:start] + relative_replace(body, old, replacement, 1) + code[end:]
    return code


def relative_replace(source, before, after, count=1):
    if count != 1 or source.count(before) != 1:
        raise ValueError("relative sequence boundary changed: " + before[:80])
    return source.replace(before, after, 1)


# Generate paired headers from the same compiler used for native-cell modules.
# CLI supplies compiler, Sys interface and owned build output. Returns zero on success.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--sys", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--idehost", type=Path)
    parser.add_argument("--fircompiler", type=Path)
    parser.add_argument("--native-demo", type=Path)
    parser.add_argument("--native-bindings", type=Path)
    args = parser.parse_args()
    subprocess.run(
        [sys.executable, str(Path(__file__).with_name("prepare_exception.py")),
         "--source", str(args.source.parent / "emu/port/exception.c"),
         "--output", str(args.output / "exception.c")], check=True)
    # Always start from pinned source, making repeated preparation byte-identical.
    subprocess.run(
        [
            sys.executable,
            str(Path(__file__).resolve().parents[1] / "host/prepare_native_registers.py"),
            "--source",
            str(args.source),
            "--output",
            str(args.output),
        ],
        check=True,
    )
    layout_path = args.output / "leaf64-ops.h"
    layout_path.write_text(adapt_record_leaf(adapt_jit_counts(layout_path.read_text())))
    emitter_path = args.output / "amd64-emit.h"
    emitter_path.write_text(adapt_scalar_emitter(adapt_bytes_tail(adapt_aggregate_tail(adapt_owner_tail(adapt_pointer_tail(emitter_path.read_text()))))))
    arm64_emitter = args.output / "arm64-emit.h"
    arm64_emitter.write_text(adapt_arm64_owner_tail(arm64_emitter.read_text()))
    for unit in ('loader', 'exception'):
        path = args.output / (unit+'.c')
        path.write_text(adapt_boundary_counts(path.read_text(), unit))
    stack_path = args.output / "validstk.c"
    stack_path.write_text(adapt_stack_check(stack_path.read_text()))
    roots_path = args.output / "gc.c"
    roots_path.write_text(adapt_roots(roots_path.read_text()))
    # Adapt only the hosted view. Pinned upstream and standalone native fixtures
    # retain ordinary destroy. Reject drift before installing the owner hook.
    heap_path = args.output / "heap.c"
    heap_text = heap_path.read_text()
    rc_anchor = "\tif(--h->ref > 0 || gchalt > 64)"
    if heap_text.count(rc_anchor) != 1:
        raise ValueError("destroy ownership boundary changed; review RC adapter")
    heap_text = heap_text.replace(rc_anchor,
        "\tif(dvm_runtime_rc_defer(v))\n\t\treturn;\n\n" + rc_anchor, 1)
    heap_path.write_text('#include "runtime_rc.h"\n' + adapt_heap_counts(adapt_type_counts(heap_text)))
    text = subprocess.check_output([args.compiler, "-a", "-t", "Sys", args.sys], text=True)
    marker = "typedef struct{char *name;"
    at = text.index(marker)
    srv = subprocess.check_output(
        [args.compiler, "-a", "-t", "Srv", str(Path(args.sys).with_name("srv.m"))], text=True
    )
    srv_at = srv.index(marker)
    loader = subprocess.check_output(
        [args.compiler, "-a", "-t", "Loader", str(Path(args.sys).with_name("loader.m"))], text=True
    )
    loader_at = loader.index(marker)
    (args.output / "runt.h").write_text(text[:at] + srv[:srv_at] + loader[:loader_at])
    (args.output / "loadermod.h").write_text(loader[loader_at:])
    if args.idehost is not None:
        host = subprocess.check_output(
            [args.compiler, "-a", "-t", "Idehost", str(args.idehost)], text=True
        )
        host_at = host.index(marker)
        with (args.output / "runt.h").open("a") as declarations:
            declarations.write(host[:host_at])
        (args.output / "idehostmod.h").write_text(host[host_at:])
    if args.fircompiler is not None:
        compiler = subprocess.check_output(
            [args.compiler, "-a", "-t", "Fircompiler", str(args.fircompiler)], text=True)
        compiler_at = compiler.index(marker)
        with (args.output / "runt.h").open("a") as declarations:
            declarations.write(compiler[:compiler_at])
        (args.output / "fircompilermod.h").write_text(compiler[compiler_at:])
    if args.native_demo is not None:
        if args.native_bindings is None:
            raise ValueError("native module tutorial requires our Coco/R limbo_bindings")
        subprocess.run([str(args.native_bindings), "--format", "native-c-nc64", str(args.native_demo),
                        str(args.output / "nativedemo_native.h")], check=True)
    # Generate hosted-only scalar and table declarations for the native cell model.
    for module in ("Bigtable", "Processctl"):
        interface = Path(__file__).resolve().parents[1] / f"module/{module.lower()}.m"
        generated = subprocess.check_output(
            [args.compiler, "-a", "-t", module, str(interface)], text=True
        )
        table_at = generated.index(marker)
        with (args.output / "runt.h").open("a") as declarations:
            declarations.write(generated[:table_at])
        (args.output / f"{module.lower()}mod.h").write_text(generated[table_at:])
    math = subprocess.check_output(
        [args.compiler, "-a", "-t", "Math", str(Path(args.sys).with_name("math.m"))], text=True
    )
    math_at = math.index(marker)
    with (args.output / "runt.h").open("a") as declarations:
        declarations.write(math[:math_at])
    (args.output / "mathmod.h").write_text(math[math_at:])
    math_code = (args.source / "math.c").read_text()
    # This hosted build deliberately uses native libm for scalar IEEE operations.
    math_code = math_code.replace('#include "lib9.h"', '#include "lib9.h"\n#include <math.h>')
    math_code = math_code.replace("__ieee754_", "")
    (args.output / "math.c").write_text(math_code)
    (args.output / "srvmod.h").write_text(srv[srv_at:])
    srv_code = (args.source.parent / "emu/port/srv.c").read_text()
    srv_code = srv_code.replace('#include\t"ip.h"', '#include "runt.h"')
    srv_code = srv_code.replace('#include\t"srv.h"', '#include "kernel.h"')
    srv_code = srv_code.replace(
        '#include\t"srvm.h"',
        '#include "srvmod.h"\nextern void dvm_srv_lock(void);\nextern void dvm_srv_unlock(void);\nextern char *dvm_srv_string(String *);\nextern int so_gethostbyname(char *, char **, int);\nextern int so_gethostbyaddr(char *, char **, int);\nextern int so_getservbyname(char *, char *, char *);',
    )
    srv_code = srv_code.replace("static\tQLock\tdbq;", "")
    srv_code = srv_code.replace("qlock(&dbq)", "dvm_srv_lock()").replace(
        "qunlock(&dbq)", "dvm_srv_unlock()"
    )
    srv_code = srv_code.replace("string2c(f->", "dvm_srv_string(f->")
    (args.output / "srv.c").write_text(srv_code)
    (args.output / "sysmod.h").write_text(text[at:])
    functions = set(re.findall(r"void (Sys_\w+)\(void\*\);", text))
    implemented = {
        "Sys_millisec",
        "Sys_sleep",
        "Sys_print",
        "Sys_fprint",
        "Sys_werrstr",
        "Sys_sprint",
        "Sys_aprint",
        "Sys_tokenize",
        "Sys_utfbytes",
        "Sys_byte2char",
        "Sys_char2byte",
        "Sys_open",
        "Sys_write",
        "Sys_read",
        "Sys_readn",
        "Sys_fildes",
        "Sys_create",
        "Sys_seek",
        "Sys_stat",
        "Sys_fstat",
        "Sys_dirread",
    }
    source = '#include "lib9.h"\n#include "kernel.h"\nextern void error(char *);\n'
    for name in sorted(functions - implemented):
        source += "\n/* Reject an unavailable host service without reporting success. */\n"
        source += "void " + name + "(void *frame)\n{\n    USED(frame);\n"
        source += '    error("unsupported native host service: ' + name + '");\n}\n'
    (args.output / "unsupported.c").write_text(source)
    for name in ("string.c", "runt.c"):
        path = args.output / name
        code = path.read_text()
        legacy_byte_guard = "a->t->size != 1 || a->t->np != 0" in code
        staged_byte_guard = (name in ("string.c", "runt.c") and "if(view.t != &Tbyte)" in code
                             and "if(size != 1 || pointers != 0)" in code
                             and "if(owner.t != &Tarray)" in code)
        if not legacy_byte_guard and not staged_byte_guard:
            raise ValueError("upstream byte descriptor validation changed; review owner preflight: " + name)
        if name == "runt.c":
            code = code.replace("\tint i;\n", "\tWORD i;\n", 1)
            code = code.replace(
                "Dis integer arguments occupy four bytes, even on LP64 hosts.",
                "NC64 arguments occupy the explicitly selected native cell width.",
            )
        path.write_text(adapt_runt(code) if name == "runt.c" else code)
    # Relative indexes are host language extensions, kept outside the snapshot.
    for filename in ("xec.c", "string.c"):
        path = args.output / filename
        code = path.read_text()
        code = relative_replace(code, '#include "interp.h"', '#include "interp.h"\n#include <stdint.h>', 1)
        code = relative_replace(code, '#include "interp.h"', '#include "interp.h"\n#include "managed_owner.h"\n#include "shared_access.h"\n#include "shared_string_branch.h"', 1)
        # Insert helpers after all includes/macros but before the first operation.
        point = code.index("OP(indc)" if filename == "string.c" else "OP(runt)")
        code = code[:point] + '/* RELATIVE_SEQUENCE_HELPERS */\n' + code[point:]
        helpers = Path(__file__).with_name("relative_sequence.c").read_text()
        code = relative_replace(code, "/* RELATIVE_SEQUENCE_HELPERS */", helpers)
        if filename == "xec.c":
            code = relative_replace(code, 'array == H || array == nil || index < 0 ||', 'array == H || array == nil ||', 1)
            needle = 'memmove(&length, (uchar*)array+offsetof(Array, len), sizeof(length));'
            if code.count(needle) != 1:
                raise ValueError("array indexing preflight changed")
            code = relative_replace(code, needle, needle + '\n\tindex = dvm_relative_index(length, index);', 1)
            # A relative-bound array slice copies first, then applies the original
            # validated view machinery to that independent backing owner.
            start = code.index('OP(slicea)')
            finish = code.index('OP(slicela)', start)
            part = code[start:finish]
            part = relative_replace(part, 'WORD v, n, start;', 'WORD v, n, start;\n\tint relative;\n\tArray *copy;')
            part = relative_replace(part, 'if(ds == H || ds == nil){',
                'relative = start < 0 || v < 0;\n\tif(ds == H && start == 0 && v == -1) return;\n\tif(ds == H || ds == nil){', 1)
            part = relative_replace(part, 'if(start < 0 || v < start || v > ds->len)',
                'if(relative) dvm_relative_bounds(ds->len, &start, &v);\n\tif(start < 0 || v < start || v > ds->len)', 1)
            needle = 'data = arrayspan(ds, t, start, n);'
            if part.count(needle) != 1:
                raise ValueError("array slice preflight changed")
            part = relative_replace(part, needle, needle + '\n\tif(relative){\n\t\tcopy = arraycpy(ds);\n\t\tmemmove(DVM_R.d, &copy, sizeof(copy));\n\t\tdestroy(ds);\n\t\tds = copy;\n\t\tt = ds->t;\n\t\tdata = arrayspan(ds, t, start, n);\n\t}', 1)
            code = code[:start] + part + code[finish:]
            code = adapt_array_slice_captures(code)
            code = adapt_array_slice_publication(code)
            code = adapt_string_conversion_captures(code, (('CVTCL', 's', False),))
            needle = 'if(dl < 0 || dl > ds->len || ss->len < 0 || ss->len > ds->len-dl)'
            if code.count(needle) != 1:
                raise ValueError("array slice-store boundary changed")
            code = relative_replace(code, needle, 'dl = dvm_relative_index(ds->len, dl);\n\t' + needle, 1)
        else:
            needle = 'if(index < 0 || index >= length)'
            if code.count(needle) != 1:
                raise ValueError("string index boundary changed")
            code = relative_replace(code, needle, 'index = dvm_relative_index(length, index);\n\t' + needle, 1)
            point = code.index("OP(indc)")
            capture = "\tmemmove(&index, DVM_R.m, sizeof(index));"
            offset = code.index(capture, point)
            code = code[:offset] + (
                "    if(dvm_runtime_guarded_copies) {\n"
                "        const char *problem = dvm_runtime_string_index(DVM_R.s, DVM_R.m, DVM_R.d);\n"
                "        if(problem) {\n"
                "            error((char *)problem);\n"
                "        }\n"
                "        return;\n"
                "    }\n"
            ) + code[offset:]
            code = '#include "shared_bytes.h"\n#include "shared_string_branch.h"\n' + code
            needle = 'if(index < 0 || index > length)'
            if code.count(needle) != 1:
                raise ValueError("string store boundary changed")
            code = relative_replace(code, needle, 'index = dvm_relative_index(length, index);\n\t' + needle, 1)
            code = adapt_string_operand_captures(code)
            code = adapt_string_edit_publication(code)
            code = adapt_string_append_reuse(code)
            code = adapt_numeric_string_publication(code)
            code = adapt_byte_string_publication(code)
            code = '#include <fenv.h>\n' + code
            code = adapt_string_view(code)
            code = adapt_string_conversion_captures(code, (
                ('CVTCA', 'ss', True), ('CVTAC', 'a', True),
                ('CVTWC', 'value', True), ('CVTLC', 'value', True), ('CVTFC', 'value', True),
                ('CVTCW', 's', False), ('CVTCF', 's', False),
            ))
            start = code.index('OP(slicec)')
            finish = code.index('void\ncvtup', start)
            part = code[start:finish]
            part = relative_replace(part, 'if(start < 0 || end < start || (uvlong)end > 0x7fffffff)',
                'if(start >= 0 && end >= 0 && (end < start || (uvlong)end > 0x7fffffff))', 1)
            part = relative_replace(part, 'if(end > length)', 'dvm_relative_bounds(length, &start, &end);\n\t\tif(end > length)', 1)
            part = relative_replace(part, 'result = slicer((ulong)start, (ulong)end, old);',
                'if(old == H) dvm_relative_bounds(0, &start, &end);\n\tresult = slicer((ulong)start, (ulong)end, old);', 1)
            code = code[:start] + part + code[finish:]
        path.write_text(adapt_shared_string_length(adapt_string(code)) if filename == "string.c" else code)
    path = args.output / "load.c"
    code = path.read_text().replace("*(WORD*)si = disw(isp);", "*(WORD*)si = (int)disw(isp);")
    path.write_text(adapt_load(code))
    link_path = args.output / "link.c"
    link_path.write_text(adapt_link(link_path.read_text()))
    path = args.output / "xec.c"
    code = path.read_text()
    code = relative_replace(code, '#include "interp.h"', '#include "interp.h"\n#include "table_leases.h"\n'
        'extern void Processctl_killwait(void *frame);\n'
        'extern int dvm_processctl_call_ready(void *frame);')
    start = code.index('OP(mcall)')
    stop = code.index('\nOP(', start + len('OP(mcall)'))
    body = code[start:stop]
    body = relative_replace(body,
        '\tmemmove((uchar*)f+offsetof(Frame, lr), &DVM_R.PC, sizeof(f->lr));',
        '\t/* Host lease gate runs before call-frame/module publication. */\n'
        '\tif(program == nil && (!dvm_bigtable_call_ready(entry.u.runt, f) ||\n'
        '\t   (entry.u.runt == Processctl_killwait && !dvm_processctl_call_ready(f)))){\n'
        '\t\treturn;\n'
        '\t}\n'
        '\tmemmove((uchar*)f+offsetof(Frame, lr), &DVM_R.PC, sizeof(f->lr));')
    code = code[:start] + body + code[stop:]

    # Use the callback captured by checked fetch; never reread a Jit after enter.
    code = relative_replace(code, "if(fast(&DVM_R))", "if(dvm_runtime_fast(op, fast, &DVM_R))")
    # Complete entries must retain the same NC64 table policy and fast-only counts.
    fast_boundary = "if(fast != nil && fast(r))"
    if code.count(fast_boundary) != 1:
        raise ValueError("complete JIT executor changed; review NC64 fast policy")
    code = code.replace(fast_boundary, "if(fast != nil && dvm_runtime_fast(op, fast, r))")
    boundary = "\t\t/* Successful translated steps own their register/PC publication. */"
    if code.count(boundary) != 1:
        raise ValueError("instruction boundary changed; review runtime quota/count placement")
    # No configured quota needs no external call: keep the ordinary count inline.
    # Both arms still admit exactly one instruction before either execution path.
    dispatch = (
        "\t\tif(dvm_runtime_instruction_limit) {\n"
        "\t\t\tdvm_runtime_step();\n"
        "\t\t} else {\n"
        "\t\t\tdvm_runtime_instructions++;\n"
        "\t\t}\n"
    )
    dispatch = (
        "\t\tif(!dvm_runtime_instruction_limit && dvm_runtime_shared_integer) {\n"
        "\t\t\tint completed = dvm_runtime_integer_batch(fetched.module, fetched.pc, fetched.index);\n"
        "\t\t\tif(completed) {\n"
        "\t\t\t\tdvm_runtime_instructions += completed;\n"
        "\t\t\t\t(void)dvm_runtime_arithmetic_finish(p);\n"
        "#ifdef DIS_NATIVE64_CALLER_GUARD\n"
        "\t\t\t\twhile(--completed > 0) native64calleradvance(p);\n"
        "#endif\n"
        "\t\t\t\tif(p->flags & Pkilled) dvm_runtime_termination_checkpoint(p);\n"
        "\t\t\t\tif(DVM_R.IC == 0) break;\n"
        "\t\t\t\tcontinue;\n"
        "\t\t\t}\n"
        "\t\t}\n"
    ) + dispatch
    code = code.replace(boundary, dispatch + boundary, 1)
    termination = "executed:\n\t\tif(!vmyield())"
    if code.count(termination) != 1:
        raise ValueError("managed termination boundary drift")
    decode = "\t\tvmdecode(&fetched);"
    if code.count(decode) != 1:
        raise ValueError("decoded retry boundary drift")
    code = code.replace(decode, decode +
        "\n\t\tif(dvm_runtime_guarded_copies || op == IMOVP || op == ISELF) {\n"
        "\t\t\tdvm_native_execution()->arithmetic_retry_opcode = op;\n"
        "\t\t}")
    code = code.replace(termination,
        "executed:\n"
        "\t\tif((dvm_runtime_guarded_copies || op == IMOVP || op == ISELF) && dvm_runtime_arithmetic_finish(p)) {\n"
        "\t\t\tDVM_R.PC = fetched.pc;\n"
        "\t\t\tDVM_R.s = DVM_R.m = DVM_R.d = nil;\n"
        "\t\t\tdvm_runtime_instructions--;\n"
        "\t\t\tbreak;\n"
        "\t\t}\n"
        "\t\t/* Commit changes to the root geometry before the next safepoint. */\n"
        "\t\tif(p->R.FP != DVM_R.FP || p->R.SP != DVM_R.SP ||\n"
        "\t\t   p->R.EX != DVM_R.EX || p->R.TS != DVM_R.TS ||\n"
        "\t\t   p->R.M != DVM_R.M || p->R.MP != DVM_R.MP)\n"
        "\t\t\tp->R = DVM_R;\n"
        "\t\tif(p->flags & Pkilled)\n"
        "\t\t\tdvm_runtime_termination_checkpoint(p);\n\t\tif(!vmyield())", 1)
    code = code.replace(
        '#include "interp.h"', '#include "interp.h"\n'
        '#include "disvm/native_process.h"\n'
        'extern void dvm_runtime_termination_checkpoint(Prog *process);', 1)
    # Classic upstream rows are fixed-width; NC64 keeps its explicitly private cell layout.
    adapters = Path(__file__).with_name("native_tables.c").read_text()
    for operation in ("icase", "casel", "igoto"):
        pattern = r"OP\(" + operation + r"\)\n\{[\s\S]*?\n\}"
        if len(re.findall(pattern, code)) != 1:
            raise ValueError("numeric table boundary changed: " + operation)
        code = re.sub(
            pattern,
            lambda match: "/* NC64_TABLE_ADAPTERS */" if operation == "icase" else "",
            code,
            count=1,
        )
    code = code.replace("/* NC64_TABLE_ADAPTERS */", adapters)
    start = code.index('OP(casec)')
    end = code.index('\n}', start)
    body = relative_replace(code[start:end], '\tmemmove(&sv, DVM_R.s, sizeof(sv));', """    if(!nc64_table_selector(ICASEC, &sv, sizeof(sv))) {
        return;
    }""", 1)
    code = code[:start]+body+code[end:]
    capture = '\tmemmove(&s, DVM_R.s, sizeof(s));\n\tmemmove(&d, DVM_R.d, sizeof(d));'
    start = code.index('OP(tcmp)')
    end = code.index('\n}', start)
    body = relative_replace(code[start:end], capture, """    if(dvm_runtime_guarded_copies) {
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_pair_attempt(DVM_R.s, sizeof(s), &s,
            DVM_R.d, sizeof(d), &d, &claim);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ITCMP)) {
            return;
        }
        if(problem) {
            error(exTcheck);
        }
    } else {
"""+capture+"""
    }""", 1)
    code = code[:start]+body+code[end:]
    code = adapt_xec_observations(adapt_xec_snapshots(adapt_xec(adapt_opcode_counts(code))))
    code = adapt_buffer_reservation(adapt_queue_holds(adapt_wait_completion(adapt_channel_new(adapt_record_new(adapt_mapped_cons(adapt_raw_cons(adapt_scalar_xec(adapt_pointer_builder(code)))))))))
    path.write_text(adapt_channel_operand_replay(code))
    path = args.output / "alt.c"
    code = path.read_text()
    code = code.replace(
        "uvlong bytes;\n\n\tif(a == nil",
        "uvlong bytes;\n\tWORD sends, receives;\n\n\tif(a == nil",
        1,
    )
    code = code.replace(
        "memmove(nsend, (uchar*)a+offsetof(Alt, nsend), sizeof(*nsend));",
        "memmove(&sends, (uchar*)a+offsetof(Alt, nsend), sizeof(sends));",
    )
    code = code.replace(
        "memmove(nrecv, (uchar*)a+offsetof(Alt, nrecv), sizeof(*nrecv));",
        "memmove(&receives, (uchar*)a+offsetof(Alt, nrecv), sizeof(receives));\n"
        "\tif(sends < 0 || receives < 0 || sends > 0x7fffffff || receives > 0x7fffffff) return 0;\n"
        "\t*nsend = sends;\n\t*nrecv = receives;",
    )
    code = adapt_alt_result_guards(adapt_alt_reservations(adapt_alt_preflight(adapt_alt_completion(code))))
    path.write_text(adapt_alt_selection_capture(adapt_alt_header_replay(code)))

    pointer_path = args.output / "xec.c"
    hosted = pointer_path.read_text()
    for adapter in (adapt_shared_pointer, adapt_shared_bytes, adapt_shared_mapped,
                    adapt_shared_head, adapt_shared_list_pointer, adapt_shared_list_bytes,
                    adapt_shared_integer, adapt_shared_arithmetic, adapt_shared_shift, adapt_shared_branch, adapt_shared_string_branch, adapt_shared_real_branch, adapt_shared_real_arithmetic, adapt_shared_real_conversion, adapt_shared_array_length, adapt_shared_list_length):
        hosted = adapter(hosted)
    pointer_path.write_text(hosted)

    validate_gc_header((args.source.parent / 'include/interp.h').read_text())
    for unit in ('gc', 'heap'):
        path = args.output / (unit+'.c')
        code = adapt_gc_fields(path.read_text(), unit)
        if unit == "heap":
            code = adapt_channel_finalizer(code)
            anchor = "\tif(dvm_runtime_rc_defer(v))"
            if code.count(anchor) != 1:
                raise ValueError("hosted cycle loss boundary drift")
            code = '#include "runtime_gc.h"\n' + code.replace(
                anchor, "\tdvm_runtime_gc_owner_lost();\n" + anchor, 1)
        path.write_text(code)


if __name__ == "__main__":
    main()
