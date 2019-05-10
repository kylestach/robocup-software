#include "InterpolatedPath.hpp"
#include "LogUtils.hpp"
#include "Utils.hpp"
#include <protobuf/LogFrame.pb.h>
#include "SystemState.hpp"

#include <stdexcept>

using namespace std;
using namespace Geometry2d;

namespace Planning {

InterpolatedPath::InterpolatedPath(Pose p0) {
    waypoints.emplace_back(RobotInstant(p0, Geometry2d::Twist::Zero()), RJ::Seconds::zero());
}

InterpolatedPath::InterpolatedPath(Pose p0, Pose p1) {
    waypoints.emplace_back(RobotInstant(p0, Geometry2d::Twist::Zero()), 0ms);
    waypoints.emplace_back(RobotInstant(p1, Geometry2d::Twist::Zero()), 0ms);
}

float InterpolatedPath::length(unsigned int start) const {
    return length(start, waypoints.size() - 1);
}

float InterpolatedPath::length(unsigned int start, unsigned int end) const {
    if (waypoints.empty() || start >= (waypoints.size() - 1)) {
        return 0;
    }

    float length = 0;
    for (unsigned int i = start; i < end; ++i) {
        length += (waypoints[i + 1].pos() - waypoints[i].pos()).mag();
    }
    return length;
}

RobotInstant InterpolatedPath::start() const {
    return RobotInstant(waypoints.front().instant);
}

RobotInstant InterpolatedPath::end() const {
    return RobotInstant(waypoints.back().instant);
}

// Returns the index of the point in this path nearest to pt.
int InterpolatedPath::nearestIndex(Point pt) const {
    if (waypoints.size() == 0) {
        return -1;
    }

    int index = 0;
    float dist = pt.distTo(waypoints[0].pos());

    for (unsigned int i = 1; i < waypoints.size(); ++i) {
        float d = pt.distTo(waypoints[i].pos());
        if (d < dist) {
            dist = d;
            index = i;
        }
    }

    return index;
}

bool InterpolatedPath::hit(const Geometry2d::ShapeSet& obstacles,
                           RJ::Seconds startTimeIntoPath,
                           RJ::Seconds* hitTime) const {
    size_t start = 0;
    for (auto& entry : waypoints) {
        start++;
        if (entry.time > startTimeIntoPath) {
            start--;
            break;
        }
    }

    if (start >= waypoints.size()) {
        // Empty path or starting beyond end of path
        return false;
    }

    // This code disregards obstacles which the robot starts in. This allows the
    // robot to move out a obstacle if it is already in one.
    std::set<std::shared_ptr<Shape>> startHitSet =
        obstacles.hitSet(waypoints[start].pos());

    for (size_t i = start; i < waypoints.size() - 1; i++) {
        std::set<std::shared_ptr<Shape>> newHitSet = obstacles.hitSet(
            Segment(waypoints[i].pos(), waypoints[i + 1].pos()));
        if (!newHitSet.empty()) {
            for (std::shared_ptr<Shape> hit : newHitSet) {
                // If it hits something, check if the hit was in the original
                // hitSet
                if (startHitSet.find(hit) == startHitSet.end()) {
                    if (hitTime) {
                        *hitTime = waypoints[i].time;
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

float InterpolatedPath::distanceTo(Point pt) const {
    int i = nearestIndex(pt);
    if (i < 0) {
        return 0;
    }

    float dist = -1;
    for (unsigned int i = 0; i < (waypoints.size() - 1); ++i) {
        Segment s(waypoints[i].pos(), waypoints[i + 1].pos());
        const float d = s.distTo(pt);

        if (dist < 0 || d < dist) {
            dist = d;
        }
    }

    return dist;
}

Segment InterpolatedPath::nearestSegment(Point pt) const {
    Segment best;
    float dist = -1;
    if (waypoints.empty()) {
        return best;
    }

    for (unsigned int i = 0; i < (waypoints.size() - 1); ++i) {
        Segment s(waypoints[i].pos(), waypoints[i + 1].pos());
        const float d = s.distTo(pt);

        if (dist < 0 || d < dist) {
            best = s;
            dist = d;
        }
    }

    return best;
}

float InterpolatedPath::length(Point pt) const {
    float dist = -1;
    float length = 0;
    if (waypoints.empty()) {
        return 0;
    }

    for (unsigned int i = 0; i < (waypoints.size() - 1); ++i) {
        Segment s(waypoints[i].pos(), waypoints[i + 1].pos());

        // add the segment length
        length += s.length();

        const float d = s.distTo(pt);

        // if point closer to this segment
        if (dist < 0 || d < dist) {
            // closest point on segment
            Point p = s.nearestPoint(pt);

            // new best distance
            dist = d;

            // reset running length count
            length = 0;
            length += p.distTo(s.pt[1]);
        }
    }

    return length;
}

void InterpolatedPath::draw(SystemState* const state,
                            const QColor& col = Qt::black,
                            const QString& layer = "Motion") const {
    Packet::DebugRobotPath* dbg = state->logFrame->add_debug_robot_paths();
    dbg->set_layer(state->findDebugLayer(layer));

    if (waypoints.size() <= 1) {
        return;
    }

    for (const Entry& entry : waypoints) {
        Packet::DebugRobotPath::DebugRobotPathPoint* pt = dbg->add_points();
        *pt->mutable_pos() = entry.pos();
        *pt->mutable_vel() = entry.instant.velocity().linear();
    }
}

boost::optional<RobotInstant> InterpolatedPath::eval(RJ::Seconds t) const {
    if (waypoints.size() < 2 || t < waypoints[0].time) {
        return boost::none;
    }

    if (t < waypoints[0].time) {
        debugThrow(
            invalid_argument("The start time should not be less than zero"));
    }

    // Do a binary search to find the waypoints on either side.
    // Lower bound will find the first entry e such that e.time >= t, so it is the "end" of our segment.
    auto segment_end = std::lower_bound(waypoints.begin(), waypoints.end(), t, [](const Entry& e, RJ::Seconds t) {
        return e.time < t;
    });

    // Check if this is out of bounds.
    if (segment_end == waypoints.end() || segment_end - 1 == waypoints.end()) {
        return boost::none;
    }

    // Grab the two waypoints from the iterators.
    Entry w1 = *segment_end, w0 = *(segment_end - 1);

    RJ::Seconds deltaT = (w1.time - w0.time);
    if (deltaT == RJ::Seconds::zero()) {
        // dt is zero, so they should be the same. Return one arbitrarily.
        return RobotInstant(w0.instant);
    }

    double constant = (t - w0.time) / deltaT;

    return RobotInstant(
            (1 - constant) * w0.instant.pose().vector() + constant * w1.instant.pose().vector(),
            (1 - constant) * w0.instant.velocity().vector() + constant * w1.instant.velocity().vector()
            );
}

size_t InterpolatedPath::size() const { return waypoints.size(); }

RJ::Seconds InterpolatedPath::getTime(int index) const {
    return waypoints[index].time;
}

RJ::Seconds InterpolatedPath::getDuration() const {
    if (waypoints.size() > 0) {
        return waypoints.back().time;
    } else {
        return RJ::Seconds::zero();
    }
}

unique_ptr<Path> InterpolatedPath::subPath(RJ::Seconds startTime,
                                           RJ::Seconds endTime) const {
    // Check for valid arguments
    if (startTime < RJ::Seconds::zero()) {
        throw invalid_argument("InterpolatedPath::subPath(): startTime(" +
                               to_string(startTime) +
                               ") can't be less than zero");
    }

    if (endTime < RJ::Seconds::zero()) {
        throw invalid_argument("InterpolatedPath::subPath(): endTime(" +
                               to_string(endTime) +
                               ") can't be less than zero");
    }

    if (startTime > endTime) {
        throw invalid_argument(
            "InterpolatedPath::subPath(): startTime(" + to_string(startTime) +
            ") can't be after endTime(" + to_string(endTime) + ")");
    }

    if (startTime >= getDuration()) {
        debugThrow(invalid_argument(
            "InterpolatedPath::subPath(): startTime(" + to_string(startTime) +
            ") can't be greater than the duration(" + to_string(getDuration()) +
            ") of the path"));
        return unique_ptr<Path>(new InterpolatedPath());
    }

    if (startTime == RJ::Seconds::zero() && endTime >= getDuration()) {
        return this->clone();
    }

    // Do a binary search to find the first waypoint to include
    auto waypoint_iter = std::lower_bound(waypoints.begin(), waypoints.end(), startTime,
            [](const Entry& e, RJ::Seconds t) {
                return e.time < t;
            });

    // This corresponds to the case where startTime >= duration, and should never be triggered.
    assert(waypoint_iter != waypoints.end());

    auto subpath = make_unique<InterpolatedPath>();

    // Check if we can include an interpolated waypoint.
    if (startTime + RJ::Seconds(1e-9) < waypoint_iter->time) {
        // This case should be avoided via checks and verifications above which limit the range to only valid amounts.
        assert(waypoint_iter != waypoints.begin());

        // Grab the previous waypoint.
        Entry w0 = *(waypoint_iter - 1);
        Entry w1 = *waypoint_iter;
        RJ::Seconds dt = w1.time - w0.time;
        if (dt > RJ::Seconds(0)) {
            double constant = (startTime - w0.time) / dt;

            RobotInstant first_instant(
                    (1 - constant) * w0.instant.pose().vector() + constant * w1.instant.pose().vector(),
                    (1 - constant) * w0.instant.velocity().vector() + constant * w1.instant.velocity().vector()
            );
            subpath->addInstant(RJ::Seconds(0), first_instant);
        }
    }

    endTime = std::min(endTime, getDuration());

    subpath->setStartTime(this->startTime() + startTime);

    // Bound the endTime to a reasonable time.
    endTime = min(endTime, getDuration());

    while (waypoint_iter != waypoints.end() && waypoint_iter->time < endTime) {
        subpath->addInstant(waypoint_iter->time - startTime, waypoint_iter->instant);
    }

    // Handle the case where we need an interpolated waypoint after the last full segment.
    if (waypoint_iter != waypoints.end()) {
        // Assuming there are more than one waypoint in the original path, this iterator cannot be the first waypoint.
        assert(waypoint_iter != waypoints.begin());

        Entry w0 = *(waypoint_iter - 1);
        Entry w1 = *waypoint_iter;

        RJ::Seconds dt = w1.time - w0.time;
        if (dt > RJ::Seconds(0)) {
            double constant = (startTime - w0.time) / dt;

            RobotInstant last_instant(
                    (1 - constant) * w0.instant.pose().vector() + constant * w1.instant.pose().vector(),
                    (1 - constant) * w0.instant.velocity().vector() + constant * w1.instant.velocity().vector()
            );
            subpath->addInstant(endTime - startTime, last_instant);
        }
    }

    debugThrowIf(
        to_string(subpath->getDuration()) +
            to_string(std::min(getDuration() - startTime, endTime - startTime)),
        (subpath->getDuration() -
         std::min(getDuration() - startTime, endTime - startTime)).count() >
            0.00001);

    return std::move(subpath);
}

unique_ptr<Path> InterpolatedPath::clone() const {
    InterpolatedPath* cp = new InterpolatedPath();
    cp->waypoints = waypoints;
    cp->setStartTime(startTime());
    return std::unique_ptr<Path>(cp);
}

}  // namespace Planning
