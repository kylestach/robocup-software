#pragma once

#include <QWidget>
#include <Processor.hpp>

namespace {
// Style sheets used for live/non-live controls
    QString LiveStyle("border:2px solid transparent");
    QString NonLiveStyle("border:2px solid red");
};  // namespace

class Ui_MainWindow;

class ControlBar : public QWidget {
    Q_OBJECT;

public:
    ControlBar(QWidget* parent = nullptr);

    bool live() const;
    void setLive();
    void setPlayBackRate(double playbackRate);
    int frameNumber() const { return roundf(_doubleFrameNumber); }
    void frameNumber(int value) { _doubleFrameNumber = value; }

private Q_SLOTS:
    /// Log controls
    void on_logHistoryLocation_sliderMoved(int value);
    void on_logHistoryLocation_sliderReleased();
    void on_logHistoryLocation_sliderPressed();
    void on_logPlaybackRewind_clicked();
    void on_logPlaybackPrevFrame_clicked();
    void on_logPlaybackPause_clicked();
    void on_logPlaybackNextFrame_clicked();
    void on_logPlaybackPlay_clicked();
    void on_logPlaybackLive_clicked();

    // Fast Ref Buttons
    void on_fastHalt_clicked();
    void on_fastStop_clicked();
    void on_fastReady_clicked();
    void on_fastForceStart_clicked();
    void on_fastKickoffBlue_clicked();
    void on_fastKickoffYellow_clicked();
    void on_fastDirectBlue_clicked();
    void on_fastDirectYellow_clicked();
    void on_fastIndirectBlue_clicked();
    void on_fastIndirectYellow_clicked();

    void on_manualID_currentIndexChanged(int value);
    void on_goalieID_currentIndexChanged(int value);

private:
    typedef enum { Status_OK, Status_Warning, Status_Fail } StatusType;

    void updateStatus();
    void status(QString text, StatusType status);

    // Tracking fractional frames is the easiest way to allow arbitrary playback
    // rates. To keep rounding consistent, only access this with frameNumber().
    double _doubleFrameNumber;

    Ui_MainWindow* _ui;
    Processor* _processor;
    std::optional<double> _playbackRate;
};
