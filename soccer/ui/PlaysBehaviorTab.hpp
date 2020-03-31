#pragma once

#include <QWidget>

#include "ui_MainWindow.h"

class PlaysBehaviorTab : public QWidget {
    Q_OBJECT;

public:
    PlaysBehaviorTab(Processor* processor, QWidget* parent = nullptr);

private Q_SLOTS:
    // Playbook
    void on_loadPlaybook_clicked();
    void on_savePlaybook_clicked();
    void on_clearPlays_clicked();
    void playIndicatorStatus(bool color);

private:
};
