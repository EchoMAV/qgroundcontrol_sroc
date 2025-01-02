#pragma once

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
#include <mutex>
#include "MonarkDrone.h"
Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)


class MonarkDrone;

Q_DECLARE_METATYPE(QList<MonarkDrone>)

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
        ScanSuccessBadCredentials=3,
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

    MonarkManager(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkManager();

    Q_PROPERTY(int monarkState READ monarkState NOTIFY monarkStateChanged)
    Q_PROPERTY(int groundRadioUpdateState READ groundRadioUpdateState NOTIFY groundRadioUpdateStateChanged)
    Q_PROPERTY(QVariant connectedDroneList READ connectedDroneList NOTIFY connectedDroneListChanged)


    int monarkState() const { return m_monarkState;}
    int groundRadioUpdateState() const { return m_groundRadioUpdateState;}


    QVariant connectedDroneList() const { return QVariant::fromValue(m_connectedDroneList);}

    virtual void setToolbox(QGCToolbox* p_toolbox) override;

    Q_INVOKABLE void startScanning();

    Q_INVOKABLE void saveFlutterManagementSettings();

#if 0
    Q_INVOKABLE void saveEncryptionKey();
#endif


    Q_INVOKABLE void detect();

    Q_INVOKABLE void gotoBeforePairNewDrone();
    Q_INVOKABLE void gotoChangeEncryptionKey();
    Q_INVOKABLE void gotoChangeFrequencies();
    Q_INVOKABLE void gotoChangeTxPower();
    Q_INVOKABLE void gotoResetUnpairMonark();
    Q_INVOKABLE void gotoScanSuccessAndPaired();
    Q_INVOKABLE void gotoDetectionFailed();

    Q_INVOKABLE void changeTxPower(QString const& desiredTxPower);
    Q_INVOKABLE void changeFrequencies(QString const& desiredFrequency);
    Q_INVOKABLE void changeEncryptionKey(QString const& currentEncryptionKey, QString const& desiredEncryptionKey);


    static std::string sendCommands(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands, bool toDrone);


signals:


    void monarkStateChanged(int monarkState);
    void groundRadioUpdateStateChanged(int updateState);
    void connectedDroneListChanged();
    void _pingDroneSuccess(int monarkId);

private slots:
    void _onPingDroneSuccess(int monarkId);

private:
    void _initializeNetworkId(bool paired);

    void _pingAllDrones();

    void _setMonarkState(MonarkState monarkState);

    void _sendCommandsToRadioAndDrones(std::string const& currentEncryptionKey, std::vector<std::string> const& groundRadioCommands, std::vector<std::string> const& droneCommands);

protected:
    std::unique_ptr<MonarkManagerWorkerWorker> mp_slotHandler;
    MonarkSettings*          mp_monarkSettings;
    MonarkQRCodeProvider*    mp_monarkQRCodeProvider;
    QList<MonarkDrone> m_connectedDroneList;
    QAtomicInteger<int> m_groundRadioUpdateState;
private:
    QAtomicInteger<int> m_monarkState;
    std::mutex m_monarkStateMut;
    std::condition_variable m_monarkStateCondition;
    //bool m_paired;

};
