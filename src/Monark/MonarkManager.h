#pragma once

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include <QThread>
#include <QWaitCondition>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <mutex>
#include <set>
#include <future>
#include <QGCSerialPortInfo.h>
#include <unordered_map>
#include <functional>
Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)

class QSerialPort;


class MonarkManagerWorkerWorker : public QThread
{
    Q_OBJECT
public:
    MonarkManagerWorkerWorker();
    bool needDispatch();
    void dispatch(std::function<void()> t);
    void shutdown();
protected:
    void run() override;
private:
    QWaitCondition m_taskQueueCondition;
    QMutex m_taskQueueMut;
    QQueue<std::function<void()>> m_taskQueue;
    bool m_shutdown;
};

class MonarkSettings;
class MonarkQRCodeProvider;

class MonarkManager : public QGCTool
{
    Q_OBJECT
public:

    enum class MonarkState : int
    {
        BeforeScan=0,
        ScanInProgress=1,
        ScanSuccessPairingRequired=2,
        ScanSuccessBadCredentials=3, //deprecated
        ScanSuccessAndPaired=4,
        ScanFailedNotDetected=5,
        SaveSettingsInProgress=6,
        SaveSettingsFailed=7,
        BeforePairNewDrone=8,
        ShowQRCode=9,
        ResetUnpairMonark=10,
        DetectionFailed=11,
        ChangeTxPower=12,
        ChangeFrequencies=13,
        ChangeEncryptionKey=14,
    };

    enum class UpdateState : int{
        BeforeUpdate=0,
        UpdateInProgress=1,
        UpdateSuccessful=2,
        UpdateFailed=3,
    };

