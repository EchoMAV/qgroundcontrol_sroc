#include "MonarkManager.h"
#include "QGCApplication.h"
#include "Settings/SettingsManager.h"
#include "MonarkQRCodeProvider.h"

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
static constexpr inline char const*const np_successStr = "\r\nOK\r\n";
static constexpr inline uint16_t const n_defaultGroundTxPower= 20;
static constexpr inline uint16_t const n_defaultGroundFrequency = 1711;

}

std::string MonarkManager::connectToMicrohard(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands)
{
    // returnStr will be an error text if something went wrong executing commands. Otherwise, it will be the microhard output of the
    // last ran command in the list.
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::connectToMicrohard("<<p_host<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    std::string returnStr="";
    ssh_channel p_channel = nullptr;
    char p_buffer[4096];
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_new failed";
            returnStr="ssh_new failed";
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_options_set failed";
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_connect failed";
            break;
        }
        isConnected=true;
        qCInfo(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : successfully connected";
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, np_sshUsername, p_password);
            if(returnCode)
            {
                qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                returnStr="ssh_userauth_password failed";
                break;
            }
            if(p_commands && !p_commands->empty())
            {
                p_channel=ssh_channel_new(p_session);
                if(!p_channel)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_new failed";
                    break;
                }
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_open_session failed";
                    break;
                }
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_request_shell failed";
                    break;
                }
                size_t commandIndex=0;
                for(;;)
                {
                    auto const& command=(*p_commands)[commandIndex];
                    //command is sent
                    ssh_channel_write(p_channel,command.c_str(), command.size());
                    bool commandSuccess=false;
                    char* p_bufferItr = &p_buffer[0];
                    auto const start = std::chrono::system_clock::now();
                    for(;;)
                    {
                        if(!ssh_channel_is_open(p_channel))
                        {
                            break;
                        }
                        if(ssh_channel_poll_timeout(p_channel,2000,0)>0)
                        {
                            int numBytesRead = ssh_channel_read(p_channel, p_bufferItr, sizeof(p_buffer) - (p_bufferItr - (&p_buffer[0])),0);
                            p_bufferItr+=numBytesRead;
                            if(numBytesRead>0)
                            {
                                returnStr=  std::string(p_buffer,p_bufferItr - (&p_buffer[0]));
                                //qCDebug(MonarkManagerLog)<<"returnStr='";
                                //for(auto c : returnStr)
                                //{
                                //    qCDebug(MonarkManagerLog)<<c << "which is "<<int(c);
                                //}
                                //qCDebug(MonarkManagerLog)<<"' (endReturnStr)";
                                if(returnStr.find(np_successStr) != std::string::npos)
                                {

                                    commandSuccess=true;
                                    break;
                                }
                            }
                        }
                        if(std::chrono::system_clock::now() - start > std::chrono::seconds(30))
                        {
                            break;
                        }
                        if(size_t(p_bufferItr - (&p_buffer[0])) >= sizeof(p_buffer))
                        {
                            break;
                        }
                    }
                    if(command.find("AT+MWVENCRYPT") == std::string::npos && command.find("AT+MSPWD") == std::string::npos)
                    {
                        //don't put passwords in the logs
                        qCDebug(MonarkManagerLog)<<"MonarkManager::connectToMicrohard("<<p_host<<") command="<<command.c_str()<<", commandSuccess="<<commandSuccess<<" returnStr="<<returnStr.c_str();
                    }
                    if(++commandIndex==p_commands->size() || !commandSuccess)
                    {
                        break;
                    }
                }
            }
        }
        if(returnStr.empty() && (!p_commands || p_commands->empty()))
        {
            returnStr=np_successStr;
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
    qCDebug(MonarkManagerLog)<<"EXIT MonarkManager::connectToMicrohard("<<p_host<<")";
    return returnStr;
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
    , mp_monarkQRCodeProvider{nullptr}
    , m_connectedDroneIDs{}
    , m_connectedDroneList{}
    //, m_paired{false}
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
    mp_monarkQRCodeProvider = p_toolbox->monarkQRCodeProvider();
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
        assert(m_monarkState == (int)MonarkState::BeforeScan
             ||m_monarkState == (int)MonarkState::ScanFailedNotDetected );
        m_monarkState=(int)MonarkState::ScanInProgress;
        emit monarkStateChanged(m_monarkState);
        auto scanningResult=MonarkState::ScanFailedNotDetected;
        auto pingPairedResponseFuture = std::async(std::launch::async,[this](){
            auto password = this->mp_monarkSettings->encryptionKey()->cookedValueString();
            return connectToMicrohard(np_srmPairedIp, password.toStdString().c_str(), nullptr);
        });


        auto const pingDefaultResponse=connectToMicrohard(np_srmDefaultIp, nullptr, nullptr);


        if(pingDefaultResponse == np_successStr)
        {
            qCDebug(MonarkManagerLog)<<"ScanSuccessPairingRequired";
            scanningResult=MonarkState::ScanSuccessPairingRequired;
            this->mp_monarkSettings->encryptionKey()->setCookedValue("");
            this->mp_monarkSettings->groundFrequency()->setCookedValue(n_defaultGroundFrequency);
            this->mp_monarkSettings->groundTxPower()->setCookedValue(n_defaultGroundTxPower);
            _initializeNetworkId(false);
        }
        else
        {

            auto const pingPairedResponse = pingPairedResponseFuture.get();
            if(pingPairedResponse == "ssh_userauth_password failed")
            {
                qCDebug(MonarkManagerLog)<<"ScanSuccessBadCredentials";
                scanningResult = MonarkState::ScanSuccessBadCredentials;
                _initializeNetworkId(true);
            }
            else if(pingPairedResponse==np_successStr)
            {
                m_connectedDroneList.clear();
                m_connectedDroneIDs.clear();
                std::vector<std::future<std::string>> dronePingResponses;
                for(auto i=0;i<255;++i)
                {
                    dronePingResponses.push_back(std::async(std::launch::async,[i](){
                        auto const ip = "172.20.2."+std::to_string(i);
                        return connectToMicrohard(ip.c_str(), nullptr, nullptr);
                    }));
                }
                qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                scanningResult=MonarkState::ScanSuccessAndPaired;

                _initializeNetworkId(true);
                for(auto i=0;i<255;++i)
                {
                    auto responseStr = dronePingResponses[i].get();
                    if(np_successStr==responseStr)
                    {
                        m_connectedDroneIDs.push_back(i);
                        m_connectedDroneList.push_back(QString("MONARK ")+i);
                    }
                }
                emit connectedDroneListChanged(m_connectedDroneList);
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"ScanFailedNotDetected";
            }
        }
        m_monarkState=(int)scanningResult;
        emit monarkStateChanged(m_monarkState);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::startScanning()";
    }
}

