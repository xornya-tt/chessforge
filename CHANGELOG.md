# Changelog

## 0.2.2

- Settings, Controls and Help screens no longer show pieces of the sidebar behind the panel.
- Release packaging: `ChessForge3D-manual.zip` (unzip into the SD card root, recommended), `ChessForge3D.muxapp` (experimental), `SHA256SUMS.txt`, release notes taken from this file.
- Licence notices are shipped with the game (`licenses/`): Stockfish (GPLv3) and the Lichess `cburnett` piece set (GPLv2+, Colin M.L. Burnett).
- README: installation by unzipping, correct `saves/` path, credits and licences.

## 0.2.1

- Four Lichess board themes (Brown, Blue, Green, Purple) with Lichess-style dark interface colours.

## 0.2.0

- 8 interface languages (EN, RU, UK, DE, ES, FR, IT, PT), chosen in Settings; Cyrillic added to the built-in font.
- In-game button remapping (Settings -> Controls), stored in `saves/controls.cfg`; on-screen hints show the real buttons.
- Two-player mode on one device with optional automatic board turn and a remembered cursor per player.
- Cursor, selection and move markers are coloured against the real colour of the square (contrast >= 3:1 on every theme and custom palette) and get a thin black/white halo.
- New 3D piece models (lathe-turned bodies, crenellated rook, horse-head knight) rendered as shaded sprites with outline and contact shadow; the selected piece is lifted.
- Board coordinates follow the board orientation in 2D and are shown in 3D.
- "Check" indicator in the sidebar; long texts are shrunk to fit instead of running off the panel.
- New tests: `ui_tests` (translations and font coverage, button mapping, 3D pieces).