    MonarkManager(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkManager();
    virtual void setToolbox(QGCToolbox* p_toolbox) override;

    Q_PROPERTY(int         monarkState             READ monarkState             NOTIFY monarkStateChanged)
    Q_PROPERTY(int         groundRadioUpdateState  READ groundRadioUpdateState  NOTIFY groundRadioUpdateStateChanged)
    Q_PROPERTY(QString     allDrones               READ allDrones               NOTIFY allDronesChanged);
    Q_PROPERTY(QString     beforeUpdateDrones      READ beforeUpdateDrones      NOTIFY beforeUpdateDronesChanged);
    Q_PROPERTY(QString     updateInProgressDrones  READ updateInProgressDrones  NOTIFY updateInProgressDronesChanged);
    Q_PROPERTY(QString     updateSuccessfulDrones  READ updateSuccessfulDrones  NOTIFY updateSuccessfulDronesChanged);
    Q_PROPERTY(QString     updateFailedDrones      READ updateFailedDrones      NOTIFY updateFailedDronesChanged);
    Q_PROPERTY(QStringList validFrequencies        READ    validFrequencies     NOTIFY validFrequenciesChanged);
    Q_PROPERTY(int         minimumPower            READ minimumPower            NOTIFY minMaxPowersChanged);
    Q_PROPERTY(int         maximumPower            READ maximumPower            NOTIFY minMaxPowersChanged);
    Q_PROPERTY(QString     newGcsVersion           READ newGcsVersion           NOTIFY newGcsVersionChanged);
    Q_PROPERTY(QString     newGcsDescription       READ newGcsDescription       NOTIFY newGcsDescriptionChanged);
    Q_PROPERTY(QString     newGcsURL               READ newGcsURL               NOTIFY newGcsURLChanged);
    Q_PROPERTY(QString     newGcsReleaseDate       READ newGcsReleaseDate       NOTIFY newGcsReleaseDateChanged);
    Q_PROPERTY(QString     newDroneVersion         READ newDroneVersion         NOTIFY newDroneVersionChanged);
    Q_PROPERTY(QString     newDroneDescription     READ newDroneDescription     NOTIFY newDroneDescriptionChanged);
    Q_PROPERTY(QString     newDroneURL             READ newDroneURL             NOTIFY newDroneURLChanged);
    Q_PROPERTY(QString     newDroneReleaseDate     READ newDroneReleaseDate     NOTIFY newDroneReleaseDateChanged);
    Q_PROPERTY(int         monarkUpdatePushPercent READ monarkUpdatePushPercent NOTIFY monarkUpdatePushPercentChanged);
    Q_PROPERTY(QString     monarkUpdatePushError   READ monarkUpdatePushError   NOTIFY monarkUpdatePushErrorChanged);

    int         monarkState            () const { return m_monarkState; }
    int         groundRadioUpdateState () const { return m_groundRadioUpdateState; }
    QString     newGcsVersion          () const { return m_newGcsVersion; }
    QString     newGcsDescription      () const { return m_newGcsDescription; }
    QString     newGcsURL              () const { return m_newGcsURL; }
    QString     newGcsReleaseDate      () const { return m_newGcsReleaseDate; }
    QString     newDroneVersion        () const { return m_newDroneVersion; }
    QString     newDroneDescription    () const { return m_newDroneDescription; }
    QString     newDroneURL            () const { return m_newDroneURL; }
    QString     newDroneReleaseDate    () const { return m_newDroneReleaseDate; }
    int         monarkUpdatePushPercent() const { return m_monarkUpdatePushPercent; }
    QString     monarkUpdatePushError  () const { return m_monarkUpdatePushError; }
    QString     allDrones              () const;
    QString     beforeUpdateDrones     () const;
    QString     updateInProgressDrones () const;
    QString     updateSuccessfulDrones () const;
    QString     updateFailedDrones     () const;
    QStringList validFrequencies       () const;
    int         minimumPower           () const;
    int         maximumPower           () const;

    Q_INVOKABLE void startScanning                ();
    Q_INVOKABLE void refreshDroneList             ();
    Q_INVOKABLE void removeDrone                  (int monarkID);
    Q_INVOKABLE void saveFlutterManagementSettings(QString const& frequency);
    Q_INVOKABLE void resetActiveVehicle           ();
    Q_INVOKABLE void rebootActiveVehicle          ();
    Q_INVOKABLE void detect                       ();
    Q_INVOKABLE void openGcsDownload              ();
    Q_INVOKABLE void pushMonarkDownload           (int monarkID);
    Q_INVOKABLE void gotoBeforePairNewDrone       ();
    Q_INVOKABLE void gotoChangeEncryptionKey      ();
    Q_INVOKABLE void gotoChangeFrequencies        ();
    Q_INVOKABLE void gotoChangeTxPower            ();
    Q_INVOKABLE void gotoResetUnpairMonark        ();
    Q_INVOKABLE void gotoScanSuccessAndPaired     ();
    Q_INVOKABLE void gotoDetectionFailed          ();
    Q_INVOKABLE void changeTxPower                (QString const& desiredTxPower);
    Q_INVOKABLE void changeFrequencies            (QString const& desiredFrequency);
    Q_INVOKABLE void changeEncryptionKey          ();
    Q_INVOKABLE void tryDroneUpdate               (int monarkId);

signals:

    void monarkStateChanged            (int monarkState);
    void groundRadioUpdateStateChanged (int updateState);
    void allDronesChanged              ();
    void beforeUpdateDronesChanged     ();
    void updateInProgressDronesChanged ();
    void updateSuccessfulDronesChanged ();
    void updateFailedDronesChanged     ();
    void displayMonarkUpdateMessage    (int monarkId);
    void displayRestartMessage         ();
    void displayGcsUpdateMessage       ();
    void validFrequenciesChanged       ();
    void newGcsVersionChanged          ();
    void newGcsDescriptionChanged      ();
    void newGcsURLChanged              ();
    void newGcsReleaseDateChanged      ();
    void newDroneVersionChanged        ();
    void newDroneDescriptionChanged    ();
    void newDroneURLChanged            ();
    void newDroneReleaseDateChanged    ();
    void minMaxPowersChanged           ();
    void monarkUpdatePushPercentChanged();
    void monarkUpdatePushErrorChanged  ();

private:

    void                    _checkForUpdates                   ();
    void                    _gcsVersionCheck                   (QString /*remoteFile*/, QString localFile, QString errorMsg);
    void                    _renameFirmwareFile                (QString /*remoteFile*/, QString localFile, QString errorMsg);
    bool                    _checkIfDroneNeedsUpdate           (int monarkId);
    void                    _initializeNetworkId               ();
    void                    _initializeFrequency               ();
    void                    _initializeTxPower                 ();
    void                    _pingAllDrones                     ();
    void                    _setMonarkState                    (MonarkState monarkState);
    bool                    _changeGroundRadioFrequency        (QString const& desiredFrequency, bool reversion);
    bool                    _changeGroundRadioTxPower          (QString const& desiredPower);
    void                    _resetToBeforeUpdate               ();
    void                    _waitForPingResponses              (std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>>& pingDroneResponses,  std::function<void(int)> const& responseGoodFunc, std::function<void(int)> const& responseBadFunc);
    bool                    _parseInfoJsonResponse             (QString json);
    void                    _findEchoLinkDevice                ();
    void                    _setEchoLinkRadioModel             ();
    void                    _echoLinkBatteryVoltageTimerHandler();
    void                    _sendEncryptionKeyToGcsRadio       (QString const& password);
    bool                    _changeGroundRadioEncryptionKey    (QString const& currentEncryptionKey, QString const& desiredKey, bool reversion);
    QString                 _getEncryptionKeyFromGcsRadio      ();
    std::pair<bool,QString> _runEchoLinkSerialCommand          (QString const& command, std::function<std::pair<bool,QString>(QString const&)>&& func, int numSeconds=5);
    std::pair<bool,QString> _runEchoLinkSSHCommand             (QString const& command, bool configured, std::function<std::pair<bool,QString>(QString const&)>&& func, int numSeconds=5);

private:
    std::unique_ptr<MonarkManagerWorkerWorker> mp_slotHandler;
    MonarkSettings*                            mp_monarkSettings;
    MonarkQRCodeProvider*                      mp_monarkQRCodeProvider;
    std::set<int>                              m_allDrones;
    std::set<int>                              m_beforeUpdateDrones;
    std::set<int>                              m_updateInProgressDrones;
    std::set<int>                              m_updateSuccessfulDrones;
    std::set<int>                              m_updateFailedDrones;
    QAtomicInteger<int>                        m_groundRadioUpdateState;
    QAtomicInteger<int>                        m_monarkState;
    std::mutex                                 m_monarkStateMut;
    std::condition_variable                    m_monarkStateCondition;
    QString                                    m_newGcsVersion;
    QString                                    m_newGcsDescription;
    QString                                    m_newGcsURL;
    QString                                    m_newGcsReleaseDate;
    QString                                    m_newDroneVersion;
    QString                                    m_newDroneDescription;
    QString                                    m_newDroneURL;
    QString                                    m_newDroneReleaseDate;
    float                                      m_echoLinkBatteryVoltage;
    QTimer                                     m_echoLinkBatteryVoltageTimer;
    bool                                       m_paired;
    std::unordered_map<int,QString>            m_droneRadioModels;
    QString                                    m_groundRadioModel;
    int                                        m_monarkUpdatePushPercent;
    QString                                    m_monarkUpdatePushError;
    std::recursive_mutex                                 m_serialMut;
    QSerialPort*                               mp_serialPort;
};
