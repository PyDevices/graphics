#!/usr/bin/env bash
# Apply CircuitPython graphics integration (unix/coverage by default).
set -euo pipefail

GRAPHICS_MOD_DIR=$(cd "$(dirname "$0")" && pwd)
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$GRAPHICS_MOD_DIR/.." && pwd)}"
CP_DIR="${CP_DIR:-$WORKSPACE_DIR/circuitpython}"
PORT="${PORT:-unix}"
VARIANT="${VARIANT:-coverage}"
SPIKE_DIR="$GRAPHICS_MOD_DIR/circuitpython_spike"

[[ -d "$CP_DIR/.git" ]] || { echo "CircuitPython not found: $CP_DIR" >&2; exit 1; }

insert_after_line() {
    local file="$1" needle="$2" block="$3"
    if grep -qF "$needle" "$file" && ! grep -qF "graphics-cmod" "$file" 2>/dev/null; then
        :
    fi
    if grep -qF "$(printf '%s' "$block" | head -1)" "$file" 2>/dev/null; then
        return 0
    fi
    python3 - "$file" "$needle" "$block" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
needle = sys.argv[2]
block = sys.argv[3]
text = path.read_text()
if block.splitlines()[0] in text:
    sys.exit(0)
if needle not in text:
    print(f"needle not found in {path}: {needle!r}", file=sys.stderr)
    sys.exit(1)
text = text.replace(needle, needle + block, 1)
path.write_text(text)
PY
}

insert_raw_after_line() {
    local file="$1" needle="$2" raw="$3"
    if grep -qF "shared-bindings/graphics/__init__.c" "$file" 2>/dev/null || grep -qF $'\tgraphics/__init__.c \\' "$file" 2>/dev/null; then
        return 0
    fi
    python3 - "$file" "$needle" "$raw" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
needle = sys.argv[2]
raw = sys.argv[3]
text = path.read_text()
if "graphics/__init__.c" in text:
    sys.exit(0)
if needle not in text:
    print(f"needle not found in {path}: {needle!r}", file=sys.stderr)
    sys.exit(1)
text = text.replace(needle, needle + "\n" + raw, 1)
path.write_text(text)
PY
}

copy_spike() {
    while read -r dest src; do
        [[ -z "$dest" ]] && continue
        mkdir -p "$CP_DIR/$dest"
        cp "$SPIKE_DIR/$dest/$src" "$CP_DIR/$dest/$src"
    done < "$SPIKE_DIR/copy_manifest.txt"
}

VARIANT_MK="$CP_DIR/ports/$PORT/variants/$VARIANT/mpconfigvariant.mk"
PORT_MK="$CP_DIR/ports/$PORT/Makefile"
MPCONFIG_MK="$CP_DIR/py/circuitpy_mpconfig.mk"
DEFNS_MK="$CP_DIR/py/circuitpy_defns.mk"

# Drop obsolete graphics_native spike stubs.
rm -rf "$CP_DIR/shared-bindings/graphics_native" "$CP_DIR/shared-module/graphics_native"

copy_spike

if ! grep -q "graphics-cmod" "$VARIANT_MK" 2>/dev/null; then
    if grep -q "graphics_native-cmod" "$VARIANT_MK" 2>/dev/null; then
        sed -i '/# >>> graphics_native-cmod begin/,/# >>> graphics_native-cmod end/d' "$VARIANT_MK"
    fi
    cat >> "$VARIANT_MK" <<EOF

# >>> graphics-cmod begin
CIRCUITPY_GRAPHICS = 1
CFLAGS += -DCIRCUITPY_GRAPHICS=1
GRAPHICS_MOD_DIR := \$(abspath ../../../graphics)
include \$(GRAPHICS_MOD_DIR)/circuitpython.mk
# >>> graphics-cmod end
EOF
elif ! grep -q "CIRCUITPY_GRAPHICS = 1" "$VARIANT_MK" 2>/dev/null; then
    sed -i '/# >>> graphics-cmod begin/a CIRCUITPY_GRAPHICS = 1\nCFLAGS += -DCIRCUITPY_GRAPHICS=1' "$VARIANT_MK"
fi

insert_raw_after_line "$VARIANT_MK" $'shared-bindings/lvgl/__init__.c \\' $'\tshared-bindings/graphics/__init__.c \\'
insert_raw_after_line "$VARIANT_MK" $'shared-module/lvgl/__init__.c \\' $'\tshared-module/graphics/__init__.c \\'

if ! grep -q "graphics-cmod" "$PORT_MK" 2>/dev/null; then
    if grep -q "graphics_native-cmod" "$PORT_MK" 2>/dev/null; then
        sed -i '/# >>> graphics_native-cmod begin/,/# >>> graphics_native-cmod end/d' "$PORT_MK"
    fi
    cat >> "$PORT_MK" <<'EOF'

# >>> graphics-cmod begin
ifneq ($(wildcard $(abspath ../../../graphics/circuitpython.mk)),)
GRAPHICS_MOD_DIR ?= $(abspath ../../../graphics)
include $(GRAPHICS_MOD_DIR)/circuitpython.mk
endif
# >>> graphics-cmod end
EOF
fi

if ! grep -q "CIRCUITPY_GRAPHICS ?=" "$MPCONFIG_MK" 2>/dev/null; then
    insert_after_line "$MPCONFIG_MK" "CFLAGS += -DCIRCUITPY_LOCALE=\$(CIRCUITPY_LOCALE)" $'

# >>> graphics-cmod begin
CIRCUITPY_GRAPHICS ?= 0
CFLAGS += -DCIRCUITPY_GRAPHICS=$(CIRCUITPY_GRAPHICS)
# >>> graphics-cmod end
'
fi

if ! grep -q "CIRCUITPY_GRAPHICS),1)" "$DEFNS_MK" 2>/dev/null; then
    python3 - "$DEFNS_MK" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
text = path.read_text()
block = """
# >>> graphics-cmod begin
ifeq ($(CIRCUITPY_GRAPHICS),1)
SRC_PATTERNS += graphics/%
endif
# >>> graphics-cmod end
"""
needle = "ifeq ($(CIRCUITPY_MATH),1)"
if "CIRCUITPY_GRAPHICS),1)" in text:
    sys.exit(0)
if needle not in text:
    raise SystemExit(f"needle not found in {path}")
path.write_text(text.replace(needle, block + needle, 1))
PY
fi

insert_raw_after_line "$DEFNS_MK" $'\tlvgl/__init__.c \\' $'\tgraphics/__init__.c \\'

echo "graphics CP patches applied"
