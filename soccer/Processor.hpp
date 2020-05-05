
// FIXME - Move a lot of stuff like blueTeam and worldToTeam to a globally
// accessible place

#pragma once

#include <protobuf/LogFrame.pb.h>
#include <string.h>

#include <Geometry2d/Point.hpp>
#include <Geometry2d/Pose.hpp>
#include <Geometry2d/TransformMatrix.hpp>
#include <Logger.hpp>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <Referee.hpp>
#include <SystemState.hpp>
#include <optional>
#include <vector>

#include "GrSimCommunicator.hpp"
#include "Node.hpp"
#include "VisionReceiver.hpp"
#include "motion/MotionControlNode.hpp"
#include "radio/Radio.hpp"
#include "radio/RadioNode.hpp"
#include "rc-fshare/rtp.hpp"

class Configuration;
class RobotStatus;
class Joystick;
struct JoystickControlValues;
class Radio;
class VisionFilter;

namespace Gameplay {
class GameplayModule;
}

namespace Planning {
class MultiRobotPathPlanner;
}

/**
 * @brief Brings all the pieces together
 *
 * @details The processor ties together all the moving parts for controlling
 * a team of soccer robots.  Its responsibities include:
 * - receiving and handling vision packets (see VisionReceiver)
 * - receiving and handling referee packets (see RefereeModule)
 * - radio IO (see Radio)
 * - running the BallTracker
 * - running the Gameplay::GameplayModule
 * - running the Loggercout
 * - handling the Configuration
 * - handling the Joystick
 * - running motion control for each robot (see OurRobot#motionControl)
 */
class Processor {
public:
    struct Status {
        Status() {}

        RJ::Time lastLoopTime;
        RJ::Time lastVisionTime;
        RJ::Time lastRefereeTime;
        RJ::Time lastRadioRxTime;
    };

    static void createConfiguration(Configuration* cfg);

    Processor(bool sim, bool defendPlus,
              bool blueTeam, std::string readLogFile);
    virtual ~Processor();

    /**
     * Stop the execution of Processor, from another thread.
     */
    void stop();

    // Joystick control
    JoystickControlValues getJoystickControlValue(Joystick& joy);
    std::vector<JoystickControlValues> getJoystickControlValues();
    std::vector<int> getJoystickRobotIds();

    // Module/Context getters
    // These should not exist, and are temporary hacks to expose functionality
    // that is not yet supported by Context
    std::shared_ptr<Gameplay::GameplayModule> gameplayModule() const {
        return _gameplayModule;
    }
    std::shared_ptr<Referee> refereeModule() const { return _refereeModule; }
    SystemState* state() { return &_context.state; }
    const Logger& logger() const { return _logger; }
    Radio* radio() { return _radio->getRadio(); }

    Status status() {
        QMutexLocker lock(&_statusMutex);
        return _status;
    }

    /**
     * Open the given log filename
     * @param filename
     * @return true if it was successfully opened.
     */
    bool openLog(const QString& filename) { return _logger.open(filename); }

    /**
     * Close the file descriptor associated with the current log file, if one
     * exists.
     */
    void closeLog() { _logger.close(); }

    /**
     * @return a mutex controlling access to the Context struct.
     *  Holding this mutex prevents Processor's loop from running.
     */
    QMutex* loopMutex() { return &_loopMutex; }

    // Time of the first LogFrame
    std::optional<RJ::Time> firstLogTime;

    Context* context() { return &_context; }

    /**
     * @return Whether the system is initialized.
     */
    bool isInitialized() const;

    void run();

private:
    /**
     * Recalculate the world-to-team transform. This should be run whenever
     * defendPlusX is changed.
     */
    void recalculateWorldToTeamTransform(const Field_Dimensions& field_dims,
                                         double field_angle);

    // Configuration for the robot.
    // TODO(Kyle): Add back in configuration values for different years.
    static std::unique_ptr<RobotConfig> robot_config_init;

    /**
     * Apply the specified joystick control values to a robot.
     */
    void applyJoystickControls(const JoystickControlValues& controlVals,
                               OurRobot* robot);

    /**
     * Whether or not there exists a valid joystick.
     */
    bool joystickValid() const;

    /**
     * Send out the radio data for the radio program
     */
    void sendRadioData();

    /**
     * Run the vision models against our backlog of camera frames.
     */
    void runModels();

    // Helpers for dealing with change conditions (i.e. triggering a resize when
    // field dimensions change)

    /**
     * Update field dimensions, if the value in Context has changed.
     */
    void updateFieldDimensions(const Field_Dimensions& dims);
    Field_Dimensions _last_dimensions;
    void setFieldDimensions(const Field_Dimensions& dims);

    /**
     * Update the orientation, if it has changed.
     */
    void updateOrientation(bool defendPlusX);
    bool _last_defend_plus_x;
    double _teamAngle;

    /**
     * Used to start and stop the thread
     */
    std::atomic_bool _running;

    Logger _logger;

    /**
     * Locked when processing loop stuff is happening (not when blocked for
     * timing or I/O). This is public so the GUI thread can lock it to access
     * fields in Context.
     */
    QMutex _loopMutex;

    /**
     * Global system state.
     */
    Context _context;

    // Processing period in microseconds
    RJ::Seconds _framePeriod = RJ::Seconds(1) / 60;

    // This is used by the GUI to indicate status of the processing loop and
    // network
    QMutex _statusMutex;
    Status _status;

    // modules
    std::shared_ptr<VisionFilter> _vision;
    std::shared_ptr<Referee> _refereeModule;
    std::shared_ptr<Gameplay::GameplayModule> _gameplayModule;
    std::unique_ptr<Planning::MultiRobotPathPlanner> _pathPlanner;
    std::unique_ptr<VisionReceiver> _visionReceiver;
    std::unique_ptr<MotionControlNode> _motionControl;
    std::unique_ptr<RadioNode> _radio;
    std::unique_ptr<GrSimCommunicator> _grSimCom;

    std::vector<Node*> _nodes;

    // joystick control
    std::vector<Joystick*> _joysticks;
};
