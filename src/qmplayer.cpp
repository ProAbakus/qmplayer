#include "qmplayer.h"
#include <QDebug>
#include <QRegExp>
#include <QCoreApplication>

QString QMPlayer::sm_mplayerPath = "mplayer";
QString QMPlayer::sm_mplayerVersion;

QMPlayer::QMPlayer(QObject *parent) :
    QObject(parent), m_process(), m_mode(mdAuto), m_mediaInfo(), m_state(stNotStarted),
    m_notifyErrors(), m_parameterValues(), m_sendPendingParameter(),
    m_pendingParameter(paNone, 0), m_error(etNoErr, "No Error")
{
    m_sendPendingParameter.setInterval(50);
    m_sendPendingParameter.setSingleShot(true);
    connect(&m_sendPendingParameter, SIGNAL(timeout()), SLOT(sendPendingParameter()));

    m_notifyErrors.setInterval(100);
    m_notifyErrors.setSingleShot(true);
    connect(&m_notifyErrors, SIGNAL(timeout()), SLOT(emitErrors()));

    m_notifyFinishedPlay.setInterval(500);
    m_notifyFinishedPlay.setSingleShot(true);
    connect(&m_notifyFinishedPlay, SIGNAL(timeout()), SLOT(emitFinishPlay()));

    connect(&m_process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(readyReadStandardError()), SLOT(processReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadyReadStandardOutput()));

    m_parameterValues[paMediaProgress] = 0;
    m_parameterValues[paAudioDelay] = 0;
    m_parameterValues[paAudioVolume] = 100;
    m_parameterValues[paAudioMute] = false;
    m_parameterValues[paVideoBrightness] = 0;
    m_parameterValues[paVideoContrast] = 0;
    m_parameterValues[paVideoGamma] = 0;
    m_parameterValues[paVideoHue] = 0;
    m_parameterValues[paVideoSaturation] = 0;
}

QMPlayer::~QMPlayer() {
    stopProcess();
}

bool QMPlayer::startProcess(qint32 a_winId, const QStringList& a_args) {
    QStringList l_args;
    QString l_videoOutput;

    if (!stopProcess()) return false;

#ifdef Q_WS_WIN
    if (m_mode == mdAuto)
        m_mode = QMPlayer::mdEmbeddedMode;
    l_videoOutput = "directx,directx:noaccel";
#elif defined(Q_WS_X11)
    if (m_mode == mdAuto)
        m_mode = QMPlayer::mdEmbeddedMode;
    l_videoOutput = "xv:ck=set,";
#elif defined(Q_WS_MAC)
    if (m_mode == mdAuto)
        m_mode = QMPlayer::mdPipeMode;
    l_videoOutput = "quartz";
#endif

    l_args += "-zoom";
    l_args += "-noautosub";
    l_args += "-slave";
    l_args += "-colorkey";
    l_args += "0x020202";
    l_args += "-ao";
    l_args += "alsa,";
    l_args += "-osdlevel";
    l_args += "0";
    l_args += "-contrast";
    l_args += QString::number(m_parameterValues[paVideoContrast], 'f', 0);
    l_args += "-brightness";
    l_args += QString::number(m_parameterValues[paVideoBrightness], 'f', 0);
    l_args += "-hue";
    l_args += QString::number(m_parameterValues[paVideoHue], 'f', 0);
    l_args += "-saturation";
    l_args += QString::number(m_parameterValues[paVideoSaturation], 'f', 0);
    l_args += "-framedrop";
    l_args += "-fontconfig";
    l_args += "-font";
    l_args += "Sans";
    l_args += "-subfont-autoscale";
    l_args += "3";
    l_args += "-subfont-text-scale";
    l_args += "3";
    l_args += "-double";
    l_args += "-noquiet";
    l_args += "-msglevel";
    l_args += "identify=4";
    l_args += "-idle";
    l_args += "-af";
    l_args += "volnorm";
    l_args += "-input";
    l_args += "nodefault-bindings";
    l_args += "-noconfig";
    l_args += "all";

    if (m_mode == QMPlayer::mdEmbeddedMode) {
        l_args += "-wid";
        l_args += QString::number(a_winId);
        l_args += "-vo";
        l_args += l_videoOutput;
    }
    l_args += a_args;

    m_process.start(sm_mplayerPath, l_args);
    if (!m_process.waitForStarted()) {
        setError(etFatal, "Process not started: " + m_process.errorString());
        return false;
    }

    setState(stIdle);
    setAudioVolume(100);

    return true;
}

