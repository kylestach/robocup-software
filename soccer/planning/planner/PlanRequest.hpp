#pragma once

#include <map>
#include <memory>
#include <planning/MotionConstraints.hpp>
#include <planning/Trajectory.hpp>
#include <planning/planner/MotionCommand.hpp>
#include <utility>

#include "Context.hpp"
#include "WorldState.hpp"
#include "planning/DynamicObstacle.hpp"
#include "planning/Instant.hpp"
#include "planning/RobotConstraints.hpp"

namespace Planning {

/**
 * @brief Encapsulates information needed for planner to make a path
 *
 * @details This struct contains ALL information necessary for a single
 * robot path to be planned.
 */
struct PlanRequest {
    PlanRequest(RobotInstant start, MotionCommand command,
                RobotConstraints constraints,
                Geometry2d::ShapeSet field_obstacles,
                Geometry2d::ShapeSet virtual_obstacles,
                std::array<Trajectory*, Num_Shells> planned_trajectories,
                unsigned shellID, const WorldState* world_state,
                int8_t priority = 0, DebugDrawer* debug_drawer = nullptr)
        : start(start),
          motionCommand(command),
          constraints(constraints),
          field_obstacles(std::move(field_obstacles)),
          virtual_obstacles(std::move(virtual_obstacles)),
          planned_trajectories(planned_trajectories),
          shellID(shellID),
          priority(priority),
          world_state(world_state),
          debug_drawer(debug_drawer) {}

    /**
     * The robot's starting state.
     */
    RobotInstant start;

    /**
     * The goal to plan for.
     */
    MotionCommand motionCommand;

    /**
     * Angular and linear acceleration and velocity constraints on the robot.
     */
    RobotConstraints constraints;

    /**
     * The list of field obstacles.
     */
    Geometry2d::ShapeSet field_obstacles;

    /**
     * The list of "virtual" obstacles, generated by gameplay representing soft
     * constraints.
     */
    Geometry2d::ShapeSet virtual_obstacles;

    /**
     * Trajectories for each of the robots that has already been planned.
     * nullptr for unplanned robots.
     */
    std::array<Trajectory*, Num_Shells> planned_trajectories;

    /**
     * The robot's shell ID. Used for debug drawing.
     */
    unsigned shellID;

    /**
     * The state of the world, containing robot and ball states.
     *
     * For obstacle-avoidance purposes, obstacles should be used instead. This
     * can be used for lookup of robots/balls by ID.
     */
    const WorldState* world_state;

    /**
     * The priority of this plan request.
     */
    int8_t priority;

    /**
     * Allows debug drawing in the world. If this is nullptr, no debug drawing
     * should be performed.
     */
    DebugDrawer* debug_drawer;
};

/**
 * Fill the obstacle fields.
 *
 * @param in the plan request.
 * @param out_static an (empty) vector of static obstacles to be populated.
 *  This will be filled with field obstacles, local (virtual) obstacles,
 *  opponent robots, and our robots that have not yet been planned.
 * @param out_dynamic an (empty) vector of dynamic obstacles to be populated.
 *  This will be filled with trajectories for our robots that have already been
 *  planned.
 * @param avoid_ball whether or not to avoid the ball. If this is true,
 *  out_ball_trajectory should point to a valid trajectory.
 * @param ball_trajectory temporary storage for the ball trajectory. This must
 *  outlive the usage of out_dynamic. If avoid_ball == false, this should be
 *  nullptr.
 */
void FillObstacles(const PlanRequest& in, Geometry2d::ShapeSet* out_static,
                   std::vector<DynamicObstacle>* out_dynamic, bool avoid_ball,
                   Trajectory* out_ball_trajectory = nullptr);

}  // namespace Planning
