#pragma once
#include <Geometry2d/ShapeSet.hpp>
#include <vector>

#include "Planner.hpp"
#include "planning/low_level/Replanner.hpp"
#include "planning/low_level/VelocityProfiling.hpp"

namespace Planning {

class PathTargetPlanner : public PlannerForCommandType<PathTargetCommand> {
public:
    PathTargetPlanner() : PlannerForCommandType("PathTargetPlanner") {}
    ~PathTargetPlanner() override = default;

    Trajectory plan(PlanRequest&& request) override;
    void reset() override { previous = Trajectory(); }

    double drawRadius = Robot_Radius;
    QColor drawColor = Qt::black;
    QString drawLayer = "Planning";

private:
    [[nodiscard]] static AngleFunction getAngleFunction(const PlanRequest& request);

    Replanner replanner;
    Trajectory previous;
};

}  // namespace Planning