bool QMPlayer::stopProcess() {
    if (m_state != stNotStarted) {
        setState(stStopped);
        writeCommand("quit\n");
        return m_process.waitForFinished();
    }

    return true;
}

void QMPlayer::writeCommand(QByteArray a_cmd) {
    if (!a_cmd.endsWith("\n")) a_cmd += "\n";
    m_process.write(a_cmd);

    a_cmd.chop(1);
    qDebug() << "Mplayer stdin: " << QString::fromUtf8(a_cmd);
}

QProcess::ProcessState QMPlayer::processState() const {
    return m_process.state();
}

QMPlayer::State QMPlayer::state() const {
    return m_state;
}

qreal QMPlayer::audioDelay() const {
    return m_parameterValues.value(paAudioDelay, 0);
}

qreal QMPlayer::audioVolume() const {
    return m_parameterValues.value(paAudioVolume, 0);
}

bool QMPlayer::audioMute() const {
    return !qFuzzyIsNull(m_parameterValues.value(paAudioMute, 0));
}

qreal QMPlayer::videoBrightness() const {
    return m_parameterValues.value(paVideoBrightness, 0);
}

qreal QMPlayer::videoContrast() const {
    return m_parameterValues.value(paVideoContrast, 0);
}

qreal QMPlayer::videoGama() const {
    return m_parameterValues.value(paVideoGamma, 0);
}

qreal QMPlayer::videoHue() const {
    return m_parameterValues.value(paVideoHue, 0);
}

qreal QMPlayer::videoSaturation() const {
    return m_parameterValues.value(paVideoSaturation, 0);
}

qint64 QMPlayer::tell() const {
    return m_parameterValues.value(paMediaProgress, -1);
}

const QMPlayer::MediaInfo& QMPlayer::mediaInfo() const {
    return m_mediaInfo;
}


void QMPlayer::setMPlayerPath(const QString& a_path) {
    QMPlayer::sm_mplayerPath = a_path;
    QMPlayer::sm_mplayerVersion = QString();
}

QString QMPlayer::mPlayerPath() {
    return QMPlayer::sm_mplayerPath;
}

QString QMPlayer::mPlayerVersion() {
    if (QMPlayer::sm_mplayerVersion.isEmpty()) {
        QProcess p;

        p.start(QMPlayer::sm_mplayerPath, QStringList("-version"));
        if (p.waitForStarted() && p.waitForFinished()) {
            QMPlayer::sm_mplayerVersion = QString(p.readLine());

            QRegExp re("MPlayer ([^ ]*)");
            if (re.indexIn(QMPlayer::sm_mplayerVersion) > -1) {
                QMPlayer::sm_mplayerVersion = re.cap(1);
            }
        } else {
            return "Not Avaible";
        }
    }
    return QMPlayer::sm_mplayerVersion;
}

void QMPlayer::setAudioDelay(qreal a_ms, bool a_absolute) {
    setParameter(paAudioDelay, a_ms, a_absolute);
}

void QMPlayer::setAudioVolume(qreal a_db, bool a_absolute) {
    setParameter(paAudioVolume, a_db, a_absolute);
}

void QMPlayer::setAudioMute(bool a_mute) {
    setParameter(paAudioMute, a_mute, true);
}

void QMPlayer::setVideoBrightness(qreal a_perc, bool a_absolute) {
    setParameter(paVideoBrightness, a_perc, a_absolute);
}

void QMPlayer::setVideoContrast(qreal a_perc, bool a_absolute) {
    setParameter(paVideoContrast, a_perc, a_absolute);
}

void QMPlayer::setVideoGamma(qreal a_perc, bool a_absolute) {
    setParameter(paVideoGamma, a_perc, a_absolute);
}

void QMPlayer::setVideoHue(qreal a_perc, bool a_absolute) {
    setParameter(paVideoHue, a_perc, a_absolute);
}

void QMPlayer::setVideoSaturation(qreal a_perc, bool a_absolute) {
    setParameter(paVideoSaturation, a_perc, a_absolute);
}

