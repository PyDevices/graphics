#!/usr/bin/env bash
# Apply CircuitPython pygraphics integration (unix only).
#
# Out-of-tree substitute for Adafruit Extending CircuitPython (no upstream PR):
#   Learn / design-guide step              This script
#   shared-bindings/<mod>/                 copy spike → CP shared-bindings/pygraphics/
#   shared-module/<mod>/                   copy spike → CP shared-module/pygraphics/
#   enable CIRCUITPY_* / mpconfig          CIRCUITPY_PYGRAPHICS in variant mk + mpconfig
#   list sources in port Makefile          variant .mk SRC lists + SRC_PATTERNS
#   build CircuitPython                    caller runs make (or workspace build_cp.sh)
# Conceptual refs:
#   https://learn.adafruit.com/extending-circuitpython
#   https://docs.circuitpython.org/en/latest/docs/design_guide.html
#
# Usage:
#   ./apply_cp_patches.sh --dry-run [--port PORT] [--variant VARIANT]
#   ./apply_cp_patches.sh --apply [--port PORT] [--variant VARIANT]
#   ./apply_cp_patches.sh --status [--port PORT] [--variant VARIANT]
#
# Environment:
#   CP_DIR          CircuitPython tree (default: sibling circuitpython/ next to this repo)
#   WORKSPACE_DIR   Parent of pygraphics (default: parent of this repo)
#   PORT            Must be unix (default: unix); other ports skip with exit 0
#   VARIANT         Unix variant (default: coverage)
#
# Standalone: clone circuitpython + pygraphics as siblings, then:
#   ./apply_cp_patches.sh --apply --port unix --variant coverage
#   cd ../circuitpython/ports/unix && make -j VARIANT=coverage
# No other usermods required.
#
# Migrates legacy ``graphics`` / ``graphics-cmod`` / ``CIRCUITPY_GRAPHICS``
# patches to ``pygraphics`` / ``CIRCUITPY_PYGRAPHICS``.

set -euo pipefail

PYGRAPHICS_MOD_DIR=$(cd "$(dirname "$0")" && pwd)
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$PYGRAPHICS_MOD_DIR/.." && pwd)}"

PORT="${PORT:-unix}"
BOARD="${BOARD:-}"
VARIANT="${VARIANT:-coverage}"
MODE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dry-run|--apply|--status|--force-apply) MODE="$1"; shift ;;
        --port)    PORT="$2"; shift 2 ;;
        --board)   BOARD="$2"; shift 2 ;;
        --variant) VARIANT="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1 (try --help)" >&2
            exit 1
            ;;
    esac
done

MODE="${MODE:---dry-run}"
case "$MODE" in
    --force-apply) MODE=--apply ;;
    --dry-run|--apply|--status) ;;
    *)
        echo "Unknown mode: $MODE (try --help)" >&2
        exit 1
        ;;
esac

# Resolve CircuitPython: CP_DIR, else sibling circuitpython/ under WORKSPACE_DIR.
if [[ -n "${CP_DIR:-}" && -d "${CP_DIR}/ports" ]]; then
    CP_DIR=$(cd "$CP_DIR" && pwd)
elif [[ -d "$WORKSPACE_DIR/circuitpython/ports" ]]; then
    CP_DIR=$(cd "$WORKSPACE_DIR/circuitpython" && pwd)
else
    CP_DIR=""
fi

SPIKE_DIR="$PYGRAPHICS_MOD_DIR/src/circuitpython_spike"
SPIKE_MANIFEST="$SPIKE_DIR/copy_manifest.txt"

MARKER_TAG="pygraphics-cmod begin"
MARKER_BEGIN="# >>> $MARKER_TAG"
MARKER_END="# >>> pygraphics-cmod end"

DRY_RUN=0
# Nonzero if any dry-run check finds something --apply could not do.
DRY_RC=0
APPLY=0
case "$MODE" in
    --dry-run) DRY_RUN=1 ;;
    --apply) APPLY=1 ;;
esac

log() { echo "$*"; }

die() { echo "error: $*" >&2; exit 1; }

