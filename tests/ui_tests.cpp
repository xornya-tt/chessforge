#include "font.hpp"
#include "i18n.hpp"
#include "input_map.hpp"
#include "pieces3d.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace chessforge;

namespace {
int failures = 0;
void check(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}
}  // namespace

int main() {
    // Every string exists in every language and only uses glyphs the font can draw.
    for (int lang = 0; lang < LanguageCount; ++lang) {
        for (int key = 0; key < static_cast<int>(Str::Count); ++key) {
            const char* text = trIn(static_cast<Str>(key), lang);
            const std::string where = std::string(languageCode(lang)) + " string #" + std::to_string(key);
            check(text != nullptr && text[0] != '\0', where + " is empty");
            if (text == nullptr) continue;
            for (const char32_t c : decodeUtf8(text)) {
                Glyph glyph;
                check(glyphFor(c, glyph), where + " has no glyph for U+" + std::to_string(static_cast<unsigned>(c)) + " in \"" + text + "\"");
            }
        }
        Glyph glyph;
        for (const char32_t c : decodeUtf8(languageName(lang))) check(glyphFor(c, glyph), std::string("language name ") + languageName(lang));
    }
    check(languageFromCode("ru") == 1, "language code lookup");
    check(languageFromCode("xx") == -1, "unknown language code");

    // Default bindings reproduce the layout of earlier versions.
    PadLayout layout;
    layout.nintendo = true;
    InputMap map(layout);
    check(map.lookup(padid::controller(padid::B)) == Action::Confirm, "nintendo: SDL B confirms");
    check(map.lookup(padid::controller(padid::A)) == Action::Back, "nintendo: SDL A goes back");
    check(map.label(Action::Confirm) == "A", "confirm is printed as A on nintendo layout");
    check(map.lookup(padid::controller(padid::DpadUp)) == Action::Up, "dpad up");
    check(map.lookup(padid::axis(0, true)) == Action::Right, "stick right");
    check(!map.lookup(12345).has_value(), "unknown button");

    // Remapping swaps with the previous owner so that no action is left without a button.
    const int l1 = padid::controller(padid::LeftShoulder);
    const int confirmBefore = map.bindings(Action::Confirm).front();
    check(map.assign(Action::Confirm, l1), "assigning a used button reports a swap");
    check(map.lookup(l1) == Action::Confirm, "confirm now on L1");
    check(map.lookup(confirmBefore) == Action::CamLeft, "camera left received the old confirm button");
    for (int i = 0; i < ActionCount; ++i) check(!map.bindings(static_cast<Action>(i)).empty(), "action keeps a button");

    // Save / load round trip.
    const std::string path = "ui_tests_controls.cfg";
    check(map.save(path), "save bindings");
    InputMap loaded(layout);
    loaded.load(path);
    for (int i = 0; i < ActionCount; ++i) {
        check(loaded.bindings(static_cast<Action>(i)) == map.bindings(static_cast<Action>(i)), "round trip of action " + std::to_string(i));
    }
    std::remove(path.c_str());

    map.resetDefaults();
    check(map.lookup(padid::controller(padid::B)) == Action::Confirm, "reset restores defaults");

    // The 3D piece renderer produces distinct, non-empty sprites.
    const PieceType types[] = {PieceType::Pawn, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen, PieceType::King};
    std::size_t previous = 0;
    for (const PieceType type : types) {
        const SpriteImage sprite = renderPieceSprite(type, true, 0.3F, 0.85F, {240, 240, 230}, {20, 20, 20}, 48);
        std::size_t covered = 0;
        for (std::size_t i = 3; i < sprite.rgba.size(); i += 4) covered += sprite.rgba[i] > 128 ? 1U : 0U;
        check(sprite.width > 0 && sprite.height > 0 && covered > 200, "sprite is drawn");
        check(covered != previous, "pieces have different silhouettes");
        previous = covered;
    }

    if (failures == 0) std::cout << "UI tests passed\n";
    return failures == 0 ? 0 : 1;
}
