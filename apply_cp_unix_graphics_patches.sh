#!/usr/bin/env bash
# Apply CircuitPython graphics_native integration (unix/coverage).
set -euo pipefail

GRAPHICS_MOD_DIR=$(cd "$(dirname "$0")" && pwd)
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$GRAPHICS_MOD_DIR/.." && pwd)}"
CP_DIR="${CP_DIR:-$WORKSPACE_DIR/circuitpython}"
PORT=unix
VARIANT="${VARIANT:-coverage}"
SPIKE_DIR="$GRAPHICS_MOD_DIR/circuitpython_spike"

[[ -d "$CP_DIR/.git" ]] || { echo "CircuitPython not found: $CP_DIR" >&2; exit 1; }

copy_spike() {
    while read -r dest src; do
        [[ -z "$dest" ]] && continue
        mkdir -p "$CP_DIR/$dest"
        cp "$SPIKE_DIR/$dest/$src" "$CP_DIR/$dest/$src"
    done < "$SPIKE_DIR/copy_manifest.txt"
}

VARIANT_MK="$CP_DIR/ports/$PORT/variants/$VARIANT/mpconfigvariant.mk"
if ! grep -q "graphics_native-cmod" "$VARIANT_MK" 2>/dev/null; then
    cat >> "$VARIANT_MK" <<EOF

# >>> graphics_native-cmod begin
GRAPHICS_MOD_DIR := \$(abspath ../../../graphics)
include \$(GRAPHICS_MOD_DIR)/circuitpython.mk
# >>> graphics_native-cmod end
EOF
fi

PORT_MK="$CP_DIR/ports/$PORT/Makefile"
if ! grep -q "graphics_native-cmod" "$PORT_MK" 2>/dev/null; then
    cat >> "$PORT_MK" <<'EOF'

# >>> graphics_native-cmod begin
ifneq ($(wildcard $(abspath ../../../graphics/circuitpython.mk)),)
GRAPHICS_MOD_DIR ?= $(abspath ../../../graphics)
include $(GRAPHICS_MOD_DIR)/circuitpython.mk
endif
# >>> graphics_native-cmod end
EOF
fi

copy_spike
echo "graphics_native CP patches applied"
