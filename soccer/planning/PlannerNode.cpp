#include "PlannerNode.hpp"

#include "Instant.hpp"
#include "Robot.hpp"
#include "planning/planner/CollectPlanner.hpp"
#include "planning/planner/EscapeObstaclesPathPlanner.hpp"
#include "planning/planner/LineKickPlanner.hpp"
#include "planning/planner/PathTargetPlanner.hpp"
#include "planning/planner/PivotPathPlanner.hpp"
#include "planning/planner/SettlePlanner.hpp"

namespace Planning {

PlannerNode::PlannerNode(Context* context) : context_(context) {
    robots_planners_.resize(Num_Shells);
}

using namespace Geometry2d;
void PlannerNode::run() {
    const ShapeSet& global_obstacles = context_->globalObstacles;
    const WorldState& world_state = context_->world_state;
    const auto& robot_intents = context_->robot_intents;
    DebugDrawer* debug_drawer = &context_->debug_drawer;
    auto* trajectories = &context_->trajectories;

    if (context_->game_state.state == GameState::Halt) {
        context_->trajectories.fill(Trajectory());
        return;
    }

    std::array<Trajectory*, Num_Shells> planned{};

    // Sort the robots by priority
    std::array<int, Num_Shells> shells{};
    for (int i = 0; i < Num_Shells; i++) {
        shells[i] = i;
    }
    std::sort(shells.begin(), shells.end(), [&] (int a, int b) {
        return robot_intents.at(a).priority > robot_intents.at(b).priority;
    });

    for (int i = 0; i < Num_Shells; i++) {
        unsigned int shell = shells.at(i);
        const auto& robot = world_state.our_robots.at(shell);
        const auto& intent = robot_intents.at(shell);

        if (!robot.visible) {
            // For invalid robots, early return with an empty path.
            trajectories->at(shell) = Trajectory();
            planned.at(shell) = &trajectories->at(shell);
            continue;
        }

        RobotInstant start{robot.pose, robot.velocity, robot.timestamp};

        // TODO: Put motion constraints in intent.
        PlanRequest request{
            start,
            intent.motion_command,
            RobotConstraints(),
            global_obstacles,
            intent.local_obstacles,
            planned,
            shell,
            &world_state,
            intent.priority,
            debug_drawer
        };

        Trajectory trajectory = robots_planners_.at(shell).PlanForRobot(std::move(request));
        trajectory.draw(&context_->debug_drawer);
        trajectories->at(shell) = std::move(trajectory);

        planned.at(shell) = &trajectories->at(shell);
    }
}

PlannerForRobot::PlannerForRobot()
    : planner_idx_(-1) {
    planners_.push_back(std::make_unique<PathTargetPlanner>());
    planners_.push_back(std::make_unique<SettlePlanner>());
    planners_.push_back(std::make_unique<CollectPlanner>());
    planners_.push_back(std::make_unique<LineKickPlanner>());
    planners_.push_back(std::make_unique<PivotPathPlanner>());

    // The empty planner should always be last.
    planners_.push_back(std::make_unique<EscapeObstaclesPathPlanner>());
}


Trajectory PlannerForRobot::PlanForRobot(Planning::PlanRequest&& request) {
    // Try each planner in sequence until we find one that is applicable.
    // This gives the planners a sort of "priority" - this makes sense, because
    // the empty planner is always last.
    Trajectory trajectory;
    for (auto& planner : planners_) {
        // If this planner could possibly plan for this command, try to make
        // a plan.
        if (trajectory.empty() &&
            planner->isApplicable(request.motionCommand)) {
            RobotInstant startInstant = request.start;
            RJ::Time t0 = RJ::now();
            trajectory = planner->plan(std::move(request));
        }

        // If it fails, or if the planner was not used, the trajectory will
        // still be empty. Reset the planner.
        if (trajectory.empty()) {
            planner->reset();
        } else {
            if (!trajectory.angles_valid()) {
                throw std::runtime_error(
                    "Trajectory returned from " + planner->name() + " has no angle profile!");
            }

            if (!trajectory.timeCreated().has_value()) {
                throw std::runtime_error(
                    "Trajectory returned from " + planner->name() + " has no timestamp!");
            }
        }
    }

    if (trajectory.empty()) {
        std::cerr
            << "No valid planner! Did you forget to specify a default planner?"
            << std::endl;
        trajectory = Trajectory {{request.start}};
        trajectory.setDebugText("Error: No Valid Planners");
    }

    return trajectory;
}

}  // namespace Planning

