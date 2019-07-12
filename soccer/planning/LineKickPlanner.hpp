#pragma once

#include <Geometry2d/Point.hpp>
#include "RRTPlanner.hpp"
#include "SingleRobotPathPlanner.hpp"
class Configuration;
class ConfigDouble;

namespace Planning {

/**
 * Planner which plans a path to line kick a ball.
 * Uses the System State object to get the position of the ball
 * and predict its Path. It chooses the closest intersection point
 * with the ball Path it can reach in time and plans a Path so the
 * ball and robot intersect at the same time.
 *
 * TODO(ashaw596): Fix bug with replanning on real robots.
 */
class LineKickPlanner : public SingleRobotPathPlanner {
public:
    LineKickPlanner(Context context) : SingleRobotPathPlanner(false), rrtPlanner(context, 0, 250), context(context) {};
    virtual std::unique_ptr<Path> run(PlanRequest& planRequest) override;

    virtual MotionCommand::CommandType commandType() const override {
        return MotionCommand::LineKick;
    }

private:
    bool shouldReplan(const PlanRequest& planRequest) const;

    Context context;

    RRTPlanner rrtPlanner;
    bool finalApproach = false;
    boost::optional<Geometry2d::Point> targetKickPos;
    int reusePathCount = 0;
};

}  // namespace Planning