void MonarkManager::_initializeNetworkId(bool paired)
{
    std::string macAddress="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MNEMAC\n");
    auto encryptionKey=paired?mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& returnStr = connectToMicrohard(paired?np_srmPairedIp:np_srmDefaultIp,encryptionKey.c_str(), &commands);
    //qCDebug(MonarkManagerLog)<<"returnStr = '"<<returnStr.c_str()<<"' (end returnStr)";

    if(returnStr.find(np_successStr) != std::string::npos)
    {
        auto macStart = returnStr.find_first_of('"',0);
        auto macEnd = returnStr.find_first_of('"',macStart+1);
        if(macStart != std::string::npos && macEnd!= std::string::npos && macStart != macEnd)
        {
            ++macStart;
            macAddress = returnStr.substr(macStart, macEnd-macStart);
            macAddress.erase(std::remove(std::begin(macAddress), std::end(macAddress),':'), std::end(macAddress));
        }
    }
    if(!macAddress.empty())
    {
        this->mp_monarkSettings->networkID()->setCookedValue("MONARK-"+QString::fromStdString(macAddress));
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
        assert(m_monarkState == (int)MonarkState::ScanSuccessPairingRequired
            || m_monarkState == (int)MonarkState::ScanSuccessBadCredentials
            || m_monarkState == (int)MonarkState::SaveSettingsFailed );
        auto const paired = m_monarkState==(int)MonarkState::ScanSuccessBadCredentials;
        m_monarkState=(int)MonarkState::SaveSettingsInProgress;
        emit monarkStateChanged(m_monarkState);
        std::vector<std::string> commands;
        if(!paired)
        {
            using namespace std::string_literals;
            auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString().toStdString();
            auto frequency=mp_monarkSettings->groundFrequency()->cookedValueString().toStdString();
            auto networkId=mp_monarkSettings->networkID()->cookedValueString().toStdString();
            auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
            commands.emplace_back("AT+MWRADIO=1\n");
            commands.emplace_back("AT+MWTXPOWER="+txPower+"\n");
            commands.emplace_back("AT+MWFREQ="+frequency+"\n");
            commands.emplace_back("AT+MWNETWORKID="+networkId+"\n");
            commands.emplace_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
            commands.emplace_back("AT+MSPWD="+encryptionKey+","+encryptionKey+"\n");
            commands.emplace_back("AT+MWVMODE=0\n");
            commands.emplace_back("AT+MNLAN=LAN,EDIT,0,"s+np_srmPairedIp+",255.255.0.0,0\n");
            commands.emplace_back("AT+MNLANDHCP=LAN,1,"s+np_srocIp+",1,0\n");
            commands.emplace_back("AT&W\n");
        }
        auto saveResult=paired ? MonarkState::ScanSuccessBadCredentials: MonarkState::SaveSettingsFailed;
        auto const& returnStr = connectToMicrohard(paired?np_srmPairedIp:np_srmDefaultIp,paired?mp_monarkSettings->getOldEncryptionKey().toStdString().c_str():np_sshUsername, &commands);
        if(returnStr.find(np_successStr)!= std::string::npos)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            mp_monarkSettings->onSaveSettings();
        }
        m_monarkState=(int)saveResult;
        emit monarkStateChanged(m_monarkState);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::saveFlutterManagementSettings()";
    }
}

