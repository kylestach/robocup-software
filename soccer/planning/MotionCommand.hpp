#pragma once

#include <Geometry2d/Point.hpp>
#include <boost/optional.hpp>
#include "planning/MotionInstant.hpp"
#include "Utils.hpp"

namespace Planning {

/*
 * This is a superclass for different MotionCommands.
 * Currently implemented are PathTarget, WorldVel, Pivot, DirectPathtarget,
 * TuningPath, None
 */
class MotionCommand {
public:
    enum CommandType {
        PathTarget,
        WorldVel,
        Pivot,
        DirectPathTarget,
        TuningPath,
        LineKick,
        None
    };
    virtual ~MotionCommand() = default;
    CommandType getCommandType() const { return commandType; }
    virtual std::unique_ptr<Planning::MotionCommand> clone() const = 0;

protected:
    MotionCommand(const MotionCommand& that) = default;
    MotionCommand(CommandType command) : commandType(command) {}

private:
    // The type of command
    const CommandType commandType;
};

struct EmptyCommand : public MotionCommand {
    EmptyCommand() : MotionCommand(MotionCommand::None){};

    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<EmptyCommand>();
    }
};

struct PathTargetCommand : public MotionCommand {
    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<PathTargetCommand>(*this);
    }
    explicit PathTargetCommand(const RobotInstant& goal)
        : MotionCommand(MotionCommand::PathTarget), pathGoal(goal){};
    explicit PathTargetCommand(const Geometry2d::Pose& goal)
            : MotionCommand(MotionCommand::PathTarget), pathGoal(goal, Geometry2d::Twist::Zero()){};
    const RobotInstant pathGoal;
};

struct WorldVelTargetCommand : public MotionCommand {
    explicit WorldVelTargetCommand(Geometry2d::Point vel)
        : MotionCommand(MotionCommand::WorldVel), worldVel(vel, 0){};
    explicit WorldVelTargetCommand(Geometry2d::Twist vel)
            : MotionCommand(MotionCommand::WorldVel), worldVel(vel){};
    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<WorldVelTargetCommand>(*this);
    }
    const Geometry2d::Twist worldVel;
};
struct PivotCommand : public MotionCommand {
    explicit PivotCommand(Geometry2d::Point pivotPoint,
                          Geometry2d::Point target, float radius)
        : MotionCommand(MotionCommand::Pivot),
          pivotPoint(pivotPoint),
          pivotTarget(target),
          radius(radius) {}

    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<PivotCommand>(*this);
    }

    Geometry2d::Point pivotTarget;
    Geometry2d::Point pivotPoint;
    float radius;
};

struct DirectPathTargetCommand : public MotionCommand {
    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<DirectPathTargetCommand>(*this);
    }
    explicit DirectPathTargetCommand(const RobotInstant& goal)
        : MotionCommand(MotionCommand::DirectPathTarget), pathGoal(goal){};
    explicit DirectPathTargetCommand(const Geometry2d::Pose& goal)
            : MotionCommand(MotionCommand::DirectPathTarget), pathGoal(goal, Geometry2d::Twist::Zero()) {};
    explicit DirectPathTargetCommand(
            const Geometry2d::Point& goal_position,
            const Geometry2d::Point& goal_vel = Geometry2d::Point(0, 0))
            : MotionCommand(MotionCommand::DirectPathTarget),
              pathGoal(Geometry2d::Pose(goal_position, 0), Geometry2d::Twist(goal_vel, 0)) {};
    const RobotInstant pathGoal;
};

struct TuningPathCommand : public MotionCommand {
    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<TuningPathCommand>(*this);
    }
    explicit TuningPathCommand(const RobotInstant& goal)
        : MotionCommand(MotionCommand::TuningPath), pathGoal(goal){};
    const RobotInstant pathGoal;
};

struct LineKickCommand : public MotionCommand {
    explicit LineKickCommand(Geometry2d::Point target)
        : MotionCommand(MotionCommand::LineKick), target(target){};

    virtual std::unique_ptr<Planning::MotionCommand> clone() const override {
        return std::make_unique<LineKickCommand>(*this);
    };

    const Geometry2d::Point target;
};
}  // namespace Planning