void QMPlayer::play(const QString& a_url) {
    if (m_state == stNotStarted) {
        setError(etFatal, "Call startProcess(...) first");
        return;
    }

    if ((!a_url.isEmpty())
    &&  (a_url != m_mediaInfo.url)) {
        writeCommand("stop\n");
        setState(stIdle);

        m_mediaInfo.url = a_url;
    } else {
        if ((m_state == stPaused)
        ||  (m_state == stStopped)) {
            pause();
            return;
        }
        if (m_state == stPlaying) {
            return;
        }
    }

    m_mediaInfo = MediaInfo(m_mediaInfo.url);
    emit mediaInfoChange();
    m_parameterValues[paMediaProgress] = 0;
    emit tick(m_parameterValues[paMediaProgress]);

    setState(stLoading);
    writeCommand(QByteArray("loadfile '[FILE_NAME]'\n").replace("[FILE_NAME]", m_mediaInfo.url.toUtf8()));
}

void QMPlayer::pause() {
    if (m_state == stNotStarted) {
        setError(etFatal, "Call startProcess(...) first");
        return;
    }

    if (m_state == stIdle) {
        play();
        return;
    }

    if (m_notifyFinishedPlay.timerId() != -1) {
        setError(etWarning, "Near the end, pause not permited now");
        return;
    }

    if ((m_state == stPlaying)
    ||  (m_state == stPaused)
    ||  (m_state == stStopped)) {
        writeCommand("pause\n");
        setState((m_state == QMPlayer::stPlaying) ? QMPlayer::stPaused : QMPlayer::stPlaying);
    }
}

void QMPlayer::stop() {
    if (m_state == stNotStarted) {
        setError(etFatal, "Call startProcess(...) first");
        return;
    }

    if (m_notifyFinishedPlay.timerId() != -1) {
        setError(etWarning, "Near the end, stop not permited now");
        return;
    }

    if ((m_state == stPlaying)
    ||  (m_state == stPaused)) {
        seek(0);
        sendPendingParameter();
        writeCommand("pause\n");
        setState(stStopped);
    }
}

void QMPlayer::seek(qreal a_value, bool a_absolute) {
    if (m_notifyFinishedPlay.timerId() != -1) {
        setError(etWarning, "Near the end, pause not permited now");
        return;
    }

    setParameter(paMediaProgress, a_value, a_absolute);
}

QPair<QMPlayer::ErrType, QString> QMPlayer::lastError() {
    return m_error;
}


void QMPlayer::setError(QMPlayer::ErrType a_type, const QString& a_error) {
    if ((m_error.first != a_type)
    &&  (m_notifyErrors.timerId() != -1)) {
        emitErrors();
    }

    if (m_notifyErrors.timerId() == -1) {
        m_error = QPair<QMPlayer::ErrType, QString>(a_type, "");
    }
    if (!m_error.second.isEmpty())
        m_error.second += "\n";

    m_error.second += a_error;
    m_notifyErrors.start();
}

void QMPlayer::setState(QMPlayer::State a_new) {
    if (a_new != m_state) {
        QMPlayer::State l_old = m_state;
        m_state = a_new;

        emit stateChange(a_new, l_old);
    }
}

void QMPlayer::setParameter(Parameter a_param, qreal a_v, bool a_absolute) {
    if (m_state == stNotStarted) {
        setError(etFatal, "Call startProcess(...) first");
        return;
    }

    if (m_pendingParameter.first != a_param) {
        sendPendingParameter();
        if (!a_absolute) a_v += m_parameterValues[a_param];
    } else {
        if (!a_absolute) a_v += m_pendingParameter.second;
    }

    m_pendingParameter = QPair<Parameter, qreal>(a_param, a_v);
    m_sendPendingParameter.start();
}