if [[ -z "${CP_DIR:-}" ]] || [[ ! -d "$CP_DIR/ports" ]]; then
    die "CircuitPython not found (set CP_DIR, or place circuitpython/ next to this repo under $WORKSPACE_DIR)."
fi
CP_DIR=$(cd "$CP_DIR" && pwd)
[[ -f "$SPIKE_MANIFEST" ]] || die "Spike manifest missing: $SPIKE_MANIFEST"

PORT_DIR="$CP_DIR/ports/$PORT"
# The "enable" makefile differs by port: unix builds select features per
# variant, MCU ports per board. Everything else this script touches
# (circuitpy_mpconfig.mk, circuitpy_defns.mk, the port Makefile) is generic.
if [[ "$PORT" == unix ]]; then
    ENABLE_MK="$PORT_DIR/variants/$VARIANT/mpconfigvariant.mk"
else
    [[ -n "$BOARD" ]] || die "non-unix ports need --board (got port=$PORT)"
    ENABLE_MK="$PORT_DIR/boards/$BOARD/mpconfigboard.mk"
fi
PORT_MK="$PORT_DIR/Makefile"
MPCONFIG_MK="$CP_DIR/py/circuitpy_mpconfig.mk"
DEFNS_MK="$CP_DIR/py/circuitpy_defns.mk"
PYGRAPHICS_MOD_REL=$(python3 -c "import os; print(os.path.relpath('$PYGRAPHICS_MOD_DIR', '$PORT_DIR'))")

for f in "$ENABLE_MK" "$PORT_MK" "$MPCONFIG_MK" "$DEFNS_MK"; do
    [[ -f "$f" ]] || die "missing: $f"
done

remove_marked_blocks() {
    local file="$1"
    shift
    local tag
    [ -f "$file" ] || return 0
    for tag in "$@"; do
        if ! grep -qF "$tag" "$file" 2>/dev/null; then
            continue
        fi
        if [ "$DRY_RUN" = 1 ]; then
            log "  [dry-run] remove marked block '$tag' from $file"
            continue
        fi
        python3 - "$file" "$tag" <<'PY'
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
tag = re.escape(sys.argv[2])
text = path.read_text()
# Match begin…end with either "graphics-cmod" or "pygraphics-cmod" style ends.
end = r"(?:graphics-cmod|pygraphics-cmod|graphics_native-cmod) end"
pat = rf"\n?# >>> {tag}\n.*?\n# >>> {end}\n?"
text2, n = re.subn(pat, "\n", text, count=0, flags=re.DOTALL)
if n:
    path.write_text(text2)
PY
        log "  removed marked block '$tag' from $file"
    done
}

replace_in_file() {
    local file="$1" old="$2" new="$3"
    [ -f "$file" ] || return 0
    if ! grep -qF "$old" "$file" 2>/dev/null; then
        return 0
    fi
    if [ "$DRY_RUN" = 1 ]; then
        log "  [dry-run] replace in $file: ${old//$'\t'/\\t} -> ${new//$'\t'/\\t}"
        return 0
    fi
    python3 - "$file" "$old" "$new" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
old, new = sys.argv[2], sys.argv[3]
text = path.read_text()
if old not in text:
    sys.exit(0)
# Do not rewrite ``pygraphics/...`` when replacing legacy ``graphics/...``.
out = []
i = 0
while True:
    j = text.find(old, i)
    if j < 0:
        out.append(text[i:])
        break
    # Skip if this ``graphics`` is the suffix of ``pygraphics``.
    if old.lstrip("\t").startswith("graphics/") and j >= 2 and text[j - 2 : j] == "py":
        # find() landed on the ``graphics`` inside ``pygraphics`` — advance past it.
        out.append(text[i : j + len(old)])
        i = j + len(old)
        continue
    out.append(text[i:j])
    out.append(new)
    i = j + len(old)
path.write_text("".join(out))
PY
    log "  replaced in $file: graphics → pygraphics path"
}

