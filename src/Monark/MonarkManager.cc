#include "MonarkManager.h"
#include "QGCApplication.h"
#include "Settings/SettingsManager.h"
#include "MonarkQRCodeProvider.h"
#include "MonarkDrone.h"
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
static constexpr inline char const*const np_groundRadioSuccessStr = "\r\nOK\r\n";
static constexpr inline char const*const np_droneSuccessStr="'is_success': True";
static constexpr inline uint16_t const n_defaultGroundTxPower= 20;
static constexpr inline uint16_t const n_defaultGroundFrequency = 1711;

}



std::string MonarkManager::sendCommands(char const*const p_host, char const*const p_password, std::vector<std::string> const*const p_commands, bool toDrone)
{
    // returnStr will be an error text if something went wrong executing commands. Otherwise, it will be the microhard output of the
    // last ran command in the list.
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::sendCommands("<<p_host<<")";
    //qCDebug(MonarkManagerLog)<<"password = '"<<p_password<<"'";
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
            qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_new failed";
            returnStr="ssh_new failed";
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_options_set failed";
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            returnStr="ssh_connect failed";
            break;
        }
        isConnected=true;
        qCInfo(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : successfully connected";
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, np_sshUsername, p_password);
            if(returnCode)
            {
                qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                returnStr="ssh_userauth_password failed";
                break;
            }
            if(p_commands && !p_commands->empty())
            {
                p_channel=ssh_channel_new(p_session);
                if(!p_channel)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_new failed";
                    break;
                }
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    returnStr="ssh_channel_open_session failed";
                    break;
                }
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
                                if(returnStr.find(toDrone?np_droneSuccessStr:np_groundRadioSuccessStr) != std::string::npos)
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
                    if(command.find(toDrone?"export ENCRYPTION_KEY=":"AT+MWVENCRYPT") == std::string::npos && command.find(toDrone?"export NEW_ENCRYPTION_KEY=":"AT+MSPWD") == std::string::npos)
                    {
                        //don't put passwords in the logs
                        qCDebug(MonarkManagerLog)<<"MonarkManager::sendCommands("<<p_host<<") command="<<command.c_str()<<", commandSuccess="<<commandSuccess<<" returnStr="<<returnStr.c_str();
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
            returnStr=toDrone?np_droneSuccessStr:np_groundRadioSuccessStr;
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
    qCDebug(MonarkManagerLog)<<"EXIT MonarkManager::sendCommands("<<p_host<<")";
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
    , mp_monarkSettings{nullptr}
    , mp_monarkQRCodeProvider{nullptr}
    , m_connectedDroneList{}
    , m_groundRadioUpdateState{(int)MonarkDrone::UpdateState::BeforeUpdate}
    , m_monarkState{(int)MonarkState::BeforeScan}
    , m_monarkStateMut{}
    , m_monarkStateCondition{}
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::MonarkManager()()";
    qRegisterMetaType<QList<MonarkDrone>>();
    connect(this, &MonarkManager::_pingDroneSuccess, this, &MonarkManager::_onPingDroneSuccess);
    mp_slotHandler->start();



    qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::MonarkManager()()";
}

 void MonarkManager::_setMonarkState(MonarkState monarkState)
{
    {
        std::lock_guard<std::mutex> lock(m_monarkStateMut);
        m_monarkState=(int)monarkState;
    }
    m_monarkStateCondition.notify_all();
    emit monarkStateChanged(m_monarkState);
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
        _setMonarkState(MonarkState::ScanInProgress);
        auto scanningResult=MonarkState::ScanFailedNotDetected;
        auto pingPairedResponseFuture = std::async(std::launch::async,[this](){
            auto password = this->mp_monarkSettings->encryptionKey()->cookedValueString();
            return sendCommands(np_srmPairedIp, password.toStdString().c_str(), nullptr, false);
        });
        auto const pingDefaultResponse=sendCommands(np_srmDefaultIp, nullptr, nullptr, false);
        if(pingDefaultResponse == np_groundRadioSuccessStr)
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
            else if(pingPairedResponse==np_groundRadioSuccessStr)
            {
                m_connectedDroneList.clear();
                std::vector<std::future<std::string>> dronePingResponses;
                for(auto i=0;i<255;++i)
                {
                    dronePingResponses.push_back(std::async(std::launch::async,[i](){
                        auto const ip = "172.20.2."+std::to_string(i);
                        return sendCommands(ip.c_str(), nullptr, nullptr, true);
                    }));
                }
                qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                scanningResult=MonarkState::ScanSuccessAndPaired;

                _initializeNetworkId(true);
                for(auto i=0;i<255;++i)
                {
                    auto responseStr = dronePingResponses[i].get();
                    //if(np_droneSuccessStr==responseStr)
                    {
                        emit _pingDroneSuccess(i);
                        //m_connectedDroneList.push_back(MonarkDrone (i, this));
                    }
                }
                //emit connectedDroneListChanged(m_connectedDroneList);
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"ScanFailedNotDetected";
            }
        }
        _setMonarkState(scanningResult);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::startScanning()";
    }
}

