#include "MonarkManager.h"
#include "QGCApplication.h"
#include "Settings/SettingsManager.h"

#include <libssh/libssh.h>

#include <QQmlEngine>
#include <future>

QGC_LOGGING_CATEGORY(MonarkManagerLog, "MonarkManagerLog")

namespace
{
static constexpr inline char const*const np_sshUsername = "admin";
static constexpr inline char const*const np_srmDefaultIp="192.168.168.1";
static constexpr inline char const*const np_srmPairedIp="172.20.1.2";
static constexpr inline char const*const np_srocIp="172.20.1.1";

bool waitForResponse(int isStdErr, ssh_channel p_channel)
{
    bool commandSuccess=false;
    char p_buffer[4096];
    char* p_bufferItr = &p_buffer[0];
    auto const start = std::chrono::system_clock::now();
    for(;;)
    {
        if(!p_channel)
        {
            break;
        }
        if(!ssh_channel_is_open(p_channel))
        {
            break;
        }
        if(ssh_channel_poll_timeout(p_channel,2000,isStdErr)>0)
        {
            int numBytesRead = ssh_channel_read(p_channel, p_bufferItr, sizeof(p_buffer) - (p_bufferItr - (&p_buffer[0])),isStdErr);
            p_bufferItr+=numBytesRead;
            if(numBytesRead>0)
            {
                std::string result(p_buffer,p_bufferItr - (&p_buffer[0]));
                if(result.find("OK") != std::string::npos)
                {
                    commandSuccess=true;
                }
            }
        }
        if(commandSuccess)
        {
            break;
        }
        if(std::chrono::system_clock::now() - start > std::chrono::seconds(15))
        {
            break;
        }
        if(size_t(p_bufferItr - (&p_buffer[0])) >= sizeof(p_buffer))
        {
            break;
        }
    }
    return commandSuccess;
}

std::string _connectToSRM(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands)
{
    qCDebug(MonarkManagerLog)<<"ENTER: ::_connectToSRM("<<p_host<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    std::string returnStr="";
    ssh_channel p_channel = nullptr;
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_new failed";
            returnStr="ssh_new failed";
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_options_set failed";
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_connect failed";
            break;
        }
        isConnected=true;
        qCInfo(MonarkManagerLog)<<":_connectToSRM("<<p_host<<") : successfully connected";
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, np_sshUsername, p_password);
            if(returnCode)
            {
                qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                returnStr="ssh_userauth_password failed";
                break;
            }
            if(p_commands)
            {

                p_channel=ssh_channel_new(p_session);
                if(!p_channel)
                {
                    qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_new failed";
                    break;
                }
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_open_session failed";
                    break;
                }
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_request_shell failed";
                    break;
                }
                size_t commandIndex=0;
                for(;;)
                {
                    auto const& command=(*p_commands)[commandIndex];
                    ssh_channel_write(p_channel,command.c_str(), command.size());
                    auto const commandSuccess = waitForResponse(0,p_channel);
                    if(!commandSuccess)
                    {
                        qCCritical(MonarkManagerLog)<<"::_connectToSRM("<<p_host<<") : command at index "<<commandIndex<<" failed";
                        returnStr="command at index "+std::to_string(commandIndex)+" failed";
                        break;
                    }
                    if(++commandIndex==p_commands->size())
                    {

                        break;
                    }
                }

            }
        }
        if(returnStr.empty())
        {
            returnStr="SUCCESS";
        }
    }
    while(false);
    if(p_channel)
    {
        ssh_channel_close(p_channel);
        ssh_channel_free(p_channel);
        p_channel=nullptr;
    }
    if(isConnected)
    {
        ssh_disconnect(p_session);
        isConnected=false;
    }
    if(p_session)
    {
        ssh_free(p_session);
        p_session=nullptr;
    }
    ssh_finalize();
    qCDebug(MonarkManagerLog)<<"EXIT : ::_connectToSRM("<<p_host<<")";
    return returnStr;
}
}


   MonarkManagerWorkerWorker::MonarkManagerWorkerWorker()
        : m_taskQueueCondition{}
        , m_taskQueueMut{}
        , m_taskQueue{}
        , m_shutdown{false}
    {

    }
    bool MonarkManagerWorkerWorker::needDispatch()
    {
        return QThread::currentThread() != this;
    }
    void MonarkManagerWorkerWorker::dispatch(std::function<void()> t)
    {
        QMutexLocker lock(&m_taskQueueMut);
        m_taskQueue.enqueue(t);
        m_taskQueueCondition.wakeOne();
    }
    void MonarkManagerWorkerWorker::shutdown()
    {
        if(needDispatch())
        {
            dispatch([this](){m_shutdown=true;});
            QThread::wait();
        }
        else
        {
            QThread::terminate();
        }
    }

    void MonarkManagerWorkerWorker::run()
    {
        while(!m_shutdown)
        {
            m_taskQueueMut.lock();
            while(m_taskQueue.isEmpty())
            {
                m_taskQueueCondition.wait(&m_taskQueueMut);
            }
            auto const t = m_taskQueue.dequeue();
            m_taskQueueMut.unlock();
            t();
        }
    }


