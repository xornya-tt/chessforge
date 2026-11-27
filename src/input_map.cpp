#include "input_map.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <sstream>

namespace chessforge {

void PadLayout::loadFile(const std::string& path) {
    std::ifstream input(path);
    std::string key;
    std::string value;
    while (std::getline(input, key, '=') && std::getline(input, value)) {
        // Allow comment lines and Windows line endings.
        while (!key.empty() && (key.front() == '\n' || key.front() == '\r' || key.front() == ' ')) key.erase(key.begin());
        if (key.empty() || key.front() == '#') continue;
        try {
            const int v = std::stoi(value);
            if (key == "nintendo_layout") nintendo = v != 0;
            else if (key == "btn_a") a = v;
            else if (key == "btn_b") b = v;
            else if (key == "btn_x") x = v;
            else if (key == "btn_y") y = v;
            else if (key == "btn_l") l = v;
            else if (key == "btn_r") r = v;
            else if (key == "btn_select") select = v;
            else if (key == "btn_start") start = v;
            else if (key == "btn_menu") menu = v;
        } catch (const std::exception&) {
        }
    }
}

InputMap::InputMap(PadLayout layout) : layout_(layout) { resetDefaults(); }

std::vector<int> InputMap::defaultsFor(Action action) const {
    using namespace padid;
    const bool n = layout_.nintendo;
    switch (action) {
        case Action::Up: return {controller(DpadUp), axis(1, false), Hat + 0};
        case Action::Down: return {controller(DpadDown), axis(1, true), Hat + 1};
        case Action::Left: return {controller(DpadLeft), axis(0, false), Hat + 2};
        case Action::Right: return {controller(DpadRight), axis(0, true), Hat + 3};
        case Action::Confirm: return {controller(n ? B : A), joystick(layout_.a)};
        case Action::Back: return {controller(n ? A : B), joystick(layout_.b)};
        case Action::ToggleView: return {controller(n ? Y : X), joystick(layout_.x)};
        case Action::Settings: return {controller(n ? X : Y), joystick(layout_.y)};
        case Action::NewGame: return {controller(Start), joystick(layout_.start)};
        case Action::Help: return {controller(Back), joystick(layout_.select)};
        case Action::CamLeft: return {controller(LeftShoulder), joystick(layout_.l)};
        case Action::CamRight: return {controller(RightShoulder), joystick(layout_.r)};
        case Action::Quit: return {controller(Guide), joystick(layout_.menu)};
        case Action::Count: break;
    }
    return {};
}

void InputMap::resetDefaults() {
    for (int i = 0; i < ActionCount; ++i) bindings_[static_cast<std::size_t>(i)] = defaultsFor(static_cast<Action>(i));
    rebuild();
}

void InputMap::rebuild() {
    reverse_.clear();
    for (int i = 0; i < ActionCount; ++i) {
        for (const int id : bindings_[static_cast<std::size_t>(i)]) reverse_.emplace(id, static_cast<Action>(i));
    }
}

std::optional<Action> InputMap::lookup(int id) const {
    const auto found = reverse_.find(id);
    if (found == reverse_.end()) return std::nullopt;
    return found->second;
}

const std::vector<int>& InputMap::bindings(Action action) const { return bindings_[static_cast<std::size_t>(action)]; }

bool InputMap::assign(Action action, int id) {
    auto& own = bindings_[static_cast<std::size_t>(action)];
    const std::optional<Action> owner = lookup(id);
    bool swapped = false;
    if (owner && *owner != action) {
        swapped = true;
        auto& other = bindings_[static_cast<std::size_t>(*owner)];
        other.erase(std::remove(other.begin(), other.end(), id), other.end());
        if (!own.empty()) other.push_back(own.front());
        if (other.empty()) other = defaultsFor(*owner);
    }
    own = {id};
    rebuild();
    // A default restored for the previous owner must not steal the new button back.
    if (swapped) {
        auto& other = bindings_[static_cast<std::size_t>(*owner)];
        other.erase(std::remove(other.begin(), other.end(), id), other.end());
        rebuild();
    }
    return swapped;
}

bool InputMap::isDirection(Action action) {
    return action == Action::Up || action == Action::Down || action == Action::Left || action == Action::Right;
}

const char* InputMap::actionKey(Action action) {
    switch (action) {
        case Action::Up: return "up";
        case Action::Down: return "down";
        case Action::Left: return "left";
        case Action::Right: return "right";
        case Action::Confirm: return "confirm";
        case Action::Back: return "back";
        case Action::ToggleView: return "view";
        case Action::Settings: return "settings";
        case Action::NewGame: return "newgame";
        case Action::Help: return "help";
        case Action::CamLeft: return "camleft";
        case Action::CamRight: return "camright";
        case Action::Quit: return "quit";
        case Action::Count: break;
    }
    return "";
}

std::string InputMap::idName(int id) const {
    using namespace padid;
    const bool n = layout_.nintendo;
    if (id >= ControllerButton && id < JoystickButton) {
        switch (id - ControllerButton) {
            case A: return n ? "B" : "A";
            case B: return n ? "A" : "B";
            case X: return n ? "Y" : "X";
            case Y: return n ? "X" : "Y";
            case Back: return "SELECT";
            case Guide: return "MENU";
            case Start: return "START";
            case LeftStick: return "L3";
            case RightStick: return "R3";
            case LeftShoulder: return "L1";
            case RightShoulder: return "R1";
            case DpadUp: return "DPAD UP";
            case DpadDown: return "DPAD DOWN";
            case DpadLeft: return "DPAD LEFT";
            case DpadRight: return "DPAD RIGHT";
            default: return "PAD " + std::to_string(id - ControllerButton);
        }
    }
    if (id >= JoystickButton && id < Hat) return "BTN " + std::to_string(id - JoystickButton);
    if (id >= Hat && id < Axis) {
        static const char* const names[4] = {"HAT UP", "HAT DOWN", "HAT LEFT", "HAT RIGHT"};
        const int index = id - Hat;
        return index < 4 ? names[index] : "HAT " + std::to_string(index);
    }
    if (id >= Axis) {
        const int axisIndex = (id - Axis) / 2;
        const bool positive = ((id - Axis) % 2) == 1;
        if (axisIndex == 0) return positive ? "STICK RIGHT" : "STICK LEFT";
        if (axisIndex == 1) return positive ? "STICK DOWN" : "STICK UP";
        if (axisIndex == 4 && positive) return "L2";
        if (axisIndex == 5 && positive) return "R2";
        return "AXIS " + std::to_string(axisIndex) + (positive ? "+" : "-");
    }
    return "?";
}

std::string InputMap::label(Action action) const {
    const auto& list = bindings(action);
    return list.empty() ? std::string("-") : idName(list.front());
}

std::string InputMap::labels(Action action) const {
    const auto& list = bindings(action);
    std::string result;
    for (const int id : list) {
        if (!result.empty()) result += " / ";
        result += idName(id);
    }
    return result.empty() ? std::string("-") : result;
}

void InputMap::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) return;
    std::string line;
    while (std::getline(input, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || line.compare(0, 5, "bind_") != 0) continue;
        const std::string key = line.substr(5, equals - 5);
        for (int i = 0; i < ActionCount; ++i) {
            if (key != actionKey(static_cast<Action>(i))) continue;
            std::vector<int> ids;
            std::stringstream values(line.substr(equals + 1));
            std::string item;
            while (std::getline(values, item, ',')) {
                try {
                    ids.push_back(std::stoi(item));
                } catch (const std::exception&) {
                }
            }
            bindings_[static_cast<std::size_t>(i)] = ids;
        }
    }
    // Every button belongs to one action only; an action never stays without buttons.
    std::unordered_map<int, bool> used;
    for (auto& list : bindings_) {
        std::vector<int> unique;
        for (const int id : list) {
            if (used.emplace(id, true).second) unique.push_back(id);
        }
        list = unique;
    }
    for (int i = 0; i < ActionCount; ++i) {
        auto& list = bindings_[static_cast<std::size_t>(i)];
        if (!list.empty()) continue;
        for (const int id : defaultsFor(static_cast<Action>(i))) {
            if (used.emplace(id, true).second) list.push_back(id);
        }
    }
    rebuild();
}

bool InputMap::save(const std::string& path) const {
    std::ofstream output(path, std::ios::trunc);
    if (!output) return false;
    output << "# ChessForge 3D button bindings (edited in Settings -> Controls)\n";
    for (int i = 0; i < ActionCount; ++i) {
        output << "bind_" << actionKey(static_cast<Action>(i)) << '=';
        const auto& list = bindings_[static_cast<std::size_t>(i)];
        for (std::size_t k = 0; k < list.size(); ++k) output << (k == 0 ? "" : ",") << list[k];
        output << '\n';
    }
    return static_cast<bool>(output);
}

}  // namespace chessforge
