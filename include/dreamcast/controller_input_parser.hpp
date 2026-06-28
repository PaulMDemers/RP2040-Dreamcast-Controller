#pragma once

#include <string>

#include "dreamcast/dreamcast_controller.hpp"

namespace dreamcast {

enum class ControllerInputParseStatus {
    Ok,
    Empty,
    UnknownCommand,
    UnknownControl,
    InvalidValue,
};

struct ControllerInputParseResult {
    ControllerInputParseStatus status = ControllerInputParseStatus::Empty;
};

ControllerInputParseResult apply_controller_input_command(ControllerState& state,
                                                          const std::string& line);

}  // namespace dreamcast