insert_raw_after_line() {
    local file="$1" needle="$2" raw="$3" already="$4"
    if grep -qF "$already" "$file" 2>/dev/null; then
        return 0
    fi
    if [ "$DRY_RUN" = 1 ]; then
        if grep -qF "$needle" "$file" 2>/dev/null; then
            log "  [dry-run] insert after needle in $file: $already"
        else
            # The apply path fails here when the needle is gone. The dry run
            # used to print the same line either way, so a moved upstream
            # anchor - the one thing a pin move must stop on - was invisible
            # until the apply.
            log "  [dry-run] ERROR: needle not found in $file: $needle"
            DRY_RC=1
        fi
        return 0
    fi
    python3 - "$file" "$needle" "$raw" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
needle = sys.argv[2]
raw = sys.argv[3]
text = path.read_text()
if raw in text:
    sys.exit(0)
if needle not in text:
    print(f"needle not found in {path}: {needle!r}", file=sys.stderr)
    sys.exit(1)
path.write_text(text.replace(needle, needle + "\n" + raw, 1))
PY
}

ensure_block_append() {
    local file="$1" marker_grep="$2" block="$3"
    if grep -qF "$marker_grep" "$file" 2>/dev/null; then
        return 0
    fi
    if [ "$DRY_RUN" = 1 ]; then
        log "  [dry-run] append $marker_grep block to $file"
        return 0
    fi
    # Keep a blank line before the block; printf %s avoids $(…) eating newlines.
    printf '\n%s\n' "$block" >> "$file"
    log "  appended $marker_grep block to $file"
}

insert_block_before_needle() {
    local file="$1" needle="$2" block="$3" already="$4"
    if grep -qF "$already" "$file" 2>/dev/null; then
        return 0
    fi
    if [ "$DRY_RUN" = 1 ]; then
        if grep -qF "$needle" "$file" 2>/dev/null; then
            log "  [dry-run] insert block before needle in $file"
        else
            # The apply path fails here when the needle is gone. The dry run
            # used to print the same line either way, so a moved upstream
            # anchor - the one thing a pin move must stop on - was invisible
            # until the apply.
            log "  [dry-run] ERROR: needle not found in $file: $needle"
            DRY_RC=1
        fi
        return 0
    fi
    python3 - "$file" "$needle" "$block" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
needle = sys.argv[2]
block = sys.argv[3]
if not block.endswith("\n"):
    block += "\n"
text = path.read_text()
if "CIRCUITPY_PYGRAPHICS),1)" in text or "SRC_PATTERNS += pygraphics/%" in text:
    sys.exit(0)
if needle not in text:
    raise SystemExit(f"needle not found in {path}")
path.write_text(text.replace(needle, block + needle, 1))
PY
    log "  inserted SRC_PATTERNS block into $file"
}

copy_spike() {
    if [ "$DRY_RUN" = 1 ]; then
        log "  [dry-run] copy spike from $SPIKE_DIR"
        return 0
    fi
    while read -r dest src; do
        [[ -z "${dest:-}" ]] && continue
        mkdir -p "$CP_DIR/$dest"
        cp "$SPIKE_DIR/$dest/$src" "$CP_DIR/$dest/$src"
    done < "$SPIKE_MANIFEST"
    log "  copied pygraphics spike into $CP_DIR"
}

# Sets STATUS_RC=1 if anything is not in the state --apply would leave behind:
# a pygraphics tree missing, or a legacy graphics tree still STALE. It used to
# report the defect and exit 0, so a caller that checked $? read a broken tree
# as a good one - found by planting a fault in the pin move's step-0 rehearsal,
# 2026-09-09.
STATUS_RC=0
status_report() {
    echo "CP_DIR=$CP_DIR"
    echo "VARIANT=$VARIANT"
    echo
    [[ -f $CP_DIR/shared-bindings/pygraphics/__init__.c ]] || STATUS_RC=1
    [[ -f $CP_DIR/shared-module/pygraphics/__init__.c ]] || STATUS_RC=1
    [[ -f $CP_DIR/shared-bindings/graphics/__init__.c ]] && STATUS_RC=1
    [[ -f $CP_DIR/shared-module/graphics/__init__.c ]] && STATUS_RC=1
    printf '  %-40s %s\n' "shared-bindings/pygraphics" \
        "$( [[ -f $CP_DIR/shared-bindings/pygraphics/__init__.c ]] && echo present || echo MISSING )"
    printf '  %-40s %s\n' "shared-module/pygraphics" \
        "$( [[ -f $CP_DIR/shared-module/pygraphics/__init__.c ]] && echo present || echo MISSING )"
    printf '  %-40s %s\n' "legacy shared-bindings/graphics" \
        "$( [[ -f $CP_DIR/shared-bindings/graphics/__init__.c ]] && echo STALE || echo gone )"
    printf '  %-40s %s\n' "legacy shared-module/graphics" \
        "$( [[ -f $CP_DIR/shared-module/graphics/__init__.c ]] && echo STALE || echo gone )"
    echo
    grep -nE 'CIRCUITPY_(PY)?GRAPHICS|pygraphics-cmod|graphics-cmod|shared-bindings/(py)?graphics|SRC_PATTERNS \+= (py)?graphics' \
        "$ENABLE_MK" "$PORT_MK" "$MPCONFIG_MK" "$DEFNS_MK" 2>/dev/null || true
}

