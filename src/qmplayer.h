#ifndef QMPLAYER_H
#define QMPLAYER_H

#include <QObject>
#include <QProcess>
#include <QSize>
#include <QTimer>
#include <QHash>
#include <QPair>

class QMPlayer : public QObject
{
    Q_OBJECT

public:
    enum State {
        stNotStarted = -1,
        stIdle,
        stLoading,
        stBuffering,
        stPlaying,
        stPaused,
        stStopped
    };

    enum Mode {
        mdAuto = -1,
        mdEmbeddedMode = 0,
//        mdPipeMode
    };

    enum Parameter {
        paNone = -1,

        paMediaProgress,

        // Time in ms
        paAudioDelay,
        // Decibels percent 0 to 255
        paAudioVolume,
        paAudioMute,

        // a_perc => -100 <-> 100 default 0
        paVideoBrightness,
        paVideoContrast,
        paVideoGamma,
        paVideoHue,
        paVideoSaturation
    };

    enum ErrType {
        etNoErr,
        etWarning,
        etFatal,
        etUnknown
    };

    struct MediaInfo {
        QString url;

        struct VideoInfo {
            QString codec;
            QString format;
            qint32 bitrate;
            QSize size;
            qreal fps;

            VideoInfo() : format(), bitrate(0), size(), fps(0) {}
        } video;

        struct AudioInfo {
            QString codec;
            QString format;
            qint32 bitrate;
            qint32 sampleRate;
            qint32 numChannels;

            AudioInfo() : format(), bitrate(0), sampleRate(0), numChannels(0) {}
        } audio;

        QHash<QString, QString> tags;

        bool valid;
        double length;
        bool seekable;

        MediaInfo(const QString& a_url = QString()) : url(a_url), video(), audio(), tags(), valid(false), length(0), seekable(false) {}

        bool hasVideo() { return !video.format.isEmpty(); }
        bool hasAudio() { return !audio.format.isEmpty(); }
    };

public:
    explicit QMPlayer(QObject* a_parent = 0);
    virtual ~QMPlayer();

    // process control
    bool startProcess(qint32 a_winId = 0, const QStringList& a_args = QStringList());
    bool stopProcess();

    void writeCommand(QByteArray a_cmd);

    // info
    QProcess::ProcessState processState() const;
    QMPlayer::State state() const;

    // audio
    qreal audioDelay() const;
    qreal audioVolume() const;
    bool audioMute() const;

    // video
    qreal videoBrightness() const;
    qreal videoContrast() const;
    qreal videoGama() const;
    qreal videoHue() const;
    qreal videoSaturation() const;

    // media
    qint64 tell() const;

    const QMPlayer::MediaInfo& mediaInfo() const;

    static void setMPlayerPath(const QString& a_path);
    static QString mPlayerPath();
    static QString mPlayerVersion();

public slots:
    // audio
    void setAudioDelay(qreal a_ms, bool a_absolute = true);
    void setAudioVolume(qreal a_db, bool a_absolute = true);
    void setAudioMute(bool a_mute);

    // video
    void setVideoBrightness(qreal a_perc, bool a_absolute = true);
    void setVideoContrast(qreal a_perc, bool a_absolute = true);
    void setVideoGamma(qreal a_perc, bool a_absolute = true);
    void setVideoHue(qreal a_perc, bool a_absolute = true);
    void setVideoSaturation(qreal a_perc, bool a_absolute = true);

    // media
    void play(const QString& a_url = QString());
    void pause();
    void stop();
    void seek(qreal a_value, bool a_absolute = true);

    QPair<QMPlayer::ErrType, QString> lastError();

private:
    void setError(QMPlayer::ErrType a_type, const QString& a_error);
    void setState(QMPlayer::State a_new);
    void setParameter(Parameter a_param, qreal a_v, bool a_absolute);

private slots:
    void sendPendingParameter();
    void emitErrors();
    void emitFinishPlay();

    void processFinished(int, QProcess::ExitStatus);
    void processReadyReadStandardError();
    void processReadyReadStandardOutput();
    void parseMediaInfo(QString a_line);
    void parsePosition(QString a_line);

signals:
    void tick(qreal a_currentTime);
    void stateChange(QMPlayer::State a_new, QMPlayer::State a_old);
    void mediaInfoChange();
    void error(QMPlayer::ErrType a_type, const QString& a_error);
    void finish();

private:
    QProcess m_process;
    Mode m_mode;

    MediaInfo m_mediaInfo;

    State m_state;
    QTimer m_notifyFinishedPlay;
    QTimer m_notifyErrors;

    QHash<Parameter, qreal> m_parameterValues;
    QTimer m_sendPendingParameter;
    QPair<Parameter, qreal> m_pendingParameter;

    QPair<QMPlayer::ErrType, QString> m_error;

    static QString sm_mplayerPath;
    static QString sm_mplayerVersion;
};

#endif // QMPLAYER_H