void QMPlayer::sendPendingParameter() {
    QString l_toSend;
    bool l_setOrig = true;

    switch (m_pendingParameter.first) {
        case paNone: return;

        case paMediaProgress: {
            if (m_state == stIdle) {
                play();

                while (m_state == stLoading) {
                    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
                }

                if (m_state == stPlaying) pause();
            }

            if ((m_state != stPlaying)
            &&  (m_state != stPaused)
            &&  (m_state != stStopped)) {
                setError(etWarning, "Invalid state for seek");
                return;
            }

            if (!m_mediaInfo.seekable) {
                setError(etWarning, "That media file is not seekable");
                return;
            }

            m_pendingParameter.second = qBound(0.0, m_pendingParameter.second, m_mediaInfo.length);
            l_toSend = QString("seek %1 2\n").arg(m_pendingParameter.second, 0, 'f', 1);
            l_setOrig = false;
        } break;
        case paAudioDelay: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("audio_delay %1 2\n").arg(qRound(m_pendingParameter.second));
        } break;
        case paAudioVolume: {
            m_pendingParameter.second = qBound(0.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("volume %1 1\n").arg(m_pendingParameter.second);
        } break;
        case paAudioMute: {
            l_toSend = QString("mute %1\n").arg(!qFuzzyIsNull(m_pendingParameter.second));
        } break;
        case paVideoBrightness: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("brightness %1 1\n").arg(qRound(m_pendingParameter.second));
        } break;
        case paVideoContrast: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("contrast %1 1\n").arg(qRound(m_pendingParameter.second));
        } break;
        case paVideoGamma: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("gamma %1 1\n").arg(qRound(m_pendingParameter.second));
        } break;
        case paVideoHue: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("hue %1 1\n").arg(qRound(m_pendingParameter.second));
        } break;
        case paVideoSaturation: {
            m_pendingParameter.second = qBound(-100.0, m_pendingParameter.second, 100.0);
            l_toSend = QString("saturation %1 1\n").arg(qRound(m_pendingParameter.second));
        } break;
    }
    m_pendingParameter.first = paNone;

    if (l_toSend.isEmpty()) {
        setError(etWarning, "Not implemented");
        return;
    }

    if (!qFuzzyCompare(m_parameterValues[m_pendingParameter.first], m_pendingParameter.second)) {
        if (l_setOrig)
            m_parameterValues[m_pendingParameter.first] = m_pendingParameter.second;

        writeCommand(l_toSend.toUtf8());

        if ((m_pendingParameter.first == paMediaProgress)
        ||  (m_state == stPaused)
        ||  (m_state == stStopped)) {
            // send the seek make the media go to play state
            setState(stPlaying);
            pause();
        }
    }
}

void QMPlayer::emitErrors() {
    if (m_error.first != etNoErr) {
        emit error(m_error.first, m_error.second);
    }
}

void QMPlayer::emitFinishPlay() {
    setState(stIdle);
    emit finish();
}

void QMPlayer::processFinished(int a_code, QProcess::ExitStatus a_status) {
    Q_UNUSED(a_code);

    if (a_status == QProcess::CrashExit) {
        setError(etFatal, "Process mplayer crashed");
    }

    if ((m_state == stPlaying)
    ||  (m_state == stPaused)) {
        setError(etWarning, "Playback interrupted");
        setState(stStopped);
    }
    setState(stNotStarted);
}

void QMPlayer::processReadyReadStandardError() {
    QStringList l_lines = QString::fromUtf8(m_process.readAllStandardError()).split("\n");
    foreach (QString l_line, l_lines) {
        qDebug() << "MPlayer stderr: " << l_line;

        if (l_line.contains("Seek failed")) {
            setError(etFatal, "Seek failed");
            setState(stIdle);
            continue;
        }

        setError(etUnknown, l_line);
    }
}

void QMPlayer::processReadyReadStandardOutput() {
    QStringList l_lines = QString::fromUtf8(m_process.readAllStandardOutput()).split("\n", QString::SkipEmptyParts);
    foreach (QString l_line, l_lines) {
        qDebug() << "MPlayer stdout: " << l_line;

        if (l_line.startsWith("Playing ")) continue;
        if (l_line.startsWith("Cache fill:")) {
            setState(QMPlayer::stBuffering);
            continue;
        }
        if (l_line.startsWith("Starting playback...")) {
            m_mediaInfo.valid = true; // No more info here
            emit mediaInfoChange();
            m_parameterValues[paMediaProgress] = 0;
            emit tick(m_parameterValues[paMediaProgress]);
            setState(QMPlayer::stPlaying);
            continue;
        }
        if (l_line.startsWith("File not found: ")) {
            setError(etFatal, l_line);
            setState(QMPlayer::stStopped);
            continue;
        }
        if (l_line.contains("ID_PAUSED")) continue;
        if (l_line.startsWith("ID_SIGNAL")) continue;
        if (l_line.startsWith("ID_EXIT")) continue;

        if (l_line.startsWith("ID_")) {
            parseMediaInfo(l_line);
            continue;
        }
        if (l_line.startsWith("No stream found")) {
            setError(etFatal, l_line);
            setState(QMPlayer::stStopped);
            continue;
        }
        if (l_line.startsWith("A:") || l_line.startsWith("V:")) {
            parsePosition(l_line);
            continue;
        }
        if (l_line.startsWith("Exiting...")) {
            setState(QMPlayer::stNotStarted);
            continue;
        }
    }
}