void MonarkManager::_initializeNetworkId(bool paired)
{
    std::string macAddress="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MNEMAC\n");
    auto encryptionKey=paired?mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& returnStr = sendCommands(paired?np_srmPairedIp:np_srmDefaultIp,encryptionKey.c_str(), &commands, false);
    if(returnStr.find(np_groundRadioSuccessStr) != std::string::npos)
    {
        auto macStart = returnStr.find_first_of('"',0);
        auto macEnd = returnStr.find_first_of('"',macStart+1);
        if(macStart != std::string::npos && macEnd!= std::string::npos && macStart != macEnd)
        {
            ++macStart;
            macAddress = returnStr.substr(macStart, macEnd-macStart);
            macAddress.erase(std::remove(std::begin(macAddress), std::end(macAddress),':'), std::end(macAddress));
        }
        if(macAddress.length()>4)
        {
            macAddress=macAddress.substr(macAddress.length()-4,4);
        }
    }
    if(!macAddress.empty())
    {
        this->mp_monarkSettings->networkID()->setCookedValue("MONARK-"+QString::fromStdString(macAddress));
    }
}


void MonarkManager::gotoBeforePairNewDrone()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoBeforePairNewDrone();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoBeforePairNewDrone()";
        _setMonarkState(MonarkState::BeforePairNewDrone);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoBeforePairNewDrone()";
    }
}

void MonarkManager::gotoChangeEncryptionKey()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoChangeEncryptionKey();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeEncryptionKey()";
        for(auto& connectedDrone : m_connectedDroneList)
        {
            connectedDrone.setUpdateState((int)MonarkDrone::UpdateState::BeforeUpdate);
        }
        m_groundRadioUpdateState=(int)MonarkDrone::UpdateState::BeforeUpdate;
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        _setMonarkState(MonarkState::ChangeEncryptionKey);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoChangeEncryptionKey()";
    }
}

void MonarkManager::gotoChangeFrequencies()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoChangeFrequencies();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeFrequencies()";
        for(auto& connectedDrone : m_connectedDroneList)
        {
            connectedDrone.setUpdateState((int)MonarkDrone::UpdateState::BeforeUpdate);
        }
        m_groundRadioUpdateState=(int)MonarkDrone::UpdateState::BeforeUpdate;
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        _setMonarkState(MonarkState::ChangeFrequencies);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoChangeFrequencies()";
    }
}

void MonarkManager::gotoChangeTxPower()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoChangeTxPower();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeTxPower()";
        for(auto& connectedDrone : m_connectedDroneList)
        {
            connectedDrone.setUpdateState((int)MonarkDrone::UpdateState::BeforeUpdate);
        }
        m_groundRadioUpdateState=(int)MonarkDrone::UpdateState::BeforeUpdate;
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        _setMonarkState(MonarkState::ChangeTxPower);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoChangeTxPower()";
    }
}

void MonarkManager::gotoResetUnpairMonark()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoResetUnpairMonark();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoResetUnpairMonark()";
        _setMonarkState(MonarkState::ResetUnpairMonark);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoResetUnpairMonark()";
    }
}

void MonarkManager::gotoScanSuccessAndPaired()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){gotoScanSuccessAndPaired();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoScanSuccessAndPaired()";
        _setMonarkState(MonarkState::ScanSuccessAndPaired);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoScanSuccessAndPaired()";
    }
}

