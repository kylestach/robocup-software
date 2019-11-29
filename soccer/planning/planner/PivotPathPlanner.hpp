#pragma once

#include "Planner.hpp"

namespace Planning {
    
class PivotPathPlanner: public PlannerForCommandType<PivotCommand> {
public:
    static void createConfiguration(Configuration* cfg);
    Trajectory plan(PlanRequest&& request) override;
    std::string name() const override {return "PivotPathPlanner";}
private:
    bool targetChanged(const PlanRequest& request) const;
    static ConfigDouble* _pivotRadiusMultiplier;
};
}