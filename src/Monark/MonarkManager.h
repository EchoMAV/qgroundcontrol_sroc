#pragma once

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
#include <mutex>
#include <set>
#include <future>
Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)




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

    enum class UpdateState : int{
        BeforeUpdate=0,
        UpdateInProgress=1,
        UpdateSuccessful=2,
        UpdateFailed=3,
    };

    MonarkManager(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkManager();

    Q_PROPERTY(int monarkState READ monarkState NOTIFY monarkStateChanged)
    Q_PROPERTY(int groundRadioUpdateState READ groundRadioUpdateState NOTIFY groundRadioUpdateStateChanged)
    Q_PROPERTY(QString allDrones READ allDrones NOTIFY allDronesChanged);
    Q_PROPERTY(QString beforeUpdateDrones READ beforeUpdateDrones NOTIFY beforeUpdateDronesChanged);
    Q_PROPERTY(QString updateInProgressDrones READ updateInProgressDrones NOTIFY updateInProgressDronesChanged);
    Q_PROPERTY(QString updateSuccessfulDrones READ updateSuccessfulDrones NOTIFY updateSuccessfulDronesChanged);
    Q_PROPERTY(QString updateFailedDrones READ updateFailedDrones NOTIFY updateFailedDronesChanged);
    Q_PROPERTY(int newDroneId READ newDroneId NOTIFY newDroneIdChanged);

    //Q_PROPERTY(QList<QString> connectedDroneListNames READ connectedDroneListNames NOTIFY connectedDroneListNamesChanged)
    //Q_PROPERTY(QList<QString> connectedDroneListStatuses READ connectedDroneListStatuses NOTIFY connectedDroneListStatusesChanged)



    int monarkState() const { return m_monarkState;}
    int groundRadioUpdateState() const { return m_groundRadioUpdateState;}

    QString allDrones() const;
    QString beforeUpdateDrones() const;
    QString updateInProgressDrones() const;
    QString updateSuccessfulDrones() const;
    QString updateFailedDrones() const;

    int newDroneId() const{return m_newDroneId;}

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
    Q_INVOKABLE void changeEncryptionKey(QString const& desiredEncryptionKey);




signals:


    void monarkStateChanged(int monarkState);
    void groundRadioUpdateStateChanged(int updateState);
    void allDronesChanged();
    void beforeUpdateDronesChanged();
    void updateInProgressDronesChanged();
    void updateSuccessfulDronesChanged();
    void updateFailedDronesChanged();
    void newDroneIdChanged();



private:

    void _initializeNetworkId(bool paired);
    void _initializeFrequency(bool paired);
    void _initializeTxPower(bool paired);


    void _pingAllDrones();

    void _setMonarkState(MonarkState monarkState);

    bool _changeGroundRadioFrequency(std::string const& desiredFrequency, bool reversion);

    bool _changeGroundRadioEncryptionKey(std::string const& currentEncryptionKey, std::string const& desiredKey, bool reversion);

    bool _changeGroundRadioTxPower(std::string const& desiredPower);

    void _resetToBeforeUpdate();

    void _waitForPingResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>>& pingDroneResponses, char const*const p_successStr, std::function<void(int)> const& responseGoodFunc, std::function<void(int)> const& responseBadFunc);

protected:
    std::unique_ptr<MonarkManagerWorkerWorker> mp_slotHandler;
    MonarkSettings*          mp_monarkSettings;
    MonarkQRCodeProvider*    mp_monarkQRCodeProvider;
    std::set<int> m_beforeUpdateDrones;
    std::set<int> m_updateInProgressDrones;
    std::set<int> m_updateSuccessfulDrones;
    std::set<int> m_updateFailedDrones;
    QAtomicInteger<int> m_groundRadioUpdateState;
private:
    QAtomicInteger<int> m_monarkState;
    std::mutex m_monarkStateMut;
    std::condition_variable m_monarkStateCondition;
    int m_newDroneId;

};
