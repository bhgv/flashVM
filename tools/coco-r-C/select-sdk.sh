#!/usr/bin/env bash
# Select Coco/R C frames and generator for the CMake workspace frontend builds.
# This file is sourced; it never downloads tools or modifies the invoking shell's files.


# Select a complete SDK or the bundled source build. Parameters: workspace root,
# build root. Sets COCOR_ROOT, COCOR_EXECUTABLE and need_cocor for the caller.
# COCOR_SOURCE=auto accepts complete overrides and repairs stale ones with a
# warning; bundled ignores overrides; external is strict and never falls back.
# Returns zero on selection, two on explicit configuration errors.
select_cocor_sdk() {
    local sdk_workspace=$1 sdk_build=$2 sdk_mode=${COCOR_SOURCE:-auto}
    local sdk_root=${COCOR_ROOT:-} sdk_executable=${COCOR_EXECUTABLE:-}
    local sdk_frame sdk_valid=1 sdk_reason="" sdk_has_override=0
    case "$sdk_mode" in
        auto | bundled | external) ;;
        *)
            printf 'Coco/R: invalid COCOR_SOURCE=%q; choose auto, bundled or external\n' "$sdk_mode" >&2
            return 2
            ;;
    esac
    if [[ -n "$sdk_root" || -n "$sdk_executable" ]]; then
        sdk_has_override=1
    fi
    if [[ "$sdk_mode" != bundled && ("$sdk_has_override" == 1 || "$sdk_mode" == external) ]]; then
        # Accept the owned wrapper directory as well as its actual upstream root.
        if [[ -n "$sdk_root" && ! -f "$sdk_root/frames/scan_c.frm" && -f "$sdk_root/upstream/frames/scan_c.frm" ]]; then
            sdk_root="$sdk_root/upstream"
        fi
        for sdk_frame in scan_c.frm scan_h.frm parser_c.frm parser_h.frm; do
            if [[ -z "$sdk_root" || ! -f "$sdk_root/frames/$sdk_frame" ]]; then
                sdk_valid=0
                sdk_reason="missing frames/$sdk_frame"
                break
            fi
        done
        if [[ -z "$sdk_executable" || ! -f "$sdk_executable" || ! -x "$sdk_executable" ]]; then
            sdk_valid=0
            sdk_reason="missing or non-executable COCOR_EXECUTABLE"
        fi
        if [[ "$sdk_valid" == 1 ]]; then
            case "$sdk_root" in /*) ;; *) sdk_root="$PWD/$sdk_root" ;; esac
            case "$sdk_executable" in /*) ;; *) sdk_executable="$PWD/$sdk_executable" ;; esac
            COCOR_ROOT=$sdk_root
            COCOR_EXECUTABLE=$sdk_executable
            need_cocor=0
            printf 'Coco/R: external SDK root=%q generator=%q\n' "$COCOR_ROOT" "$COCOR_EXECUTABLE"
            return 0
        fi
        printf 'Coco/R: %s; COCOR_ROOT=%q COCOR_EXECUTABLE=%q\n' \
            "$sdk_reason" "${COCOR_ROOT:-}" "${COCOR_EXECUTABLE:-}" >&2
        if [[ "$sdk_mode" == external ]]; then
            printf 'Coco/R: external mode is strict; repair both SDK paths or select COCOR_SOURCE=bundled\n' >&2
            return 2
        fi
        printf 'Coco/R: ignoring incomplete/stale SDK overrides in auto mode; using bundled sources\n' >&2
    fi
    COCOR_ROOT="$sdk_workspace/tools/coco-r-C/upstream"
    COCOR_EXECUTABLE="$sdk_build/cocor/cocor"
    for sdk_frame in scan_c.frm scan_h.frm parser_c.frm parser_h.frm; do
        if [[ ! -f "$COCOR_ROOT/frames/$sdk_frame" ]]; then
            printf 'Coco/R: bundled SDK is incomplete: %q\n' "$COCOR_ROOT/frames/$sdk_frame" >&2
            return 2
        fi
    done
    need_cocor=1
    printf 'Coco/R: bundled SDK root=%q; generator will be built locally\n' "$COCOR_ROOT"
    return 0
}
