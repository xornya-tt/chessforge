# Piece sets

Each folder here is a piece set that can be picked in the game (Y -> PIECES).

    pieces/<set name>/wP.bmp wN.bmp wB.bmp wR.bmp wQ.bmp wK.bmp
                      bP.bmp bN.bmp bB.bmp bR.bmp bQ.bmp bK.bmp

* 12 files, w = white, b = black; P N B R Q K = pawn knight bishop rook queen king.
* BMP only (SDL has no PNG reader built in). 32-bit BMP with alpha is best, 96x96 px recommended.
  If the BMP has no alpha channel, magenta (255,0,255) is treated as transparent.
* A missing file falls back to the built-in piece.

Lichess sets: the SVG files live in https://github.com/lichess-org/lila/tree/master/public/piece
Convert them, for example:

    rsvg-convert -w 96 -h 96 wP.svg -o wP.png
    convert wP.png -define bmp:format=bmp4 wP.bmp

Every set has its own licence (see COPYING.md in the lila repository); some are non-commercial or non-derivative.
The `cburnett` set is GPLv2+. Check the licence and keep the author credit before you share a set.
