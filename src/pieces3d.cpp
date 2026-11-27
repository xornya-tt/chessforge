#include "pieces3d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace chessforge {
namespace {

constexpr float Pi = 3.14159265358979F;

struct V3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
float length(V3 a) { return std::sqrt(dot(a, a)); }
V3 normalized(V3 a) {
    const float l = length(a);
    return l > 1e-8F ? a * (1.0F / l) : V3{0.0F, 1.0F, 0.0F};
}

enum Material : unsigned char { Body = 0, Dark = 1 };

struct Mesh {
    std::vector<V3> position;
    std::vector<V3> normal;
    std::vector<std::array<int, 3>> triangle;
    std::vector<unsigned char> material;

    int addVertex(V3 p, V3 n) {
        position.push_back(p);
        normal.push_back(n);
        return static_cast<int>(position.size()) - 1;
    }
    void addTriangle(int a, int b, int c, unsigned char mat) {
        triangle.push_back({a, b, c});
        material.push_back(mat);
    }
};

using Profile = std::vector<std::pair<float, float>>;  // (radius, height) from bottom to top

// Surface of revolution around a vertical axis through (cx, cz). Creases (sharp
// profile corners) keep separate normals, gentle bends are smoothed.
void addLathe(Mesh& mesh, const Profile& profile, float cx, float cz, float y0, int segments, unsigned char mat) {
    const std::size_t count = profile.size();
    if (count < 2) return;
    std::vector<std::array<float, 2>> bandNormal(count - 1);
    for (std::size_t i = 0; i + 1 < count; ++i) {
        const float dr = profile[i + 1].first - profile[i].first;
        const float dy = profile[i + 1].second - profile[i].second;
        const float len = std::max(1e-6F, std::sqrt(dr * dr + dy * dy));
        bandNormal[i] = {dy / len, -dr / len};
    }
    for (std::size_t i = 0; i + 1 < count; ++i) {
        std::array<float, 2> low = bandNormal[i];
        std::array<float, 2> high = bandNormal[i];
        if (i > 0) {
            const auto& prev = bandNormal[i - 1];
            if (prev[0] * low[0] + prev[1] * low[1] > 0.55F) {
                low = {prev[0] + bandNormal[i][0], prev[1] + bandNormal[i][1]};
                const float l = std::max(1e-6F, std::sqrt(low[0] * low[0] + low[1] * low[1]));
                low = {low[0] / l, low[1] / l};
            }
        }
        if (i + 2 < count) {
            const auto& next = bandNormal[i + 1];
            if (next[0] * high[0] + next[1] * high[1] > 0.55F) {
                high = {next[0] + bandNormal[i][0], next[1] + bandNormal[i][1]};
                const float l = std::max(1e-6F, std::sqrt(high[0] * high[0] + high[1] * high[1]));
                high = {high[0] / l, high[1] / l};
            }
        }
        const int firstLow = static_cast<int>(mesh.position.size());
        for (int s = 0; s <= segments; ++s) {
            const float angle = 2.0F * Pi * static_cast<float>(s) / static_cast<float>(segments);
            const float c = std::cos(angle);
            const float n = std::sin(angle);
            mesh.addVertex({cx + profile[i].first * c, y0 + profile[i].second, cz + profile[i].first * n},
                           normalized({low[0] * c, low[1], low[0] * n}));
        }
        const int firstHigh = static_cast<int>(mesh.position.size());
        for (int s = 0; s <= segments; ++s) {
            const float angle = 2.0F * Pi * static_cast<float>(s) / static_cast<float>(segments);
            const float c = std::cos(angle);
            const float n = std::sin(angle);
            mesh.addVertex({cx + profile[i + 1].first * c, y0 + profile[i + 1].second, cz + profile[i + 1].first * n},
                           normalized({high[0] * c, high[1], high[0] * n}));
        }
        for (int s = 0; s < segments; ++s) {
            mesh.addTriangle(firstLow + s, firstLow + s + 1, firstHigh + s + 1, mat);
            mesh.addTriangle(firstLow + s, firstHigh + s + 1, firstHigh + s, mat);
        }
    }
}

// Flat-shaded box, rotated around the vertical axis by `turn`.
void addBox(Mesh& mesh, V3 centre, V3 half, float turn, unsigned char mat) {
    const float c = std::cos(turn);
    const float s = std::sin(turn);
    const auto place = [&](float x, float y, float z) { return V3{centre.x + x * c - z * s, centre.y + y, centre.z + x * s + z * c}; };
    const auto rotate = [&](V3 n) { return V3{n.x * c - n.z * s, n.y, n.x * s + n.z * c}; };
    const std::array<V3, 6> normals{{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
    for (const V3& n : normals) {
        const V3 u = std::abs(n.y) > 0.5F ? V3{1, 0, 0} : V3{0, 1, 0};
        const V3 v = cross(n, u);
        const std::array<std::pair<float, float>, 4> corners{{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}};
        std::array<int, 4> index{};
        for (std::size_t i = 0; i < 4; ++i) {
            const V3 offset = u * (corners[i].first) + v * (corners[i].second);
            const V3 local{n.x * half.x + offset.x * half.x, n.y * half.y + offset.y * half.y, n.z * half.z + offset.z * half.z};
            index[i] = mesh.addVertex(place(local.x, local.y, local.z), rotate(n));
        }
        mesh.addTriangle(index[0], index[1], index[2], mat);
        mesh.addTriangle(index[0], index[2], index[3], mat);
    }
}

struct TubeNode {
    float z;
    float y;
    float a;  // half width across the piece (x axis)
    float b;  // half depth in the side plane
};

// Smoothly shaded tube swept along a bent spine in the y/z plane (used for the horse head).
void addTube(Mesh& mesh, const std::vector<TubeNode>& nodes, int segments, unsigned char mat) {
    const std::size_t count = nodes.size();
    if (count < 2) return;
    const int first = static_cast<int>(mesh.position.size());
    std::vector<V3> centres(count);
    for (std::size_t i = 0; i < count; ++i) {
        const TubeNode& previous = nodes[i == 0 ? 0 : i - 1];
        const TubeNode& next = nodes[i + 1 < count ? i + 1 : i];
        V3 tangent = normalized({0.0F, next.y - previous.y, next.z - previous.z});
        const V3 side{0.0F, tangent.z, -tangent.y};
        centres[i] = {0.0F, nodes[i].y, nodes[i].z};
        for (int s = 0; s < segments; ++s) {
            const float angle = 2.0F * Pi * static_cast<float>(s) / static_cast<float>(segments);
            const V3 p = centres[i] + V3{1.0F, 0.0F, 0.0F} * (nodes[i].a * std::cos(angle)) + side * (nodes[i].b * std::sin(angle));
            mesh.addVertex(p, {0.0F, 0.0F, 0.0F});
        }
    }
    const auto at = [&](std::size_t ring, int s) { return first + static_cast<int>(ring) * segments + (s % segments); };
    for (std::size_t ring = 0; ring + 1 < count; ++ring) {
        for (int s = 0; s < segments; ++s) {
            const int a = at(ring, s);
            const int b = at(ring, s + 1);
            const int c = at(ring + 1, s + 1);
            const int d = at(ring + 1, s);
            for (const auto& tri : {std::array<int, 3>{a, b, c}, std::array<int, 3>{a, c, d}}) {
                const V3 n = cross(mesh.position[static_cast<std::size_t>(tri[1])] - mesh.position[static_cast<std::size_t>(tri[0])],
                                   mesh.position[static_cast<std::size_t>(tri[2])] - mesh.position[static_cast<std::size_t>(tri[0])]);
                for (const int v : tri) {
                    V3& target = mesh.normal[static_cast<std::size_t>(v)];
                    target = target + n;
                }
                mesh.addTriangle(tri[0], tri[1], tri[2], mat);
            }
        }
    }
    // Orient all normals away from the spine.
    float outward = 0.0F;
    for (std::size_t ring = 0; ring < count; ++ring) {
        for (int s = 0; s < segments; ++s) {
            const std::size_t v = static_cast<std::size_t>(at(ring, s));
            outward += dot(mesh.normal[v], mesh.position[v] - centres[ring]);
        }
    }
    const float sign = outward < 0.0F ? -1.0F : 1.0F;
    for (std::size_t ring = 0; ring < count; ++ring) {
        for (int s = 0; s < segments; ++s) {
            const std::size_t v = static_cast<std::size_t>(at(ring, s));
            V3 n = normalized(mesh.normal[v] * sign);
            if (length(mesh.normal[v]) < 1e-7F) {
                const std::size_t other = ring + 1 < count ? ring + 1 : ring - 1;
                n = normalized(centres[ring] - centres[other]);
                if (ring + 1 < count) n = n * -1.0F;
            }
            mesh.normal[v] = n;
        }
    }
}

void addBase(Profile& p) {
    p = {{0.37F, 0.00F}, {0.37F, 0.05F}, {0.34F, 0.08F}, {0.27F, 0.10F}, {0.27F, 0.13F}, {0.30F, 0.145F},
         {0.30F, 0.175F}, {0.26F, 0.195F}};
}

Mesh buildPawn() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.19F, 0.22F}, {0.14F, 0.29F}, {0.12F, 0.41F}, {0.14F, 0.45F}, {0.20F, 0.48F}, {0.21F, 0.51F},
                       {0.15F, 0.53F}, {0.11F, 0.55F}, {0.14F, 0.58F}, {0.16F, 0.63F}, {0.16F, 0.68F}, {0.14F, 0.73F},
                       {0.10F, 0.77F}, {0.05F, 0.795F}, {0.00F, 0.80F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 28, Body);
    return mesh;
}

Mesh buildRook() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.22F, 0.22F}, {0.20F, 0.30F}, {0.20F, 0.53F}, {0.27F, 0.58F}, {0.29F, 0.61F}, {0.29F, 0.73F}, {0.00F, 0.73F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 28, Body);
    constexpr int Merlons = 6;
    for (int i = 0; i < Merlons; ++i) {
        const float angle = 2.0F * Pi * (static_cast<float>(i) + 0.5F) / static_cast<float>(Merlons);
        addBox(mesh, {0.215F * std::cos(angle), 0.73F + 0.075F, 0.215F * std::sin(angle)}, {0.045F, 0.075F, 0.065F}, angle, Body);
    }
    return mesh;
}

