#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace chessforge {

// Everything the player can do with the pad. Keyboard keys are fixed, pad buttons are configurable.
enum class Action : int {
    Up, Down, Left, Right, Confirm, Back, ToggleView, Settings, NewGame, Help, CamLeft, CamRight, Quit, Count
};
constexpr int ActionCount = static_cast<int>(Action::Count);

// Physical inputs are plain integers so that the bindings can be stored in a text file:
//   100 + SDL_GameControllerButton   200 + joystick button   300 + hat direction (0 up 1 down 2 left 3 right)
//   400 + axis * 2 + (1 for the positive half)
namespace padid {
constexpr int ControllerButton = 100;
constexpr int JoystickButton = 200;
constexpr int Hat = 300;
constexpr int Axis = 400;

// Same numbers as SDL_CONTROLLER_BUTTON_*.
enum ControllerButtons : int {
    A = 0, B = 1, X = 2, Y = 3, Back = 4, Guide = 5, Start = 6, LeftStick = 7, RightStick = 8,
    LeftShoulder = 9, RightShoulder = 10, DpadUp = 11, DpadDown = 12, DpadLeft = 13, DpadRight = 14
};

[[nodiscard]] constexpr int controller(int button) { return ControllerButton + button; }
[[nodiscard]] constexpr int joystick(int button) { return JoystickButton + button; }
[[nodiscard]] constexpr int axis(int index, bool positive) { return Axis + index * 2 + (positive ? 1 : 0); }
}  // namespace padid

// Layout of the device, loaded from saves/input.cfg (kept compatible with earlier versions).
struct PadLayout {
    bool nintendo = true;  // physical A/B and X/Y are swapped relative to the SDL names
    int a = 0;
    int b = 1;
    int x = 2;
    int y = 3;
    int l = 4;
    int r = 5;
    int select = 8;
    int start = 9;
    int menu = 10;

    void loadFile(const std::string& path);
};

class InputMap {
public:
    explicit InputMap(PadLayout layout = {});

    void resetDefaults();

    [[nodiscard]] std::optional<Action> lookup(int id) const;
    [[nodiscard]] const std::vector<int>& bindings(Action action) const;

    // Makes `id` the only button of `action`. A button that already belongs to another action
    // is swapped: that action receives the button the edited action used to have.
    // Returns true if such a swap happened.
    bool assign(Action action, int id);

    [[nodiscard]] static bool isDirection(Action action);
    [[nodiscard]] static const char* actionKey(Action action);

    // Physical name of a button as it is printed on the device ("A", "L1", "DPAD UP", "BTN 7").
    [[nodiscard]] std::string idName(int id) const;
    // Name of the first button of an action, for on-screen hints.
    [[nodiscard]] std::string label(Action action) const;
    // All buttons of an action, e.g. "DPAD UP / STICK UP".
    [[nodiscard]] std::string labels(Action action) const;

    void load(const std::string& path);
    bool save(const std::string& path) const;

    [[nodiscard]] const PadLayout& layout() const { return layout_; }

private:
    PadLayout layout_;
    std::array<std::vector<int>, static_cast<std::size_t>(ActionCount)> bindings_;
    std::unordered_map<int, Action> reverse_;

    [[nodiscard]] std::vector<int> defaultsFor(Action action) const;
    void rebuild();
};

}  // namespace chessforge
