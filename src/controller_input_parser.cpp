#include "dreamcast/controller_input_parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace dreamcast {

namespace {

std::string normalized(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parse_button(const std::string& name, ControllerButton& button) {
    const std::string key = normalized(name);
    if (key == "a") button = ButtonA;
    else if (key == "b") button = ButtonB;
    else if (key == "x") button = ButtonX;
    else if (key == "y") button = ButtonY;
    else if (key == "c") button = ButtonC;
    else if (key == "z") button = ButtonZ;
    else if (key == "d") button = ButtonD;
    else if (key == "start") button = ButtonStart;
    else if (key == "up") button = ButtonDpadUp;
    else if (key == "down") button = ButtonDpadDown;
    else if (key == "left") button = ButtonDpadLeft;
    else if (key == "right") button = ButtonDpadRight;
    else return false;
    return true;
}

bool parse_u8(const std::string& text, std::uint8_t& value) {
    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0' || parsed < 0 || parsed > 255) {
        return false;
    }
    value = static_cast<std::uint8_t>(parsed);
    return true;
}

bool set_axis_or_trigger(ControllerState& state,
                         const std::string& control,
                         std::uint8_t value) {
    const std::string key = normalized(control);
    if (key == "lx" || key == "leftx") state.left_x = value;
    else if (key == "ly" || key == "lefty") state.left_y = value;
    else if (key == "rx" || key == "rightx") state.right_x = value;
    else if (key == "ry" || key == "righty") state.right_y = value;
    else if (key == "lt" || key == "ltrigger" || key == "lefttrigger") state.left_trigger = value;
    else if (key == "rt" || key == "rtrigger" || key == "righttrigger") state.right_trigger = value;
    else return false;
    return true;
}

}  // namespace

ControllerInputParseResult apply_controller_input_command(ControllerState& state,
                                                          const std::string& line) {
    std::istringstream stream(line);
    std::string command;
    stream >> command;
    if (command.empty() || command[0] == '#') {
        return {ControllerInputParseStatus::Empty};
    }

    command = normalized(command);
    if (command == "neutral" || command == "reset") {
        state = ControllerState{};
        return {ControllerInputParseStatus::Ok};
    }

    if (command == "press" || command == "release") {
        std::string button_name;
        stream >> button_name;
        ControllerButton button = ButtonA;
        if (!parse_button(button_name, button)) {
            return {ControllerInputParseStatus::UnknownControl};
        }
        state.set_pressed(button, command == "press");
        return {ControllerInputParseStatus::Ok};
    }

    if (command == "set") {
        std::string control;
        std::string value_text;
        stream >> control >> value_text;
        std::uint8_t value = 0;
        if (!parse_u8(value_text, value)) {
            return {ControllerInputParseStatus::InvalidValue};
        }
        if (!set_axis_or_trigger(state, control, value)) {
            return {ControllerInputParseStatus::UnknownControl};
        }
        return {ControllerInputParseStatus::Ok};
    }

    return {ControllerInputParseStatus::UnknownCommand};
}

}  // namespace dreamcast