Mesh buildBishop() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.21F, 0.22F}, {0.15F, 0.31F}, {0.11F, 0.45F}, {0.12F, 0.48F}, {0.20F, 0.51F}, {0.20F, 0.54F},
                       {0.12F, 0.56F}, {0.15F, 0.60F}, {0.18F, 0.66F}, {0.18F, 0.72F}, {0.15F, 0.80F}, {0.10F, 0.88F},
                       {0.05F, 0.935F}, {0.065F, 0.955F}, {0.065F, 0.985F}, {0.04F, 1.015F}, {0.00F, 1.03F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 28, Body);
    // Mitre slit: a thin dark wedge on the front of the mitre.
    addBox(mesh, {0.0F, 0.80F, 0.125F}, {0.012F, 0.065F, 0.045F}, 0.0F, Dark);
    return mesh;
}

Mesh buildQueen() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.22F, 0.22F}, {0.16F, 0.31F}, {0.12F, 0.47F}, {0.13F, 0.51F}, {0.21F, 0.55F}, {0.22F, 0.59F},
                       {0.14F, 0.63F}, {0.14F, 0.67F}, {0.18F, 0.73F}, {0.23F, 0.83F}, {0.26F, 0.93F}, {0.26F, 0.97F},
                       {0.19F, 0.97F}, {0.0F, 0.94F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 32, Body);
    constexpr int Spikes = 8;
    const Profile spike{{0.050F, 0.0F}, {0.035F, 0.07F}, {0.050F, 0.10F}, {0.045F, 0.14F}, {0.0F, 0.165F}};
    for (int i = 0; i < Spikes; ++i) {
        const float angle = 2.0F * Pi * static_cast<float>(i) / static_cast<float>(Spikes);
        addLathe(mesh, spike, 0.22F * std::cos(angle), 0.22F * std::sin(angle), 0.95F, 10, Body);
    }
    const Profile orb{{0.05F, 0.0F}, {0.08F, 0.04F}, {0.075F, 0.09F}, {0.04F, 0.135F}, {0.0F, 0.15F}};
    addLathe(mesh, orb, 0.0F, 0.0F, 0.93F, 16, Body);
    return mesh;
}

