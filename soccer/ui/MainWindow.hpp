#pragma once

#include <optional>

#include <QMainWindow>
#include <QTime>
#include <QTimer>

#include <Configuration.hpp>
#include "FieldView.hpp"

#include "Processor.hpp"
#include "rc-fshare/rtp.hpp"
#include "ui_MainWindow.h"
#include "ControlBar.hpp"

class TestResultTab;
class StripChart;
class ConfigBool;

enum Side { Yellow, Blue };
/**
 * main gui thread class
 */
class MainWindow : public QMainWindow {
    Q_OBJECT;

public:
    MainWindow(Processor* processor, QWidget* parent = nullptr);

    void configuration(Configuration* config);

    void initialize();

    SystemState* state() { return _processor->state(); }

    /// Deselects all debug layers
    void allDebugOff();

    /// Selects all debug layers
    void allDebugOn();

    // Call this to update the status bar when the log file has changed
    void logFileChanged();

    QTimer updateTimer;

    void setUseRefChecked(bool use_ref);

private Q_SLOTS:
    void addLayer(int i, QString name, bool checked);
    void updateViews();

    void on_fieldView_robotSelected(int shell);
    void on_actionRawBalls_toggled(bool state);
    void on_actionRawRobots_toggled(bool state);
    void on_actionCoords_toggled(bool state);
    void on_actionDotPatterns_toggled(bool state);
    void on_actionTeam_Names_toggled(bool state);
    void on_actionTeamYellow_triggered();
    void on_actionTeamBlue_triggered();

    void on_actionUse_Field_Oriented_Controls_toggled(bool value);
    void on_actionUse_Multiple_Joysticks_toggled(bool value);

    void on_actionUse_External_Referee_toggled(bool value);

    /// Field side
    void on_actionDefendPlusX_triggered();
    void on_actionDefendMinusX_triggered();
    void on_actionUseOurHalf_toggled(bool value);
    void on_actionUseOpponentHalf_toggled(bool value);

    /// Field rotation
    void on_action0_triggered();
    void on_action90_triggered();
    void on_action180_triggered();
    void on_action270_triggered();

    /// Vision port
    void on_actionVisionPrimary_Half_triggered();
    void on_actionVisionSecondary_Half_triggered();
    void on_actionVisionFull_Field_triggered();

    /// Simulator commands
    void on_actionCenterBall_triggered();
    void on_actionStopBall_triggered();
    void on_actionResetField_triggered();

    /// Style Sheets
    void on_actionNoneStyle_triggered();
    void on_actionDarkStyle_triggered();
    void on_actionDarculizedStyle_triggered();
    void on_action1337h4x0rStyle_triggered();
    void on_actionNyanStyle_triggered();

    /// Manual control commands
    void on_actionDampedRotation_toggled(bool value);
    void on_actionDampedTranslation_toggled(bool value);

    /// Debug menu commands
    void on_actionRestartUpdateTimer_triggered();
    void on_actionStart_Logging_triggered();

    /// Gameplay menu
    void on_actionSeed_triggered();

    // Joystick settings
    void on_joystickKickOnBreakBeam_stateChanged();

    /// Debug layers
    void on_debugLayers_itemChanged(QListWidgetItem* item);
    void on_debugLayers_customContextMenuRequested(const QPoint& pos);

    /// Testing Tab
    void on_testRun_clicked();
    void on_addToTable_clicked();
    void on_removeFromTable_clicked();
    void on_testNext_clicked();

    /// Configuration
    void on_configTree_itemChanged(QTreeWidgetItem* item, int column);
    void on_loadConfig_clicked();
    void on_saveConfig_clicked();

private:
    void updateFromRefPacket(bool haveExternalReferee);
    static std::string formatLabelBold(Side side, std::string label);

    void updateRadioBaseStatus(bool usbRadio);
    void channel(int n);

    Ui_MainWindow _ui;
    const QStandardItemModel* goalieModel;

    Processor* const _processor;
    Configuration* _config;

    // Log history, copied from Logger.
    // This is used by other controls to get log data without having to copy it
    // again from the Logger.
    std::vector<std::shared_ptr<Packet::LogFrame> > _history;

    // Longer log history, copied from Logger.
    // This is used specificially via StripChart and ProtobufTree
    // To export a larger amount of data.
    std::vector<std::shared_ptr<Packet::LogFrame> > _longHistory;

    // When true, External Referee is automatically set.
    // This is cleared by manually changing the checkbox or after the
    // first referee packet is seen and the box is automatically checked.
    bool _autoExternalReferee;

    // Tree items that are not in LogFrame
    QTreeWidgetItem* _frameNumberItem;
    QTreeWidgetItem* _elapsedTimeItem;

    /// playback rate of the viewer - a value of 1 means realtime
    std::optional<double> _playbackRate;

    // This is used to update some status items less frequently than the full
    // field view
    int _updateCount;

    RJ::Time _lastUpdateTime;

    QLabel* _currentPlay;
    QLabel* _logFile;
    QLabel* _viewFPS;
    QLabel* _procFPS;
    QLabel* _logMemory;

    // QActionGroups for Radio Menu Actions
    std::map<std::string, QActionGroup*> qActionGroups{};

    // maps robot shell IDs to items in the list
    std::map<int, QListWidgetItem*> _robotStatusItemMap{};

    ControlBar* _control_bar;
};
