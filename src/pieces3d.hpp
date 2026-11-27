#pragma once

#include "chess.hpp"

#include <vector>

namespace chessforge {

struct Rgb8 {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

// A pre-rendered, shaded view of one 3D piece model (RGBA8, straight alpha).
// The model's base centre projects to (anchorX, anchorY) inside the image and
// one board square equals `pixelsPerUnit` pixels at the point of the anchor.
struct SpriteImage {
    int width = 0;
    int height = 0;
    float anchorX = 0.0F;
    float anchorY = 0.0F;
    float pixelsPerUnit = 0.0F;
    std::vector<unsigned char> rgba;
};

// Renders the procedural model of a piece (lathe-turned body, box crenellations,
// swept horse head for the knight) as seen by the board camera.
// yaw/pitch are the same angles the board camera uses. `white` only decides
// which way the knight looks (white right, black left).
[[nodiscard]] SpriteImage renderPieceSprite(PieceType type, bool white, float yaw, float pitch,
                                            Rgb8 body, Rgb8 edge, int pixelsPerUnit);

// Soft round contact shadow (black, alpha only), width == height.
[[nodiscard]] SpriteImage renderShadowSprite(int size);

}  // namespace chessforge