MonarkManager::MonarkManager(QGCApplication*const p_app, QGCToolbox*const p_toolbox)
    : QGCTool{p_app, p_toolbox}
    , mp_slotHandler{std::make_unique<MonarkManagerWorkerWorker>()}
    , m_monarkState{(int)MonarkState::BeforeScan}
    , mp_monarkSettings{nullptr}
    , m_paired{false}
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::MonarkManager()()";
    mp_slotHandler->start();
    qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::MonarkManager()()";
}

MonarkManager::~MonarkManager() {
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::~MonarkManager()()";
    mp_slotHandler->shutdown();
    qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::~MonarkManager()()";
}

void MonarkManager::setToolbox(QGCToolbox *const p_toolbox)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::setToolbox()";
    QGCTool::setToolbox(p_toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MonarkManager> ("QGroundControl.MonarkManager", 1, 0, "MonarkManager", "Reference only");
    mp_monarkSettings = p_toolbox->settingsManager()->monarkSettings();
    connect(this,
            &MonarkManager::monarkStateChanged,
            this,
            [](int const monarkState)
            {
                qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::monarkStateChanged slot() scanState="<<(int)monarkState;

                qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::monarkStateChanged slot() scanState="<<(int)monarkState;
            });
    qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::setToolbox()";
}

void MonarkManager::startScanning()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){startScanning();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::startScanning()";
        m_monarkState=(int)MonarkState::ScanInProgress;
        emit monarkStateChanged(m_monarkState);
        auto scanningResult=MonarkState::ScanFailedNotDetected; //TODO this is a placeholder
        auto pingPairedResponseFuture = std::async(std::launch::async,[this](){
            auto password = this->mp_monarkSettings->encryptionKey()->cookedValueString();
            return _connectToSRM(np_srmPairedIp, password.toStdString().c_str(), nullptr);
        });
        auto const pingDefaultResponse=_connectToSRM(np_srmDefaultIp, nullptr, nullptr);
        if(pingDefaultResponse == "SUCCESS")
        {
            scanningResult=MonarkState::ScanSuccessPairingRequired;
            m_paired=false;
        }
        else
        {
            auto const pingPairedResponse = pingPairedResponseFuture.get();
            if(pingPairedResponse == "ssh_userauth_password failed")
            {
                scanningResult = MonarkState::ScanSuccessBadCredentials;
                m_paired=true;
            }
            else if(pingPairedResponse=="SUCCESS")
            {
                scanningResult=MonarkState::ScanSuccessAndPaired;
                m_paired=true;
            }
            else
            {
                m_paired=false;
            }
        }
        m_monarkState=(int)scanningResult;
        emit monarkStateChanged(m_monarkState);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::startScanning()";
    }
}

void MonarkManager::saveFlutterManagementSettings()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){saveFlutterManagementSettings();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::saveFlutterManagementSettings()";
        m_monarkState=(int)MonarkState::SaveSettingsInProgress;
        emit monarkStateChanged(m_monarkState);
        auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString().toStdString();
        auto frequency=mp_monarkSettings->groundFrequency()->cookedValueString().toStdString();
        auto networkId=mp_monarkSettings->networkID()->cookedValueString().toStdString();
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        using namespace std::string_literals;
        std::vector<std::string> commands;
        if(!m_paired)
        {
            commands.emplace_back("AT+MWRADIO=1\n");
        }
        commands.emplace_back("AT+MWTXPOWER="+txPower+"\n");
        commands.emplace_back("AT+MWFREQ="+frequency+"\n");
        commands.emplace_back("AT+MWNETWORKID="+networkId+"\n");
        commands.emplace_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
        commands.emplace_back("AT+MSPWD="+encryptionKey+","+encryptionKey+"\n");
        if(!m_paired)
        {
            commands.emplace_back("AT+MWVMODE=0\n");
            commands.emplace_back("AT+MNLAN=LAN,EDIT,0,"s+np_srmPairedIp+",255.255.0.0,0\n");
            commands.emplace_back("AT+MNLANDHCP=LAN,1,"s+np_srocIp+",1,0\n");
        }
        commands.emplace_back("AT&W\n");
        auto saveResult=MonarkState::SaveSettingsFailed;
        auto const& returnStr = _connectToSRM(m_paired?np_srmPairedIp:np_srmDefaultIp,encryptionKey.c_str(), &commands);
        if(returnStr=="SUCCESS")
        {
            saveResult=MonarkState::SaveSettingsSuccess;
        }
        m_monarkState=(int)saveResult;
        emit monarkStateChanged(m_monarkState);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::saveFlutterManagementSettings()";
    }
}
