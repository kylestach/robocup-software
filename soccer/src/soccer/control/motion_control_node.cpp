#include "motion_control_node.hpp"

#include <spdlog/spdlog.h>

#include "robot.hpp"

namespace control {

MotionControlNode::MotionControlNode()
    : rclcpp::Node("control"),
      param_provider_(this, params::kMotionControlParamModule) {
    controllers_.reserve(kNumShells);
    for (int i = 0; i < kNumShells; i++) {
        controllers_.emplace_back(i, this, nullptr);
        manipulators_.emplace_back(i, this);
    }
}

} // namespace control