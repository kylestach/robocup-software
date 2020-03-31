#include "ui/ControlBar.hpp"
#include "ui_MainWindow.h"

ControlBar::ControlBar(Processor* processor, Ui_MainWindow* ui, QWidget* parent)
    : _processor(processor),
      _ui(ui),
      _doubleFrameNumber(0) {
    connect(_ui->manualID, SIGNAL(currentIndexChanged(int)), this,
            SLOT(on_manualID_currentIndexChanged(int)));
}

// Fast ref buttons
void ControlBar::on_fastHalt_clicked() {
    _processor->refereeModule()->command_ = RefereeModuleEnums::HALT;
}

void ControlBar::on_fastStop_clicked() {
    _processor->refereeModule()->command_ = RefereeModuleEnums::STOP;
}

void ControlBar::on_fastReady_clicked() {
    _processor->refereeModule()->command_ = RefereeModuleEnums::NORMAL_START;
}

void ControlBar::on_fastForceStart_clicked() {
    _processor->refereeModule()->command_ = RefereeModuleEnums::FORCE_START;
}

void ControlBar::on_fastKickoffBlue_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::PREPARE_KICKOFF_BLUE;
}

void ControlBar::on_fastKickoffYellow_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::PREPARE_KICKOFF_YELLOW;
}

void ControlBar::on_fastDirectBlue_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::DIRECT_FREE_BLUE;
}

void ControlBar::on_fastDirectYellow_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::DIRECT_FREE_YELLOW;
}

void ControlBar::on_fastIndirectBlue_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::INDIRECT_FREE_BLUE;
}

void ControlBar::on_fastIndirectYellow_clicked() {
    _processor->refereeModule()->command_ =
        RefereeModuleEnums::INDIRECT_FREE_YELLOW;
}

// Log controls
void ControlBar::on_logHistoryLocation_sliderMoved(int value) {
    // Sync frameNumber with logHistory slider
    _doubleFrameNumber = value;

    // pause playback
    setPlayBackRate(0);
}

void ControlBar::on_logHistoryLocation_sliderPressed() {
    on_logHistoryLocation_sliderMoved(_ui->logHistoryLocation->value());
}

void ControlBar::on_logHistoryLocation_sliderReleased() {
    on_logHistoryLocation_sliderPressed();
}

void ControlBar::on_logPlaybackRewind_clicked() {
    if (live()) {
        setPlayBackRate(-1);
    } else {
        *_playbackRate += -0.5;
    }
}

void ControlBar::on_logPlaybackPrevFrame_clicked() {
    setPlayBackRate(0);
    _doubleFrameNumber -= 1;
}

void ControlBar::on_logPlaybackPause_clicked() {
    if (live() || std::abs(*_playbackRate) > 0.1) {
        setPlayBackRate(0);
    } else {
        setPlayBackRate(1);
    }
}

void ControlBar::on_logPlaybackNextFrame_clicked() {
    setPlayBackRate(0);
    _doubleFrameNumber += 1;
}

void ControlBar::on_logPlaybackPlay_clicked() {
    if (!live()) {
        *_playbackRate += 0.5;
    }
}

void ControlBar::on_logPlaybackLive_clicked() { setLive(); }

void ControlBar::status(QString text, ControlBar::StatusType status) {
    // Assume that the status type alone won't change.
    if (_ui->statusLabel->text() != text) {
        _ui->statusLabel->setText(text);

        switch (status) {
            case Status_OK:
                _ui->statusLabel->setStyleSheet("background-color: #00ff00");
                break;

            case Status_Warning:
                _ui->statusLabel->setStyleSheet("background-color: #ffff00");
                break;

            case Status_Fail:
                _ui->statusLabel->setStyleSheet("background-color: #ff4040");
                break;
        }
    }
}