void MonarkManager::gotoDetectionFailed()
{
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoDetectionFailed()";
        _setMonarkState(MonarkState::DetectionFailed);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::gotoDetectionFailed()";
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
        auto const paired = m_monarkState==(int)MonarkState::ScanSuccessBadCredentials;
        _setMonarkState(MonarkState::SaveSettingsInProgress);
        std::vector<std::string> commands;
        if(!paired)
        {
            using namespace std::string_literals;
            auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString().toStdString();
            auto frequency=mp_monarkSettings->groundFrequency()->cookedValueString().toStdString();
            auto networkId=mp_monarkSettings->networkID()->cookedValueString().toStdString();
            auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
            commands.emplace_back("AT+MWRADIO=1\n");
            commands.emplace_back("AT+MWDISTANCE=8047\n"); //acceptable RF distance 5 miles
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
        auto const& returnStr = sendCommands(paired?np_srmPairedIp:np_srmDefaultIp,paired?mp_monarkSettings->getOldEncryptionKey().toStdString().c_str():np_sshUsername, &commands, false);
        if(returnStr.find(np_groundRadioSuccessStr)!= std::string::npos)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            mp_monarkSettings->onSaveSettings();
        }
        _setMonarkState(saveResult);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::saveFlutterManagementSettings()";
    }
}


void MonarkManager::detect()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){detect();});
#if 0
        if(returnVal)
        {
            qCDebug(MonarkManagerLog)<<"pushing to connected drone list";

            //have to do this in the main thread
            m_connectedDroneList.push_back(MonarkDrone (mp_monarkSettings->monarkID()->cookedValue().toUInt(), this));
            emit connectedDroneListChanged(m_connectedDroneList);
        }
#endif
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::detect()";
        _setMonarkState(MonarkState::ShowQRCode);
        auto detectionResult=MonarkState::DetectionFailed;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        auto monarkID = mp_monarkSettings->monarkID()->cookedValue().toUInt();
        std::string ip = "172.20.2."+std::to_string(monarkID);
        auto const startTime = std::chrono::system_clock::now();
        std::vector<std::string> commands;
        commands.push_back("export ENCRYPTION_KEY="+encryptionKey);
        commands.push_back("microhard --action=info");
        int attemptCount=0;
        for(;;)
        {
            if((std::chrono::system_clock::now()-startTime) > std::chrono::minutes(3))
            {
                break;
            }
            auto const& returnStr = sendCommands(ip.c_str(),encryptionKey.c_str(), &commands, true);
            qCDebug(MonarkManagerLog)<<"returnStr = '"<<returnStr.c_str()<<"'";
            if(returnStr.find(np_droneSuccessStr)!= std::string::npos

                || (++attemptCount==3)) //TODO remove this
            {
                qCDebug(MonarkManagerLog)<<"found drone";
                emit _pingDroneSuccess(monarkID);
                detectionResult=MonarkState::ScanSuccessAndPaired;
                break;
            }
            std::unique_lock<std::mutex> lock(m_monarkStateMut);
            m_monarkStateCondition.wait_for(lock,std::chrono::seconds(1),[this](){
                return m_monarkState==(int)MonarkState::DetectionFailed;
            });
            lock.unlock();
            if(m_monarkState == (int)MonarkState::DetectionFailed)
            {
                break;
            }
        }
        _setMonarkState(detectionResult);
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::detect()";
    }

}

void MonarkManager::_onPingDroneSuccess(int monarkId)
{
    m_connectedDroneList.push_back(MonarkDrone (monarkId, this));
    qCDebug(MonarkManagerLog)<<"_onPingDroneSuccess list size = "<<m_connectedDroneList.size();

    emit connectedDroneListChanged();
}

void MonarkManager::_sendCommandsToRadioAndDrones(std::string const& currentEncryptionKey, std::vector<std::string> const& groundRadioCommands, std::vector<std::string> const& droneCommands)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_sendCommandsToRadioAndDrones()";
     std::vector<std::future<std::string>> droneResponses;
    for(auto& connectedDrone : m_connectedDroneList)
    {
        connectedDrone.setUpdateState((int)MonarkDrone::UpdateState::UpdateInProgress);
        droneResponses.push_back(std::async(std::launch::async,[&connectedDrone,&currentEncryptionKey,&groundRadioCommands](){
            auto const ip = "172.20.2."+std::to_string(connectedDrone.droneId());
            return sendCommands(ip.c_str(),currentEncryptionKey.c_str(), &groundRadioCommands,true);
        }));
    }
    m_groundRadioUpdateState=(int) MonarkDrone::UpdateState::UpdateInProgress;
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    bool failed=false;
    for(size_t i=0;i<droneResponses.size();++i)
    {
        auto const& responseStr=droneResponses[i].get();
        auto response=(int) MonarkDrone::UpdateState::UpdateFailed;
        if(responseStr.find(np_droneSuccessStr)== std::string::npos)
        {
            failed=true;
        }
        else
        {
            response=(int) MonarkDrone::UpdateState::UpdateSuccessful;
        }
        m_connectedDroneList[i].setUpdateState((int)response);
    }
    if(failed)
    {
        m_groundRadioUpdateState=(int) MonarkDrone::UpdateState::UpdateFailed;
    }
    else
    {
        auto const& returnStr = sendCommands(np_srmPairedIp,currentEncryptionKey.c_str(), &groundRadioCommands, false);
        if(returnStr.find(np_groundRadioSuccessStr)== std::string::npos)
        {
            m_groundRadioUpdateState=(int) MonarkDrone::UpdateState::UpdateFailed;
        }
        else
        {
            m_groundRadioUpdateState=(int) MonarkDrone::UpdateState::UpdateSuccessful;
        }
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::_sendCommandsToRadioAndDrones()";
}

