#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_mplayer = new QMPlayer(this);
    m_mplayer->startProcess((qint32)m_ui->frmVideo->winId());
//    m_mplayerWdg->setSeekSlider(m_ui->sldProgress);
//    m_mplayerWdg->setVolumeSlider(m_ui->sldVolume);
//    m_ui->frmVideo->layout()->addWidget(m_mplayerWdg);

    connect(m_mplayer, SIGNAL(error(QMPlayer::ErrType,QString)), SLOT(mplayerError(QMPlayer::ErrType,QString)));
    connect(m_mplayer, SIGNAL(mediaInfoChange()), SLOT(mplayerMediaInfoChanged()));
    connect(m_mplayer, SIGNAL(stateChange(QMPlayer::State,QMPlayer::State)), SLOT(mplayerStateChanged(QMPlayer::State,QMPlayer::State)));
    connect(m_mplayer, SIGNAL(finish()), SLOT(mplayerFinished()));

    connect(m_mplayer, SIGNAL(tick(qreal)), SLOT(mplayerTick(qreal)));

    connect(m_ui->btnMute, SIGNAL(toggled(bool)), m_mplayer, SLOT(setAudioMute(bool)));
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::mplayerError(QMPlayer::ErrType a_type, QString a_comment) {
    Q_UNUSED(a_type);
    m_ui->edtLog->appendHtml("<br><b>QMPwidget::error<b><br>");
    m_ui->edtLog->appendPlainText(a_comment);
}

void MainWindow::mplayerMediaInfoChanged() {
    QMPlayer::MediaInfo l_mi = m_mplayer->mediaInfo();

    m_ui->tabMediaInfo->setRowCount(0);
    m_ui->sldProgress->setEnabled(l_mi.seekable);
    m_ui->sldProgress->setRange(0, qRound(l_mi.length * 1000.0));
    if (l_mi.valid) {
        addToTbMediaInfo("url", l_mi.url);
        if (l_mi.hasVideo()) {
            addToTbMediaInfo("video", "yes");
            addToTbMediaInfo("video.codec", l_mi.video.codec);
            addToTbMediaInfo("video.format", l_mi.video.format);
            addToTbMediaInfo("video.bitrate", QString::number(l_mi.video.bitrate));
            addToTbMediaInfo("video.size", QString("%1x%2").arg(l_mi.video.size.width()).arg(l_mi.video.size.height()));
            addToTbMediaInfo("video.fps", QString::number(l_mi.video.fps, 'f'));
        } else {
            addToTbMediaInfo("video", "no");
        }
        if (l_mi.hasAudio()) {
            addToTbMediaInfo("audio", "yes");
            addToTbMediaInfo("audio.codec", l_mi.audio.codec);
            addToTbMediaInfo("audio.format", l_mi.audio.format);
            addToTbMediaInfo("audio.bitrate", QString::number(l_mi.audio.bitrate));
            addToTbMediaInfo("audio.sampleRate", QString::number(l_mi.audio.sampleRate));
            addToTbMediaInfo("audio.numChannels", QString::number(l_mi.audio.numChannels));
        } else {
            addToTbMediaInfo("audio", "no");
        }
        addToTbMediaInfo("length", QString::number(l_mi.length, 'f'));
        addToTbMediaInfo("seekable", l_mi.seekable ? "yes" : "no");

        m_ui->tabMediaInfo->resizeColumnsToContents();
        m_ui->tabMediaInfo->resizeRowsToContents();
    }
}

void MainWindow::addToTbMediaInfo(QString a_propertie, QString a_value) {
    qint32 a_row = m_ui->tabMediaInfo->rowCount();
    m_ui->tabMediaInfo->insertRow(a_row);
    m_ui->tabMediaInfo->setItem(a_row, 0, new QTableWidgetItem(a_propertie));
    m_ui->tabMediaInfo->setItem(a_row, 1, new QTableWidgetItem(a_value));
}

void MainWindow::mplayerStateChanged(QMPlayer::State a_new, QMPlayer::State a_old) {
    Q_UNUSED(a_old)
    switch (a_new) {
        case QMPlayer::stNotStarted:
            qDebug() << "QMPlayer::stNotStarted";
            break;
        case QMPlayer::stIdle:
            qDebug() << "QMPlayer::stIdle";
            break;
        case QMPlayer::stLoading:
            qDebug() << "QMPlayer::stLoading";
            break;
        case QMPlayer::stBuffering:
            qDebug() << "QMPlayer::stBuffering";
            break;
        case QMPlayer::stPlaying:
            qDebug() << "QMPlayer::stPlaying";
            break;
        case QMPlayer::stPaused:
            qDebug() << "QMPlayer::stPaused";
            break;
        case QMPlayer::stStopped:
            qDebug() << "QMPlayer::stStopped";
            break;
    }
}

void MainWindow::mplayerTick(qreal a_tick) {
    if (!m_ui->sldProgress->isSliderDown())
        m_ui->sldProgress->setValue(qRound(a_tick * 1000));
}

void MainWindow::mplayerFinished() {
    m_ui->edtLog->appendHtml("<br><b>QMPwidget::sucess<b><br>");
    m_ui->edtLog->appendPlainText("Current playback was finished");
}

void MainWindow::on_actionOpen_triggered()
{
    QString url = QFileDialog::getOpenFileName(this, tr("Play media file..."));
    if (!url.isEmpty()) {
        m_mplayer->play(url);
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_btnPlay_clicked()
{
    m_mplayer->pause();
}

void MainWindow::on_btnStop_clicked()
{
    m_mplayer->stop();
}

void MainWindow::on_btnBack_clicked()
{
    m_mplayer->seek(m_mplayer->tell() - 10);
}

void MainWindow::on_btnFoward_clicked()
{
    m_mplayer->seek(m_mplayer->tell() + 10);
}

void MainWindow::on_btnMute_clicked()
{
//    m_mplayerWdg->toogleMute();
}

void MainWindow::on_sldProgress_sliderMoved(int position)
{
    m_mplayer->seek(qreal(position) / 1000);
}
