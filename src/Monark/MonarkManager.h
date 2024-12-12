#pragma once

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
#include "MonarkDrone.h"
Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)


class MonarkDrone;

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

        //SaveSettingsSuccess=7,
        SaveSettingsFailed=7,
        BeforePairNewDrone=8,
        ShowQRCode=9,
        ResetUnpairMonark=10,
        DetectionFailed=11,
        //DetectionInProgress=8,
        //DetectionSuccess=9,
        //DetectionFailed=10,


    };

    MonarkManager(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkManager();

    //TODO Q_PROPERTYs go here
    Q_PROPERTY(int monarkState READ monarkState NOTIFY monarkStateChanged)

    Q_PROPERTY(QList<MonarkDrone> connectedDroneList READ connectedDroneList NOTIFY connectedDroneListChanged)


    //TODO public getters go here
    int monarkState() const { return m_monarkState;}

    QList<MonarkDrone> const& connectedDroneList() const { return m_connectedDroneList;}


    //TODO public setters go here

    virtual void setToolbox(QGCToolbox* p_toolbox) override;

    Q_INVOKABLE void startScanning();

    Q_INVOKABLE void saveFlutterManagementSettings();

    Q_INVOKABLE void saveEncryptionKey();


    Q_INVOKABLE void detect();

    Q_INVOKABLE void gotoBeforePairNewDrone();


    static std::string connectToMicrohard(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands);


signals:


    void monarkStateChanged(int monarkState);
    void connectedDroneListChanged(QList<MonarkDrone> const& connectedDroneList);

private:
    void _initializeNetworkId(bool paired);

    void _pingAllDrones();

protected:
    std::unique_ptr<MonarkManagerWorkerWorker> mp_slotHandler;
    QAtomicInteger<int> m_monarkState;
    MonarkSettings*          mp_monarkSettings;
    MonarkQRCodeProvider*    mp_monarkQRCodeProvider;
    QList<MonarkDrone> m_connectedDroneList;
    //bool m_paired;

};
