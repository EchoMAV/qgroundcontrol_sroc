#pragma once

#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>
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
        SaveSettingsSuccess=7,
        SaveSettingsFailed=8,


    };

    MonarkManager(QGCApplication* p_app, QGCToolbox* p_toolbox);
    ~MonarkManager();

    //TODO Q_PROPERTYs go here
    Q_PROPERTY(int monarkState READ monarkState NOTIFY monarkStateChanged)

    //TODO public getters go here
    int monarkState() const { return m_monarkState;}


    //TODO public setters go here

    virtual void setToolbox(QGCToolbox* p_toolbox) override;

    Q_INVOKABLE void startScanning();

    Q_INVOKABLE void saveFlutterManagementSettings();

    static std::string connectToMicrohard(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands);

signals:
    void monarkStateChanged(int monarkState);

protected:
    std::unique_ptr<MonarkManagerWorkerWorker> mp_slotHandler;
    QAtomicInteger<int> m_monarkState;
    MonarkSettings*          mp_monarkSettings;
    bool m_paired;

};
