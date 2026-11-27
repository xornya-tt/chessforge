# Third-party software and assets

ChessForge 3D itself is MIT licensed (see `LICENSE`). Release packages also contain or use the following.
This project is not affiliated with Lichess or the Stockfish team.

## Stockfish (chess engine, optional `bin/stockfish`)

- Licence: GNU General Public License v3. The licence text is shipped as `licenses/Stockfish-Copying.txt`.
- ChessForge only starts it as a separate program and talks to it over the UCI protocol.
- The binary in a release is built by the GitHub Actions workflow from the unmodified source
  https://github.com/official-stockfish/Stockfish (tag `sf_17.1`, `make build ARCH=armv8`).
  The complete corresponding source is available at that address.

## SDL2 (windowing, input, rendering)

- zlib licence, https://www.libsdl.org/ . Linked dynamically; the console supplies its own copy.

## Piece set `cburnett` (optional, `pieces/cburnett/`)

- Author: Colin M.L. Burnett. Licence: GPL v2 or later (the author also offers the images under other free licences),
  as listed in https://github.com/lichess-org/lila/blob/master/COPYING.md
- Source SVG files: https://github.com/lichess-org/lila/tree/master/public/piece/cburnett
- The release workflow converts them to 96x96 BMP files without other changes. The images are separate data files that the
  game loads at run time; they are not part of ChessForge's MIT-licensed source code.

## Lichess board colours

- The `LICHESS ...` themes only use the colour values of the Lichess board themes. No Lichess artwork or logo is included.
