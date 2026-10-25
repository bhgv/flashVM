#!/usr/bin/env bash
# Reproduce both numeric SWFs using source-tree or relocated installed sibling packages.
set -euo pipefail # Stop on any failed compiler/linker stage.
SAMPLES=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) # Locate the actual adjacent inputs.
SOURCE_ROOT=$(CDPATH= cd -- "$SAMPLES/../.." && pwd) # Candidate source workspace or installed component.
if [[ -f "$SOURCE_ROOT/build-workspace.sh" ]] # A source workspace has the canonical builder.
then # Resolve default SDK paths from its local installation.
    PACKAGES="$SOURCE_ROOT/out" # Sibling tools installed by the workspace builder.
    DEFAULT_OUTPUT="$SOURCE_ROOT/build/firststeps" # Keep source fixtures immutable.
else # Installed examples live under <prefix>/<tool>/examples/firststeps.
    PACKAGES=$(CDPATH= cd -- "$SAMPLES/../../.." && pwd) # Resolve the relocated package collection.
    DEFAULT_OUTPUT="$PWD/firststeps-build" # Write only into the caller's chosen working directory.
fi # Finish layout selection without requiring the original checkout.
FIR_SOURCE=${FIR_SOURCE:-"$PACKAGES/fir/fir1_source"} # Explicit override or installed numeric frontend.
FIR_TOOL=${FIR_TOOL:-"$PACKAGES/fir/fir1"} # Explicit override or installed FIR object writer.
SWL_TOOL=${SWL_TOOL:-"$PACKAGES/linker/swl"} # Explicit override or installed SWF linker.
OUTPUT=${FIRSTSTEPS_OUT:-"$DEFAULT_OUTPUT"} # Caller may choose another writable output directory.
mkdir -p "$OUTPUT" # Prepare both generated products and temporary compiler storage.
OUTPUT=$(CDPATH= cd -- "$OUTPUT" && pwd) # Keep TMPDIR absolute even if the caller gave a relative path.
export TMPDIR="$OUTPUT" # FIR temporary staging must also work in restricted hosts.
for example in constant arithmetic # Rebuild both independently readable source programs.
do # Begin one real source/FIR/object/movie pipeline.
    "$FIR_SOURCE" as2 --input "$SAMPLES/$example.as" "$OUTPUT/$example.fir" # Lower the numeric AS2 subset.
    "$FIR_TOOL" check "$OUTPUT/$example.fir" # Verify the generated intermediate representation.
    "$FIR_TOOL" object "$OUTPUT/$example.fir" "$OUTPUT/$example.swo" # Emit the actual relocatable object.
    "$SWL_TOOL" link "$OUTPUT/$example.swf" "$OUTPUT/$example.swo" # Link the matching executable movie.
done # Leave both original checked-in SWFs unchanged.