Mesh buildKing() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.22F, 0.22F}, {0.16F, 0.31F}, {0.12F, 0.47F}, {0.13F, 0.51F}, {0.21F, 0.55F}, {0.22F, 0.59F},
                       {0.14F, 0.63F}, {0.14F, 0.67F}, {0.18F, 0.73F}, {0.23F, 0.84F}, {0.26F, 0.95F}, {0.27F, 1.00F},
                       {0.22F, 1.04F}, {0.12F, 1.05F}, {0.0F, 1.05F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 32, Body);
    addBox(mesh, {0.0F, 1.05F + 0.13F, 0.0F}, {0.038F, 0.13F, 0.038F}, 0.0F, Body);
    addBox(mesh, {0.0F, 1.05F + 0.17F, 0.0F}, {0.11F, 0.038F, 0.038F}, 0.0F, Body);
    return mesh;
}

Mesh buildKnight() {
    Mesh mesh;
    Profile p;
    addBase(p);
    const Profile rest{{0.23F, 0.22F}, {0.21F, 0.26F}, {0.0F, 0.26F}};
    p.insert(p.end(), rest.begin(), rest.end());
    addLathe(mesh, p, 0.0F, 0.0F, 0.0F, 28, Body);
    const std::vector<TubeNode> spine{
        {-0.03F, 0.20F, 0.17F, 0.19F}, {-0.07F, 0.36F, 0.14F, 0.17F}, {-0.06F, 0.54F, 0.125F, 0.16F},
        {0.00F, 0.70F, 0.12F, 0.155F}, {0.12F, 0.82F, 0.115F, 0.15F}, {0.25F, 0.80F, 0.09F, 0.12F},
        {0.35F, 0.70F, 0.075F, 0.09F}, {0.405F, 0.64F, 0.05F, 0.055F}, {0.43F, 0.615F, 0.0F, 0.0F}};
    addTube(mesh, spine, 16, Body);
    const Profile ear{{0.05F, 0.0F}, {0.035F, 0.06F}, {0.0F, 0.12F}};
    addLathe(mesh, ear, 0.055F, 0.04F, 0.93F, 10, Body);
    addLathe(mesh, ear, -0.055F, 0.04F, 0.93F, 10, Body);
    const Profile eye{{0.0F, 0.0F}, {0.03F, 0.02F}, {0.03F, 0.045F}, {0.0F, 0.065F}};
    addLathe(mesh, eye, 0.10F, 0.19F, 0.80F, 8, Dark);
    addLathe(mesh, eye, -0.10F, 0.19F, 0.80F, 8, Dark);
    // Mane: a ridge along the back of the neck.
    addBox(mesh, {0.0F, 0.62F, -0.155F}, {0.028F, 0.17F, 0.03F}, 0.0F, Dark);
    return mesh;
}

