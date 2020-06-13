#include "Planner.hpp"

#include "planning/Instant.hpp"
#include "planning/Trajectory.hpp"

namespace Planning {

#if 0
Trajectory Planner::reuse(RJ::Time now, RobotInstant start, Trajectory previous) {
    if (previous.empty()) {
        Trajectory out({start});
        out.setDebugText("Empty");
        return std::move(out);
    }
    RJ::Seconds timeElapsed = now - previous.begin_time();
    if (timeElapsed < previous.duration()) {
        previous.trimFront(timeElapsed);
        Trajectory out = std::move(previous);
        out.setDebugText("Reuse");
        return std::move(out);
    }
    Trajectory out{{previous.last()}};
    out.setDebugText("Reusing Past End");
    return std::move(out);
}
#endif

}  // namespace Planning