void QMPlayer::parseMediaInfo(QString a_line) {
    static QString sl_currentTag;
    QStringList l_info = a_line.split("=");
    if (l_info.count() < 2) {
        return;
    }

    if (l_info[0].startsWith("ID_VIDEO")) {
        if (l_info[0] == "ID_VIDEO_CODEC") {
            m_mediaInfo.video.codec = l_info[1];
        } else
        if (l_info[0] == "ID_VIDEO_FORMAT") {
            m_mediaInfo.video.format = l_info[1];
        } else
        if (l_info[0] == "ID_VIDEO_BITRATE") {
            m_mediaInfo.video.bitrate = l_info[1].toInt();
        } else
        if (l_info[0] == "ID_VIDEO_WIDTH") {
            m_mediaInfo.video.size.setWidth(l_info[1].toInt());
        } else
        if (l_info[0] == "ID_VIDEO_HEIGHT") {
            m_mediaInfo.video.size.setHeight(l_info[1].toInt());
        } else
        if (l_info[0] == "ID_VIDEO_FPS") {
            m_mediaInfo.video.fps = l_info[1].toDouble();
        }
    } else
    if (l_info[0].startsWith("ID_AUDIO")) {
        if (l_info[0] == "ID_AUDIO_CODEC") {
            m_mediaInfo.audio.codec = l_info[1];
        } else
        if (l_info[0] == "ID_AUDIO_FORMAT") {
            m_mediaInfo.audio.format = l_info[1];
        } else
        if (l_info[0] == "ID_AUDIO_BITRATE") {
            m_mediaInfo.audio.bitrate = l_info[1].toInt();
        } else
        if (l_info[0] == "ID_AUDIO_RATE") {
            m_mediaInfo.audio.sampleRate = l_info[1].toInt();
        } else
        if (l_info[0] == "ID_AUDIO_NCH") {
            m_mediaInfo.audio.numChannels = l_info[1].toInt();
        }
    } else
    if (l_info[0].startsWith("ID_CLIP")) {
        if (l_info[0].startsWith("ID_CLIP_INFO_NAME")) {
            sl_currentTag = l_info[1];
        } else
        if (l_info[0].startsWith("ID_CLIP_INFO_VALUE") && !sl_currentTag.isEmpty()) {
            m_mediaInfo.tags.insert(sl_currentTag, l_info[1]);
        }
    } else
    if (l_info[0] == "ID_LENGTH") {
        m_mediaInfo.length = l_info[1].toDouble();
    } else
    if (l_info[0] == "ID_SEEKABLE") {
        m_mediaInfo.seekable = l_info[1].toInt() != 0;
    }
}

void QMPlayer::parsePosition(QString a_line) {
    static qint32 sl_eqTimes = 0;
    static QRegExp l_rg("(A|V):[ ]*([0-9]+[.]{0,1}[0-9]*)");

    if (m_state < stPlaying) return;

    if (l_rg.indexIn(a_line) >= 0) {
        qreal l_curSeek = l_rg.cap(2).toDouble();

        if ((m_mediaInfo.length - m_parameterValues[paMediaProgress]) <= 0.5)
            m_notifyFinishedPlay.start();
        if (m_state == stPaused) sl_eqTimes = 0;
        if (qFuzzyCompare(l_curSeek, m_parameterValues[paMediaProgress])) {
            ++sl_eqTimes;
            if (sl_eqTimes > 4) {
                sl_eqTimes = 0;
                // to recory from possible mplayer bug
                writeCommand(QString("seek -1 0\n").toUtf8());
            }
            return;
        }

        sl_eqTimes = 0;
        m_parameterValues[paMediaProgress] = l_curSeek;
        emit tick(m_parameterValues[paMediaProgress]);
    }
}