const Mesh& modelFor(PieceType type) {
    static const Mesh pawn = buildPawn();
    static const Mesh knight = buildKnight();
    static const Mesh bishop = buildBishop();
    static const Mesh rook = buildRook();
    static const Mesh queen = buildQueen();
    static const Mesh king = buildKing();
    switch (type) {
        case PieceType::Knight: return knight;
        case PieceType::Bishop: return bishop;
        case PieceType::Rook: return rook;
        case PieceType::Queen: return queen;
        case PieceType::King: return king;
        default: return pawn;
    }
}

struct ScreenVertex {
    float x;
    float y;
    float z;       // camera depth, smaller is nearer
    V3 normal;     // camera space
    float height;  // model-space height, for contact darkening
};

struct Target {
    int width;
    int height;
    std::vector<float> depth;
    std::vector<unsigned char> rgb;
    std::vector<unsigned char> covered;
};

struct Shading {
    Rgb8 body;
    Rgb8 dark;
};

unsigned char toByte(float value) { return static_cast<unsigned char>(std::clamp(value, 0.0F, 255.0F) + 0.5F); }

void shadePixel(Target& target, int x, int y, float depth, V3 normal, float height, Rgb8 base) {
    const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width) + static_cast<std::size_t>(x);
    if (depth >= target.depth[index]) return;
    target.depth[index] = depth;
    const V3 n = normalized(normal);
    static const V3 light = normalized({-0.45F, 0.75F, -0.50F});
    static const V3 half = normalized(light + V3{0.0F, 0.0F, -1.0F});
    const float diffuse = std::max(0.0F, dot(n, light));
    const float hemisphere = 0.5F + 0.5F * n.y;
    const float specular = 0.30F * std::pow(std::max(0.0F, dot(n, half)), 26.0F);
    const float rim = 0.10F * std::pow(1.0F - std::clamp(-n.z, 0.0F, 1.0F), 2.0F) * std::max(0.0F, n.y + 0.4F);
    const float contact = 0.80F + 0.20F * std::clamp(height / 0.28F, 0.0F, 1.0F);
    const float shade = (0.24F + 0.20F * hemisphere + 0.68F * diffuse + rim) * contact;
    const float add = specular * 255.0F * contact;
    target.rgb[index * 3 + 0] = toByte(static_cast<float>(base.r) * shade + add);
    target.rgb[index * 3 + 1] = toByte(static_cast<float>(base.g) * shade + add);
    target.rgb[index * 3 + 2] = toByte(static_cast<float>(base.b) * shade + add);
    target.covered[index] = 1;
}