void MonarkManager::saveEncryptionKey()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){saveEncryptionKey();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::saveEncryptionKey()";
#if 0
        assert(m_monarkState == (int)MonarkState::ScanSuccessBadCredentials);
        m_monarkState=(int)MonarkState::SaveSettingsInProgress;
        emit monarkStateChanged(m_monarkState);
        auto const oldEncryptionKey = mp_monarkSettings->getOldEncryptionKey();
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        using namespace std::string_literals;
        std::vector<std::string> commands;
        commands.emplace_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
        commands.emplace_back("AT+MSPWD="+encryptionKey+","+encryptionKey+"\n");
        commands.emplace_back("AT&W\n");
        auto saveResult=MonarkState::ScanSuccessBadCredentials;
        auto const& returnStr = connectToMicrohard(np_srmPairedIp,oldEncryptionKey.toStdString().c_str(), &commands);
        if(returnStr.find(np_successStr)!= std::string::npos)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            mp_monarkSettings->onSaveSettings();
        }
        m_monarkState=(int)saveResult;
        emit monarkStateChanged(m_monarkState);
#endif
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::saveEncryptionKey()";
    }
}

void MonarkManager::detect()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){detect();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::detect()";
#if 0
        m_monarkState=(int)MonarkState::DetectionInProgress;
        emit monarkStateChanged(m_monarkState);
        auto detectionResult=MonarkState::DetectionFailed;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        std::vector<std::string> commands;
        //TODO populate commands
        std::string ip = "172.20.2."+std::to_string(mp_monarkSettings->monarkID()->cookedValue().toUInt());
        auto const startTime = std::chrono::system_clock::now();
        for(;;)
        {
            if((std::chrono::system_clock::now()-startTime) > std::chrono::minutes(2))
            {
                break;
            }
            auto const& returnStr = connectToMicrohard(ip.c_str(),encryptionKey.c_str(), &commands);
            if(returnStr.find(np_successStr)!= std::string::npos)
            {
                detectionResult=MonarkState::DetectionSuccess;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        m_monarkState=(int)detectionResult;
        emit monarkStateChanged(m_monarkState);
#endif
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::detect()";
    }

}