if [ "$MODE" = --status ]; then
    status_report
    exit $STATUS_RC
fi

log "Applying pygraphics CP patches (mode=$MODE)…"

# Drop obsolete / renamed spike stubs.
if [ "$DRY_RUN" = 1 ]; then
    log "  [dry-run] rm -rf shared-bindings/{graphics,pygraphics_native} shared-module/{graphics,pygraphics_native}"
else
    rm -rf \
        "$CP_DIR/shared-bindings/graphics" \
        "$CP_DIR/shared-module/graphics" \
        "$CP_DIR/shared-bindings/pygraphics_native" \
        "$CP_DIR/shared-module/pygraphics_native" \
        "$CP_DIR/shared-bindings/graphics_native" \
        "$CP_DIR/shared-module/graphics_native"
    log "  removed legacy graphics / graphics_native spike dirs"
fi

copy_spike

# Strip legacy and current marked blocks so we can rewrite cleanly.
for f in "$ENABLE_MK" "$PORT_MK" "$MPCONFIG_MK" "$DEFNS_MK"; do
    remove_marked_blocks "$f" \
        "graphics-cmod begin" \
        "graphics_native-cmod begin" \
        "pygraphics-cmod begin"
done

# Rewrite any leftover path references outside marked blocks.
# Only match legacy ``graphics/`` paths (not the ``graphics`` suffix of ``pygraphics``).
if [[ "$PORT" == unix ]]; then
    replace_in_file "$ENABLE_MK" $'shared-bindings/graphics/__init__.c \\' $'shared-bindings/pygraphics/__init__.c \\'
    replace_in_file "$ENABLE_MK" $'shared-module/graphics/__init__.c \\' $'shared-module/pygraphics/__init__.c \\'
fi
replace_in_file "$DEFNS_MK" $'\tgraphics/__init__.c \\' $'\tpygraphics/__init__.c \\'

# Variant enable + include circuitpython.mk (relpath works for any sibling layout)
ensure_block_append "$ENABLE_MK" "pygraphics-cmod begin" "$(cat <<EOF
# >>> pygraphics-cmod begin
CIRCUITPY_PYGRAPHICS = 1
CFLAGS += -DCIRCUITPY_PYGRAPHICS=1
PYGRAPHICS_MOD_DIR := \$(abspath $PYGRAPHICS_MOD_REL)
include \$(PYGRAPHICS_MOD_DIR)/circuitpython.mk
# >>> pygraphics-cmod end
EOF
)"