void ControlBar::updateStatus() {
    // Guidelines:
    //    Status_Fail is used for severe, usually external, errors such as
    //    hardware or network failures.
    //    Status_Warning is used for configuration problems that prevent
    //    competition operation.
    //        These can be easily changed within the soccer program.
    //    Status_OK shall only be used for "COMPETITION".
    //
    // The order of these checks is important to help debugging.
    // More specific or unlikely problems should be tested earlier.

    if (!_processor) {
        status("NO PROCESSOR", Status_Fail);
        return;
    }

    if (_processor->gameplayModule()->checkPlaybookStatus()) {
        playIndicatorStatus(false);
    }

    // Some conditions are different in simulation
    bool sim = _processor->simulation();

    if (!sim) {
        updateRadioBaseStatus(_processor->isRadioOpen());
    }

    // Get processing thread status
    Processor::Status ps = _processor->status();
    RJ::Time curTime = RJ::now();

    // Determine if we are receiving packets from an external referee
    bool haveExternalReferee = (curTime - ps.lastRefereeTime) < RJ::Seconds(1);

    std::vector<int> validIds = _processor->state()->ourValidIds();

    for (int i = 1; i <= Num_Shells; i++) {
        QStandardItem* item = goalieModel->item(i);
        if (std::find(validIds.begin(), validIds.end(), i - 1) !=
            validIds.end()) {
            // The list starts with None so i is 1 higher than the shell id
            item->setFlags(item->flags() |
                           (Qt::ItemIsSelectable | Qt::ItemIsEnabled));
        } else {
            item->setFlags(item->flags() &
                           ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
        }
    }

    if (haveExternalReferee && _autoExternalReferee) {
        // External Ref is connected and should be used
        _ui->fastHalt->setEnabled(false);
        _ui->fastStop->setEnabled(false);
        _ui->fastReady->setEnabled(false);
        _ui->fastForceStart->setEnabled(false);
        _ui->fastKickoffBlue->setEnabled(false);
        _ui->fastKickoffYellow->setEnabled(false);
        _ui->fastDirectBlue->setEnabled(false);
    } else {
        _ui->fastHalt->setEnabled(true);
        _ui->fastStop->setEnabled(true);
        _ui->fastReady->setEnabled(true);
        _ui->fastForceStart->setEnabled(true);
        _ui->fastKickoffBlue->setEnabled(true);
        _ui->fastKickoffYellow->setEnabled(true);
        _ui->fastDirectBlue->setEnabled(true);
    }

    updateFromRefPacket(haveExternalReferee);

    // Is the processing thread running?
    if (curTime - ps.lastLoopTime > RJ::Seconds(0.1)) {
        // Processing loop hasn't run recently.
        // Likely causes:
        //    Mutex deadlock (need a recursive mutex?)
        //    Excessive computation
        status("PROCESSING HUNG", Status_Fail);
        return;
    }

    // Check network activity
    if (curTime - ps.lastVisionTime > RJ::Seconds(0.1)) {
        // We must always have vision
        status("NO VISION", Status_Fail);
        return;
    }

    if (_processor->manualID() >= 0) {
        // Mixed auto/manual control
        status("MANUAL", Status_Warning);
        return;
    }

    // Driving the robots helps isolate radio problems by verifying radio TX,
    // so test this after manual driving.
    if (curTime - ps.lastRadioRxTime > RJ::Seconds(1)) {
        // Allow a long timeout in case of poor radio performance
        status("NO RADIO RX", Status_Fail);
        return;
    }

    if ((!sim || _processor->externalReferee()) && !haveExternalReferee) {
        if (_autoExternalReferee && _processor->externalReferee()) {
            // Automatically turn off external referee
            //_ui->externalReferee->setChecked(false);
        } else {
            // In simulation, we will often run without a referee, so just make
            // it a warning.
            // There is a separate status for non-simulation with internal
            // referee.
            status("NO REFEREE", Status_Fail);
            return;
        }
    }

    if (sim) {
        // Everything is good for simulation, but not for competition.
        status("SIMULATION", Status_Warning);
        return;
    }

    if (!sim && !_processor->externalReferee()) {
        // Competition must use external referee
        status("INTERNAL REF", Status_Warning);
        return;
    }

    if (!sim && !_processor->logger().recording()) {
        // We should record logs during competition
        status("NOT RECORDING", Status_Warning);
        return;
    }

    status("COMPETITION", Status_OK);
}

void ControlBar::on_manualID_currentIndexChanged(int value) {
    _processor->manualID(value - 1);
}

void ControlBar::on_goalieID_currentIndexChanged(int value) {
    _processor->goalieID(value - 1);
}

bool ControlBar::live() const { return !_playbackRate; }

void ControlBar::setLive() {
    if (!live()) {
        _ui->logTree->setStyleSheet(
                QString("QTreeWidget{%1}").arg(NonLiveStyle));
        _playbackRate = std::nullopt;
    }
}

void ControlBar::setPlayBackRate(double playbackRate) {
    if (live()) {
        _ui->logTree->setStyleSheet(
                QString("QTreeWidget{%1}").arg(LiveStyle));
    }
    _playbackRate = playbackRate;
}

