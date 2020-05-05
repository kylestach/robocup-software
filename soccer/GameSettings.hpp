#include "Field_Dimensions.hpp"
#include "Network.hpp"

struct GameSettings {
    // True if we are running with a simulator.
    // This changes network communications.
    bool simulation = true;

    // True if we are blue.
    // False if we are yellow.
    bool requestedBlueTeam = true;

    // Robot being controlled by the joystick.
    // For now we are not supporting multiple manually controlled robots.
    int manualID = -1;

    //Board ID of the goalie robot
    int goalieID = 0;

    /// Measured framerate
    float framerate = 0;

    // joystick damping
    bool dampedRotation = false;
    bool dampedTranslation = false;

    bool kickOnBreakBeam = false;

    // If true, rotates robot commands from the joystick based on its
    // orientation on the field
    bool useFieldOrientedManualDrive = false;

    // Whether to allow an external referee
    bool allowExternalReferee = true;

    bool initialized = false;

    bool paused = false;

    int visionChannel = SharedVisionPort;

    // Field transforms/properties
    bool useOurHalf, useOpponentHalf;
    bool defendPlusX;

    Field_Dimensions dimensions;
};
