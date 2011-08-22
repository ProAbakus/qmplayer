#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qmplayer.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    void addToTbMediaInfo(QString a_propertie, QString a_value);

private slots:
    void mplayerError(QMPlayer::ErrType, QString);
    void mplayerMediaInfoChanged();
    void mplayerStateChanged(QMPlayer::State a_new, QMPlayer::State a_old);
    void mplayerTick(qreal a_tick);
    void mplayerFinished();

    void on_actionOpen_triggered();

    void on_actionQuit_triggered();

    void on_btnPlay_clicked();

    void on_btnStop_clicked();

    void on_btnBack_clicked();

    void on_btnFoward_clicked();

    void on_btnMute_clicked();

    void on_sldProgress_sliderMoved(int position);

private:
    Ui::MainWindow* m_ui;
    QMPlayer* m_mplayer;
};

#endif // MAINWINDOW_H