# Module source lists (unix only: MCU ports resolve these through
# SRC_PATTERNS += pygraphics/% in circuitpy_defns.mk).
if [[ "$PORT" == unix ]]; then
    if grep -qF $'shared-bindings/lvgl/__init__.c \\' "$ENABLE_MK"; then
        insert_raw_after_line "$ENABLE_MK" $'shared-bindings/lvgl/__init__.c \\' \
            $'\tshared-bindings/pygraphics/__init__.c \\' \
            "shared-bindings/pygraphics/__init__.c"
        insert_raw_after_line "$ENABLE_MK" $'shared-module/lvgl/__init__.c \\' \
            $'\tshared-module/pygraphics/__init__.c \\' \
            "shared-module/pygraphics/__init__.c"
    elif grep -qF $'shared-bindings/usdl2/__init__.c \\' "$ENABLE_MK"; then
        insert_raw_after_line "$ENABLE_MK" $'shared-bindings/usdl2/__init__.c \\' \
            $'\tshared-bindings/pygraphics/__init__.c \\' \
            "shared-bindings/pygraphics/__init__.c"
        insert_raw_after_line "$ENABLE_MK" $'shared-module/usdl2/__init__.c \\' \
            $'\tshared-module/pygraphics/__init__.c \\' \
            "shared-module/pygraphics/__init__.c"
    else
        insert_raw_after_line "$ENABLE_MK" $'shared-bindings/jpegio/JpegDecoder.c \\' \
            $'\tshared-bindings/pygraphics/__init__.c \\' \
            "shared-bindings/pygraphics/__init__.c"
        insert_raw_after_line "$ENABLE_MK" $'shared-module/jpegio/JpegDecoder.c \\' \
            $'\tshared-module/pygraphics/__init__.c \\' \
            "shared-module/pygraphics/__init__.c"
    fi
fi

ensure_block_append "$PORT_MK" "pygraphics-cmod begin" "$(cat <<EOF
# >>> pygraphics-cmod begin
ifneq (\$(wildcard \$(abspath $PYGRAPHICS_MOD_REL/circuitpython.mk)),)
PYGRAPHICS_MOD_DIR ?= \$(abspath $PYGRAPHICS_MOD_REL)
include \$(PYGRAPHICS_MOD_DIR)/circuitpython.mk
endif
# >>> pygraphics-cmod end
EOF
)"

ensure_block_append "$MPCONFIG_MK" "CIRCUITPY_PYGRAPHICS ?=" "$(cat <<'EOF'
# >>> pygraphics-cmod begin
CIRCUITPY_PYGRAPHICS ?= 0
CFLAGS += -DCIRCUITPY_PYGRAPHICS=$(CIRCUITPY_PYGRAPHICS)
# >>> pygraphics-cmod end
EOF
)"

insert_block_before_needle "$DEFNS_MK" "ifeq (\$(CIRCUITPY_MATH),1)" "$(cat <<'EOF'
# >>> pygraphics-cmod begin
ifeq ($(CIRCUITPY_PYGRAPHICS),1)
SRC_PATTERNS += pygraphics/%
endif
# >>> pygraphics-cmod end
EOF
)" "SRC_PATTERNS += pygraphics/%"

if grep -qF $'\tlvgl/__init__.c \\' "$DEFNS_MK"; then
    insert_raw_after_line "$DEFNS_MK" $'\tlvgl/__init__.c \\' \
        $'\tpygraphics/__init__.c \\' \
        $'pygraphics/__init__.c'
elif grep -qF $'\tusdl2/__init__.c \\' "$DEFNS_MK"; then
    # Insert before usdl2 so alpha-ish order stays reasonable when both present.
    insert_raw_after_line "$DEFNS_MK" $'\tjpegio/JpegDecoder.c \\' \
        $'\tpygraphics/__init__.c \\' \
        $'pygraphics/__init__.c'
else
    insert_raw_after_line "$DEFNS_MK" $'\tjpegio/JpegDecoder.c \\' \
        $'\tpygraphics/__init__.c \\' \
        $'pygraphics/__init__.c'
fi

if [ "$APPLY" = 1 ] || [ "$DRY_RUN" = 1 ]; then
    log "pygraphics CP patches applied"
fi

if [ "$DRY_RUN" = 1 ] && [ "$DRY_RC" != 0 ]; then
    log "Dry run reported at least one ERROR above."
    exit "$DRY_RC"
fi

if [ "$DRY_RUN" = 0 ]; then
    echo
    status_report
    if [ "$APPLY" = 1 ]; then
        echo
        log "Next:"
        log "  cd $PORT_DIR && make -j VARIANT=$VARIANT"
        log "For an easier way to build with other extensions, see the org's optional aggregator workspace: https://github.com/PyDevices/cmods"
    fi
fi