void MonarkManager::changeTxPower(QString const& desiredTxPower)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this, desiredTxPower](){changeTxPower(desiredTxPower);});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeTxPower()";
        auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        auto const desiredStdString=desiredTxPower.toStdString();
        std::vector<std::string> groundRadioCommands;
        std::vector<std::string> droneCommands;
        groundRadioCommands.emplace_back("AT+MWTXPOWER="+desiredStdString+"\n");
        groundRadioCommands.emplace_back("AT&W\n");
        droneCommands.emplace_back("export ENCRYPTION_KEY="+currentEncryptionKey+"\n");
        droneCommands.emplace_back("micohard --action=update --tx_power="+desiredStdString+"\n");
        _sendCommandsToRadioAndDrones(currentEncryptionKey, groundRadioCommands,droneCommands);
        if(m_groundRadioUpdateState == (int)MonarkDrone::UpdateState::UpdateSuccessful)
        {
            mp_monarkSettings->groundTxPower()->setRawValue(desiredTxPower.toUInt());
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::changeTxPower()";
    }
}
void MonarkManager::changeFrequencies(QString const& desiredFrequency)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this,desiredFrequency](){changeFrequencies(desiredFrequency);});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeFrequencies()";
        auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        auto const desiredStdString=desiredFrequency.toStdString();
        std::vector<std::string> groundRadioCommands;
        std::vector<std::string> droneCommands;
        groundRadioCommands.emplace_back("AT+MWFREQ="+desiredStdString+"\n");
        groundRadioCommands.emplace_back("AT&W\n");
        droneCommands.emplace_back("export ENCRYPTION_KEY="+currentEncryptionKey+"\n");
        droneCommands.emplace_back("micohard --action=update --frequency="+desiredStdString+"\n");
        _sendCommandsToRadioAndDrones(currentEncryptionKey, groundRadioCommands,droneCommands);
        if(m_groundRadioUpdateState == (int)MonarkDrone::UpdateState::UpdateSuccessful)
        {
            mp_monarkSettings->groundFrequency()->setRawValue(desiredFrequency.toUInt());
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::changeFrequencies()";
    }
}
void MonarkManager::changeEncryptionKey(QString const& currentEncryptionKey, QString const& desiredEncryptionKey)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this,currentEncryptionKey,desiredEncryptionKey](){changeEncryptionKey(currentEncryptionKey,desiredEncryptionKey );});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeEncryptionKey()";
        auto const desiredStdString=desiredEncryptionKey.toStdString();
        std::vector<std::string> groundRadioCommands;
        std::vector<std::string> droneCommands;
        groundRadioCommands.emplace_back("AT+MWVENCRYPT=2,"+desiredStdString+"\n");
        groundRadioCommands.emplace_back("AT+MSPWD="+desiredStdString+","+desiredStdString+"\n");
        groundRadioCommands.emplace_back("AT&W\n");
        droneCommands.emplace_back("export ENCRYPTION_KEY="+currentEncryptionKey.toStdString()+"\n");
        droneCommands.emplace_back("export NEW_ENCRYPTION_KEY="+desiredStdString+"\n");
        droneCommands.emplace_back("microhard --action=update_encryption_key\n");
        _sendCommandsToRadioAndDrones(currentEncryptionKey.toStdString(), groundRadioCommands,droneCommands);
        if(m_groundRadioUpdateState == (int)MonarkDrone::UpdateState::UpdateSuccessful)
        {
            mp_monarkSettings->encryptionKey()->setRawValue(desiredEncryptionKey);
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT:  MonarkManager::changeEncryptionKey()";
    }
}