void rasterize(Target& target, const ScreenVertex& a, const ScreenVertex& b, const ScreenVertex& c, Rgb8 base) {
    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::abs(area) < 1e-5F) return;
    const int minX = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int maxX = std::min(target.width - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int maxY = std::min(target.height - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));
    const float inverse = 1.0F / area;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float px = static_cast<float>(x) + 0.5F;
            const float py = static_cast<float>(y) + 0.5F;
            const float w0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) * inverse;
            const float w1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) * inverse;
            const float w2 = 1.0F - w0 - w1;
            if (w0 < -0.001F || w1 < -0.001F || w2 < -0.001F) continue;
            const float depth = w0 * a.z + w1 * b.z + w2 * c.z;
            const V3 normal = a.normal * w0 + b.normal * w1 + c.normal * w2;
            const float height = w0 * a.height + w1 * b.height + w2 * c.height;
            shadePixel(target, x, y, depth, normal, height, base);
        }
    }
}

}  // namespace

SpriteImage renderPieceSprite(PieceType type, bool white, float yaw, float pitch, Rgb8 body, Rgb8 edge, int pixelsPerUnit) {
    constexpr int Supersample = 2;
    constexpr int OutlinePasses = 3;
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);
    const float cosPitch = std::cos(pitch);
    const float sinPitch = std::sin(pitch);
    const float topUnits = 1.34F * cosPitch + 0.46F * sinPitch + 0.06F;
    const float bottomUnits = 0.46F * sinPitch + 0.06F;
    const float halfWidthUnits = 0.58F;

    SpriteImage image;
    image.pixelsPerUnit = static_cast<float>(pixelsPerUnit);
    image.width = static_cast<int>(std::ceil(2.0F * halfWidthUnits * static_cast<float>(pixelsPerUnit)));
    image.height = static_cast<int>(std::ceil((topUnits + bottomUnits) * static_cast<float>(pixelsPerUnit)));
    image.anchorX = static_cast<float>(image.width) * 0.5F;
    image.anchorY = topUnits * static_cast<float>(pixelsPerUnit);
    image.rgba.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0);

    Target target;
    target.width = image.width * Supersample;
    target.height = image.height * Supersample;
    const std::size_t pixels = static_cast<std::size_t>(target.width) * static_cast<std::size_t>(target.height);
    target.depth.assign(pixels, std::numeric_limits<float>::infinity());
    target.rgb.assign(pixels * 3U, 0);
    target.covered.assign(pixels, 0);

    const float scale = static_cast<float>(pixelsPerUnit * Supersample);
    const float anchorX = image.anchorX * static_cast<float>(Supersample);
    const float anchorY = image.anchorY * static_cast<float>(Supersample);
    // Pieces are turned with the camera so that details stay readable from every side:
    // the knight shows its profile (white looks right, black left, both slightly towards the viewer),
    // the bishop slit and the king cross face the viewer.
    float turn = -yaw;
    if (type == PieceType::Knight) turn = (white ? -Pi * 0.5F - 0.45F : Pi * 0.5F + 0.45F) - yaw;
    else if (type == PieceType::Bishop) turn = Pi - yaw;
    const float cosTurn = std::cos(turn);
    const float sinTurn = std::sin(turn);

    const Mesh& mesh = modelFor(type);
    std::vector<ScreenVertex> vertices(mesh.position.size());
    const auto toCamera = [&](V3 v) {
        const float tx = v.x * cosTurn - v.z * sinTurn;
        const float tz = v.x * sinTurn + v.z * cosTurn;
        const float x1 = tx * cosYaw - tz * sinYaw;
        const float z1 = tx * sinYaw + tz * cosYaw;
        return V3{x1, v.y * cosPitch + z1 * sinPitch, z1 * cosPitch - v.y * sinPitch};
    };
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const V3 camera = toCamera(mesh.position[i]);
        vertices[i].x = anchorX + camera.x * scale;
        vertices[i].y = anchorY - camera.y * scale;
        vertices[i].z = camera.z;
        vertices[i].normal = toCamera(mesh.normal[i]);
        vertices[i].height = mesh.position[i].y;
    }
    const Rgb8 dark{static_cast<unsigned char>(body.r / 5), static_cast<unsigned char>(body.g / 5), static_cast<unsigned char>(body.b / 5)};
    for (std::size_t t = 0; t < mesh.triangle.size(); ++t) {
        const auto& tri = mesh.triangle[t];
        rasterize(target, vertices[static_cast<std::size_t>(tri[0])], vertices[static_cast<std::size_t>(tri[1])],
                  vertices[static_cast<std::size_t>(tri[2])], mesh.material[t] == Dark ? dark : body);
    }

    // Outline: grow the silhouette and paint the new pixels with the edge colour.
    std::vector<unsigned char> mask = target.covered;
    for (int pass = 0; pass < OutlinePasses; ++pass) {
        std::vector<unsigned char> grown = mask;
        for (int y = 0; y < target.height; ++y) {
            for (int x = 0; x < target.width; ++x) {
                const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width) + static_cast<std::size_t>(x);
                if (mask[index] != 0) continue;
                const bool near = (x > 0 && mask[index - 1] != 0) || (x + 1 < target.width && mask[index + 1] != 0) ||
                                  (y > 0 && mask[index - static_cast<std::size_t>(target.width)] != 0) ||
                                  (y + 1 < target.height && mask[index + static_cast<std::size_t>(target.width)] != 0);
                if (!near) continue;
                grown[index] = 1;
                target.rgb[index * 3 + 0] = edge.r;
                target.rgb[index * 3 + 1] = edge.g;
                target.rgb[index * 3 + 2] = edge.b;
            }
        }
        mask.swap(grown);
    }

    // Box filter down to the final size.
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            int count = 0;
            int r = 0;
            int g = 0;
            int b = 0;
            for (int sy = 0; sy < Supersample; ++sy) {
                for (int sx = 0; sx < Supersample; ++sx) {
                    const std::size_t index = static_cast<std::size_t>(y * Supersample + sy) * static_cast<std::size_t>(target.width) +
                                              static_cast<std::size_t>(x * Supersample + sx);
                    if (mask[index] == 0) continue;
                    ++count;
                    r += target.rgb[index * 3 + 0];
                    g += target.rgb[index * 3 + 1];
                    b += target.rgb[index * 3 + 2];
                }
            }
            if (count == 0) continue;
            const std::size_t out = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(x)) * 4U;
            image.rgba[out + 0] = static_cast<unsigned char>(r / count);
            image.rgba[out + 1] = static_cast<unsigned char>(g / count);
            image.rgba[out + 2] = static_cast<unsigned char>(b / count);
            image.rgba[out + 3] = static_cast<unsigned char>(255 * count / (Supersample * Supersample));
        }
    }
    return image;
}

SpriteImage renderShadowSprite(int size) {
    SpriteImage image;
    image.width = size;
    image.height = size;
    image.anchorX = static_cast<float>(size) * 0.5F;
    image.anchorY = static_cast<float>(size) * 0.5F;
    image.pixelsPerUnit = static_cast<float>(size);
    image.rgba.assign(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U, 0);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = (static_cast<float>(x) + 0.5F) / static_cast<float>(size) * 2.0F - 1.0F;
            const float dy = (static_cast<float>(y) + 0.5F) / static_cast<float>(size) * 2.0F - 1.0F;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const float alpha = std::clamp(1.0F - distance, 0.0F, 1.0F);
            const std::size_t out = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)) * 4U;
            image.rgba[out + 3] = static_cast<unsigned char>(alpha * alpha * 150.0F);
        }
    }
    return image;
}

}  // namespace chessforge
