#!/bin/sh
# Builds ChessForge3D.muxapp (a zip) for muOS Archive Manager.
# Usage: scripts/package.sh [build-dir] [stockfish-binary]
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build-rg35xx}"
STOCKFISH="${2:-}"
OUT="$ROOT/out"
STAGE="$OUT/stage/ChessForge3D"

rm -rf "$OUT/stage"
mkdir -p "$STAGE/bin" "$STAGE/saves" "$STAGE/glyph"
cp "$BUILD_DIR/chessforge3d" "$STAGE/bin/chessforge3d"
cp "$ROOT/muos/mux_launch.sh" "$STAGE/mux_launch.sh"
cp "$ROOT/muos/glyph/icon.png" "$STAGE/glyph/icon.png"
cp "$ROOT/muos/input.cfg.example" "$STAGE/input.cfg.example"
cp "$ROOT/themes/neon-carbon.cfg" "$STAGE/neon-carbon.cfg.example"
[ -n "$STOCKFISH" ] && [ -f "$STOCKFISH" ] && cp "$STOCKFISH" "$STAGE/bin/stockfish"
chmod +x "$STAGE/mux_launch.sh" "$STAGE/bin/"*

(cd "$STAGE" && find . -type f ! -name manifest.sha256 | sed 's|^\./||' | LC_ALL=C sort | while IFS= read -r f; do sha256sum "$f"; done > manifest.sha256)
(cd "$STAGE" && rm -f "$OUT/ChessForge3D.muxapp" && zip -0 -X -qr "$OUT/ChessForge3D.muxapp" .)
echo "Ready: $OUT/ChessForge3D.muxapp"
echo "Copy to <SD>/MUOS/ARCHIVE/ and install via Applications -> Archive Manager."
