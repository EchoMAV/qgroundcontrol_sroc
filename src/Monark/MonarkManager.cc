#include "MonarkManager.h"
#include "QGCApplication.h"
#include "Settings/SettingsManager.h"
#include "MonarkQRCodeProvider.h"
#include <libssh/libssh.h>

#include <QQmlEngine>
#include <sstream>
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

std::string _getDroneIPAddress(int monarkID)
{
    return "172.20.3."+std::to_string(monarkID);
}

QString _convertSetToString(std::set<int> const& set, bool const includeGroundRadio)
{
    std::stringstream ss;
    bool needComma=false;
    for(auto id : set)
    {
        if(needComma)
        {
            ss<<", ";
        }
        ss<<"MONARK-"<<id;
        needComma=true;
    }
    if(includeGroundRadio)
    {
        if(needComma)
        {
            ss<<", ";
        }
        ss<<"Ground Radio";
    }
    return QString::fromStdString(ss.str());
}

std::pair<bool,std::vector<std::string>> _sendCommands(char const*const p_host, char const*const p_username, char const*const p_password, std::vector<std::string> const*const p_commands, bool toDrone)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _sendCommands(p_host="<<p_host<<", p_username="<<p_username<<", toDrone="<<toDrone<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    ssh_channel p_channel = nullptr;
    char p_buffer[4096];
    bool commandsSent=false;
    std::vector<std::string> commandResponses;
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_new failed";
            commandResponses.push_back("ssh_new failed");
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            commandResponses.push_back("ssh_options_set failed");
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            commandResponses.push_back("ssh_connect failed");
            break;
        }
        isConnected=true;
        qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : successfully connected";
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, p_username, p_password);
            if(returnCode)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                commandResponses.push_back("ssh_userauth_password failed");
                break;
            }
            qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : authenticated";
            if(p_commands && !p_commands->empty())
            {
                p_channel=ssh_channel_new(p_session);
                if(!p_channel)
                {
                    qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    commandResponses.push_back("ssh_channel_new failed");
                    break;
                }
                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : created channel";
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    commandResponses.push_back("ssh_channel_open_session failed");
                    break;
                }
                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : opened session";
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    commandResponses.push_back("ssh_channel_request_shell failed");
                    break;
                }
                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : requested shell";
                commandsSent=true;
                size_t commandIndex=0;
                for(;;)
                {
                    std::string responseStr;
                    auto const& command=(*p_commands)[commandIndex];
                    //command is sent
                    ssh_channel_write(p_channel,command.c_str(), command.size());
                    //qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : wrote command '"<<command.c_str()<<"'";
                    bool commandSuccess=false;
                    if(command.find("export")==0)
                    {
                        //no response is expected
                        commandSuccess=true;
                        responseStr=np_droneSuccessStr;
                    }
                    else
                    {
                        //expect a successful response
                        char* p_bufferItr = &p_buffer[0];
                        auto const start = std::chrono::system_clock::now();
                        for(;;)
                        {
                            if(!ssh_channel_is_open(p_channel))
                            {
                                break;
                            }
                            qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_is_open=true";
                            if(ssh_channel_poll_timeout(p_channel,2000,0)>0)
                            {
                                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_poll_timeout(p_channel,2000,0)>0=true";
                                int numBytesRead = ssh_channel_read(p_channel, p_bufferItr, sizeof(p_buffer) - (p_bufferItr - (&p_buffer[0])),0);
                                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : numBytesRead="<<numBytesRead;
                                p_bufferItr+=numBytesRead;
                                if(numBytesRead>0)
                                {
                                    responseStr=  std::string(p_buffer,p_bufferItr - (&p_buffer[0]));
                                    qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : responseStr="<<   responseStr.c_str();
                                    if(responseStr.find(toDrone?np_droneSuccessStr:np_groundRadioSuccessStr) != std::string::npos)
                                    {
                                        commandSuccess=true;
                                        break;
                                    }

                                }
                            }
                            else
                            {
                                qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_poll_timeout(p_channel,2000,0)>0=false";
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
                    }
                    commandResponses.push_back(responseStr);
                    if(toDrone?(command.find("export NEWEK=") == std::string::npos):(command.find("AT+MWVENCRYPT") == std::string::npos && command.find("AT+MSPWD") == std::string::npos))
                    {
                        //don't put passwords in the logs
                        qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") command="<<command.c_str()<<", commandSuccess="<<commandSuccess<<" returnStr="<<commandResponses.back().c_str();
                    }
                    if(++commandIndex==p_commands->size() || !commandSuccess)
                    {
                        break;
                    }
                }
            }
        }
        if(commandResponses.empty() && (!p_commands || p_commands->empty()))
        {
            commandResponses.push_back(toDrone?np_droneSuccessStr:np_groundRadioSuccessStr);
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
    qCDebug(MonarkManagerLog)<<"EXIT : _sendCommands(p_host="<<p_host<<", p_username="<<p_username<<", toDrone="<<toDrone<<")";
    return std::make_pair(commandsSent,commandResponses);
}

void _waitForDroneResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>>& droneResponses, std::set<int>& succeededDrones, std::set<int>& failedDrones)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _waitForDroneResponses()";
    for(size_t i=0;i<droneResponses.size();++i)
    {
        auto id=droneResponses[i].first;
        auto const& response=droneResponses[i].second.get().second;
        if(response.empty() || response.back().find(np_droneSuccessStr)== std::string::npos)
        {
            failedDrones.insert(id);
        }
        else
        {
            succeededDrones.insert(id);
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _waitForDroneResponses()";
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneFrequencyChangeCommands(std::string const& desiredFrequency, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency.c_str()<<")";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses;
    for(auto itr = std::begin(beforeSet);;)
    {
        if(itr==std::end(beforeSet))
        {
            break;
        }
        auto id = *itr;
        itr = beforeSet.erase(itr);
        inProgressSet.insert(id);
        droneResponses.push_back(std::make_pair(id,std::async(std::launch::async,[id,&desiredFrequency](){
                                                    auto ip=_getDroneIPAddress(id);
                                                    std::vector<std::string> droneCommands;
                                                    droneCommands.emplace_back("microhard --action=update --frequency="+desiredFrequency+" --monark_id="+std::to_string(id)+"\n");
                                                    return _sendCommands(ip.c_str(),"monark", "monark", &droneCommands,true);
                                                })));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency.c_str()<<")";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneEncryptionKeyChangeCommands(std::string const& desiredEncryptionKey, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneEncryptionKeyChangeCommands()";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses;
    for(auto itr = std::begin(beforeSet);;)
    {
        if(itr==std::end(beforeSet))
        {
            break;
        }
        auto id = *itr;
        itr = beforeSet.erase(itr);
        inProgressSet.insert(id);
        droneResponses.push_back(std::make_pair(id,std::async(std::launch::async,[id,&desiredEncryptionKey](){
                                                    auto ip=_getDroneIPAddress(id);
                                                    std::vector<std::string> droneCommands;
                                                    droneCommands.emplace_back("export NEWEK="+desiredEncryptionKey+"\n");
                                                    droneCommands.emplace_back("microhard --action=update_encryption_key --monark_id="+std::to_string(id)+"\n");
                                                    return _sendCommands(ip.c_str(),"monark", "monark", &droneCommands,true);
                                                })));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneEncryptionKeyChangeCommands()";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneTxPowerChangeCommands(std::string const& desiredTxPower, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneTxPowerChangeCommands(desiredTxPower="<<desiredTxPower.c_str()<<")";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses;
    for(auto itr = std::begin(beforeSet);;)
    {
        if(itr==std::end(beforeSet))
        {
            break;
        }
        auto id = *itr;
        itr = beforeSet.erase(itr);
        inProgressSet.insert(id);
        droneResponses.push_back(std::make_pair(id,std::async(std::launch::async,[id,&desiredTxPower](){
                                                    auto ip=_getDroneIPAddress(id);
                                                    std::vector<std::string> droneCommands;
                                                    droneCommands.push_back("microhard --action=update --tx_power="+desiredTxPower+" --monark_id="+std::to_string(id)+"\n");
                                                    return _sendCommands(ip.c_str(),"monark", "monark", &droneCommands,true);
                                                })));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneTxPowerChangeCommands(desiredTxPower="<<desiredTxPower.c_str()<<")";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDronePingCommands(std::set<int> const& inputSet)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _sendDronePingCommands()";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses;
    for(auto itr = std::begin(inputSet);;)
    {
        if(itr==std::end(inputSet))
        {
            break;
        }
        auto id = *itr;
        ++itr;
        pingDroneResponses.push_back(std::make_pair(id,std::async(std::launch::async,[id](){
                                                        auto ip=_getDroneIPAddress(id);
                                                        std::this_thread::sleep_for(std::chrono::seconds(2));
                                                        std::vector<std::string> pingCommand;
                                                        pingCommand.push_back("microhard --action=info --monark_id="+std::to_string(id)+"\n");
                                                        auto const startTime = std::chrono::system_clock::now();
                                                        std::pair<bool,std::vector<std::string>> response;
                                                        for(;;)
                                                        {

                                                            response = _sendCommands(ip.c_str(),"monark", "monark", &pingCommand, true);
                                                            for(auto responseStr: response.second)
                                                            {
                                                                qCDebug(MonarkManagerLog)<<"returnStr = '"<<responseStr.c_str()<<"'";
                                                            }
                                                            if(!response.second.empty() && response.second.back().find(np_droneSuccessStr)!= std::string::npos
                                                                )
                                                            {
                                                                break;
                                                            }
                                                            if((std::chrono::system_clock::now()-startTime) > std::chrono::seconds(30))
                                                            {
                                                                break;
                                                            }
                                                        }
                                                        return response;
                                                    })));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _sendDronePingCommands()";
    return pingDroneResponses;
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
    , mp_monarkSettings{nullptr}
    , mp_monarkQRCodeProvider{nullptr}
    , m_beforeUpdateDrones{}
    , m_updateInProgressDrones{}
    , m_updateSuccessfulDrones{}
    , m_updateFailedDrones{}
    , m_groundRadioUpdateState{(int)UpdateState::BeforeUpdate}
    , m_monarkState{(int)MonarkState::BeforeScan}
    , m_monarkStateMut{}
    , m_monarkStateCondition{}
    , m_newDroneId{-1}
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::MonarkManager()()";
    mp_slotHandler->start();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::MonarkManager()()";
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
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::~MonarkManager()()";
}

void MonarkManager::setToolbox(QGCToolbox *const p_toolbox)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::setToolbox()";
    QGCTool::setToolbox(p_toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MonarkManager> ("QGroundControl.MonarkManager", 1, 0, "MonarkManager", "Reference only");
    mp_monarkSettings = p_toolbox->settingsManager()->monarkSettings();
    mp_monarkQRCodeProvider = p_toolbox->monarkQRCodeProvider();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::setToolbox()";
}

QString MonarkManager::allDrones() const
{
    std::set<int> allDrones;
    allDrones.insert(std::begin(m_beforeUpdateDrones),std::end(m_beforeUpdateDrones));
    allDrones.insert(std::begin(m_updateInProgressDrones),std::end(m_updateInProgressDrones));
    allDrones.insert(std::begin(m_updateSuccessfulDrones),std::end(m_updateSuccessfulDrones));
    allDrones.insert(std::begin(m_updateFailedDrones),std::end(m_updateFailedDrones));
    if(allDrones.empty())
    {
        return "None";
    }
    else
    {
        return _convertSetToString(allDrones,false);
    }
}

QString MonarkManager::beforeUpdateDrones() const
{
    return _convertSetToString(m_beforeUpdateDrones, m_groundRadioUpdateState==(int)UpdateState::BeforeUpdate);
}

QString MonarkManager::updateInProgressDrones() const
{
    return _convertSetToString(m_updateInProgressDrones, m_groundRadioUpdateState==(int)UpdateState::UpdateInProgress);
}

QString MonarkManager::updateSuccessfulDrones() const
{
    return _convertSetToString(m_updateSuccessfulDrones, m_groundRadioUpdateState==(int)UpdateState::UpdateSuccessful);
}

QString MonarkManager::updateFailedDrones() const
{
    return _convertSetToString(m_updateFailedDrones, m_groundRadioUpdateState==(int)UpdateState::UpdateFailed);
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
            return _sendCommands(np_srmPairedIp, "admin", password.toStdString().c_str(), nullptr, false);
        });
        auto const pingDefaultResponse=_sendCommands(np_srmDefaultIp, "admin", nullptr, nullptr, false).second;
        for(auto str: pingDefaultResponse)
        {
            qCDebug(MonarkManagerLog)<<"ping default responseStr="<<str.c_str();
        }
        if(!pingDefaultResponse.empty() && pingDefaultResponse.back() == np_groundRadioSuccessStr)
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
            auto const pingPairedResponse = pingPairedResponseFuture.get().second;
            for(auto str: pingPairedResponse)
            {
                qCDebug(MonarkManagerLog)<<"ping paired responseStr="<<str.c_str();
            }
            if(!pingPairedResponse.empty() && pingPairedResponse.back() == "ssh_userauth_password failed")
            {
                qCDebug(MonarkManagerLog)<<"ScanSuccessBadCredentials";
                scanningResult = MonarkState::ScanSuccessBadCredentials;
                _initializeNetworkId(true);
            }
            else if(!pingPairedResponse.empty() && pingPairedResponse.back() ==np_groundRadioSuccessStr)
            {
                m_beforeUpdateDrones.clear();
                m_updateInProgressDrones.clear();
                m_updateSuccessfulDrones.clear();
                m_updateFailedDrones.clear();

                std::vector<std::future<std::vector<std::string>>> dronePingResponses;
                for(auto i=0;i<255;++i)
                {
                    dronePingResponses.push_back(std::async(std::launch::async,[i](){
                        auto const ip = "172.20.2."+std::to_string(i);
                        return _sendCommands(ip.c_str(), "admin", nullptr, nullptr, true).second;
                    }));
                }
                qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                scanningResult=MonarkState::ScanSuccessAndPaired;

                _initializeNetworkId(true);
                for(auto i=0;i<255;++i)
                {
                    auto response = dronePingResponses[i].get();
                    if(!response.empty() && response.back() == np_droneSuccessStr)
                    {
                        m_beforeUpdateDrones.insert(i);
                    }
                }
                emit allDronesChanged();
                emit beforeUpdateDronesChanged();
                emit updateInProgressDronesChanged();
                emit updateSuccessfulDronesChanged();
                emit updateFailedDronesChanged();
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"ScanFailedNotDetected";
            }
        }
        _setMonarkState(scanningResult);
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::startScanning()";
    }
}

void MonarkManager::_initializeNetworkId(bool paired)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeNetworkId(paired="<<paired<<")";
    std::string macAddress="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MNEMAC\n");
    auto encryptionKey=paired?mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& response = _sendCommands(paired?np_srmPairedIp:np_srmDefaultIp,"admin", encryptionKey.c_str(), &commands, false).second;
    if(!response.empty() && response.back().find(np_groundRadioSuccessStr) != std::string::npos)
    {
        auto macStart = response.back().find_first_of('"',0);
        auto macEnd = response.back().find_first_of('"',macStart+1);
        if(macStart != std::string::npos && macEnd!= std::string::npos && macStart != macEnd)
        {
            ++macStart;
            macAddress = response.back().substr(macStart, macEnd-macStart);
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
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeNetworkId(paired="<<paired<<")";
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoBeforePairNewDrone()";
    }
}

void MonarkManager::_resetToBeforeUpdate()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_resetToBeforeUpdate()";
    for(auto id:(m_updateInProgressDrones))
    {
        m_beforeUpdateDrones.insert(id);
    }
    m_updateInProgressDrones.clear();
    for(auto id:(m_updateSuccessfulDrones))
    {
        m_beforeUpdateDrones.insert(id);
    }
    m_updateSuccessfulDrones.clear();
    for(auto id:(m_updateFailedDrones))
    {
        m_beforeUpdateDrones.insert(id);
    }
    m_updateFailedDrones.clear();
    m_groundRadioUpdateState=(int)UpdateState::BeforeUpdate;
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_resetToBeforeUpdate()";
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
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeEncryptionKey);
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeEncryptionKey()";
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
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeFrequencies);
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeFrequencies()";
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
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeTxPower);
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeTxPower()";
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoResetUnpairMonark()";
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoScanSuccessAndPaired()";
    }
}

void MonarkManager::gotoDetectionFailed()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoDetectionFailed()";
    _setMonarkState(MonarkState::DetectionFailed);
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoDetectionFailed()";
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
        auto const& response = _sendCommands(paired?np_srmPairedIp:np_srmDefaultIp,"admin", paired?mp_monarkSettings->getOldEncryptionKey().toStdString().c_str():np_sshUsername, &commands, false).second;
        if(!response.empty() && response.back().find(np_groundRadioSuccessStr)!= std::string::npos)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            mp_monarkSettings->onSaveSettings();
        }
        _setMonarkState(saveResult);
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::saveFlutterManagementSettings()";
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
        _setMonarkState(MonarkState::ShowQRCode);
        auto detectionResult=MonarkState::DetectionFailed;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        auto monarkID = mp_monarkSettings->monarkID()->cookedValue().toUInt();
        std::string ip = _getDroneIPAddress(monarkID);
        auto const startTime = std::chrono::system_clock::now();
        std::vector<std::string> commands;
        commands.push_back("microhard --action=info --monark_id="+std::to_string(monarkID)+"\n");
        //int attemptCount=0;
        //TODO upon success, bring up a message box that indicates success
        for(;;)
        {
            if((std::chrono::system_clock::now()-startTime) > std::chrono::minutes(3))
            {
                break;
            }
            auto const& response = _sendCommands(ip.c_str(),"monark", "monark", &commands, true).second;
            for(auto responseStr: response)
            {
                qCDebug(MonarkManagerLog)<<"returnStr = '"<<responseStr.c_str()<<"'";
            }
            if(!response.empty() && response.back().find(np_droneSuccessStr)!= std::string::npos

                //|| (++attemptCount==3)//TODO remove this
                )
            {
                qCDebug(MonarkManagerLog)<<"found drone";
                m_beforeUpdateDrones.insert(monarkID);
                m_updateInProgressDrones.erase(monarkID);
                m_updateSuccessfulDrones.erase(monarkID);
                m_updateFailedDrones.erase(monarkID);
                emit beforeUpdateDronesChanged();
                emit updateInProgressDronesChanged();
                emit updateSuccessfulDronesChanged();
                emit updateFailedDronesChanged();
                emit allDronesChanged();
                detectionResult=MonarkState::ScanSuccessAndPaired;
                m_newDroneId=monarkID;
                emit newDroneIdChanged();
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::detect()";
    }

}

bool MonarkManager::_changeGroundRadioFrequency(std::string const& desiredFrequency, bool reversion)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency.c_str()<<", reversion="<<reversion<<")";
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
    std::vector<std::string> groundRadioCommands;
    groundRadioCommands.emplace_back("AT+MWFREQ="+desiredFrequency+"\n");
    groundRadioCommands.emplace_back("AT&W\n");
    auto const& response = _sendCommands(np_srmPairedIp,"admin", currentEncryptionKey.c_str(), &groundRadioCommands, false);
    if(response.second.empty() || response.second.back().find(np_groundRadioSuccessStr)== std::string::npos)
    {
        m_groundRadioUpdateState=reversion?(int) UpdateState::UpdateSuccessful:(int) UpdateState::UpdateFailed;
    }
    else
    {
        m_groundRadioUpdateState=reversion?(int)UpdateState::UpdateFailed:(int) UpdateState::UpdateSuccessful;
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency.c_str()<<", reversion="<<reversion<<")";
    return response.first;
}

bool MonarkManager::_changeGroundRadioEncryptionKey(std::string const& desiredKey)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioEncryptionKey()";
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
    std::vector<std::string> groundRadioCommands;
    groundRadioCommands.emplace_back("AT+MWVENCRYPT=2,"+desiredKey+"\n");
    groundRadioCommands.emplace_back("AT+MSPWD="+desiredKey+","+desiredKey+"\n");
    groundRadioCommands.emplace_back("AT&W\n");
    auto const& response = _sendCommands(np_srmPairedIp,"admin", currentEncryptionKey.c_str(), &groundRadioCommands, false);
    if(response.second.empty() || response.second.back().find(np_groundRadioSuccessStr)== std::string::npos)
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
    }
    else
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateSuccessful;
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioEncryptionKey()";
    return response.first;
}

bool MonarkManager::_changeGroundRadioTxPower(std::string const& desiredPower)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioTxPower(desiredPower="<<desiredPower.c_str()<<")";
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
    std::vector<std::string> groundRadioCommands;
    groundRadioCommands.emplace_back("AT+MWTXPOWER="+desiredPower+"\n");
    groundRadioCommands.emplace_back("AT&W\n");
    auto const& response = _sendCommands(np_srmPairedIp,"admin", currentEncryptionKey.c_str(), &groundRadioCommands, false);
    if(response.second.empty() || response.second.back().find(np_groundRadioSuccessStr)== std::string::npos)
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
    }
    else
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateSuccessful;
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateSuccessfulDronesChanged();
    emit updateFailedDronesChanged();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioTxPower(desiredPower="<<desiredPower.c_str()<<")";
    return response.first;
}

void MonarkManager::_waitForPingResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>>& pingDroneResponses, std::function<void(int)> const& responseGoodFunc, std::function<void(int)> const& responseBadFunc)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_waitForPingResponses()";
    for(size_t i=0;i<pingDroneResponses.size();++i)
    {
        auto id=pingDroneResponses[i].first;
        auto const& response=pingDroneResponses[i].second.get().second;
        m_updateInProgressDrones.erase(id);
        emit updateInProgressDronesChanged();
        if(response.empty() || response.back().find(np_droneSuccessStr)== std::string::npos)
        {
            responseBadFunc(id);
        }
        else
        {
            responseGoodFunc(id);
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_waitForPingResponses()";
}


void MonarkManager::changeTxPower(QString const& desiredTxPower)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this, desiredTxPower](){changeTxPower(desiredTxPower);});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower.toStdString().c_str()<<")";
        _resetToBeforeUpdate();
        auto const desiredStdString=desiredTxPower.toStdString();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses=_sendDroneTxPowerChangeCommands(desiredStdString,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        _changeGroundRadioTxPower(desiredStdString);
        _waitForPingResponses(droneResponses,[this](int id){

            m_updateSuccessfulDrones.insert(id);
            emit updateSuccessfulDronesChanged();
        },
                              [this](int id){
                                  m_updateFailedDrones.insert(id);
                                  emit updateFailedDronesChanged();
                              }
                              );
        if(m_groundRadioUpdateState == (int)UpdateState::UpdateSuccessful)
        {
            qCDebug(MonarkManagerLog)<<"Setting Tx Power to "<<desiredTxPower.toUInt();
            mp_monarkSettings->groundTxPower()->setRawValue(desiredTxPower.toUInt());
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower.toStdString().c_str()<<")";
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
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency.toStdString().c_str()<<")";
        _resetToBeforeUpdate();
        auto const desiredStdString=desiredFrequency.toStdString();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses=_sendDroneFrequencyChangeCommands(desiredStdString,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        std::set<int> failedDrones;
        std::set<int> succeededDrones;
        _waitForDroneResponses(droneResponses,succeededDrones,failedDrones);
        if(failedDrones.empty())
        {
            if(_changeGroundRadioFrequency(desiredStdString,false))
            {
                qCDebug(MonarkManagerLog)<<"Ground radio frequency change successful";
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                _waitForPingResponses(pingDroneResponses,
                                      [this](int id){
                                          m_updateSuccessfulDrones.insert(id);
                                          emit updateSuccessfulDronesChanged();
                                      },
                                      [this](int id){
                                          m_updateFailedDrones.insert(id);
                                          emit updateFailedDronesChanged();
                                      }
                                      );

                if(m_updateFailedDrones.empty())
                {
                     qCDebug(MonarkManagerLog)<<"All drones successfully pinged";
                }
                else
                {
                    auto const oldFrequency=std::to_string(mp_monarkSettings->groundFrequency()->rawValue().toUInt());
                    if(m_updateSuccessfulDrones.empty())
                    {
                        qCDebug(MonarkManagerLog)<<"All drones failed to change frequencies. Reverting ground radio";
                        //all of the drones failed to update, so revert the ground radio
                        _changeGroundRadioFrequency(oldFrequency,true);
                    }
                    else
                    {
                        qCDebug(MonarkManagerLog)<<"Some drones failed. Some drones succeeded. Reverting drones that succeeded";
                        //some of the drones DID succeed, so send reversion commands to them
                        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> revertDroneResponses=_sendDroneFrequencyChangeCommands(oldFrequency,m_updateSuccessfulDrones,m_updateInProgressDrones);
                        emit updateSuccessfulDronesChanged();
                        emit updateInProgressDronesChanged();
                        for(size_t i=0;i<revertDroneResponses.size();++i)
                        {
                            droneResponses[i].second.wait();
                        }
                        //revert the ground radio back
                        if(_changeGroundRadioFrequency(oldFrequency,true))
                        {
                            qCDebug(MonarkManagerLog)<<"Ground radio reverted. Pinging drones";
                            //try to ping the drones that we reverted
                            std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> revertPingDroneResponses = _sendDronePingCommands(m_updateInProgressDrones);
                            _waitForPingResponses(revertPingDroneResponses,
                                                  [this](int id){
                                                      m_updateFailedDrones.insert(id);
                                                      emit updateFailedDronesChanged();
                                                  },
                                                  [this](int id){
                                                      m_updateSuccessfulDrones.insert(id);
                                                      emit updateSuccessfulDronesChanged();
                                                  }
                                                  );
                        }
                        else
                        {
                            qCDebug(MonarkManagerLog)<<"Ground radio reversion failed";
                            //unable to revert the ground radio back, so it is impossible to ping the drones to check if they changed
                            //instead, set those drones to failed (assumed) but set the ground radio to a success state
                            m_updateFailedDrones=m_updateInProgressDrones;
                            m_updateInProgressDrones.clear();
                            emit updateFailedDronesChanged();
                            emit updateInProgressDronesChanged();
                        }
                    }
                }
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"Commands sent to drones, but ground radio could not be changed";
                //all the commands were successfully sent, but we can't change the ground radio frequency
                //therefore, it is impossible to ping the drones for success
                //assume success on the drones, but set the ground radio state to failure
                m_updateSuccessfulDrones=succeededDrones;
                m_updateInProgressDrones.clear();
                emit updateSuccessfulDronesChanged();
                emit updateInProgressDronesChanged();
            }
        }
        else if(!succeededDrones.empty())
        {
            qCDebug(MonarkManagerLog)<<"Failed to send commands to some drones, but not all. Moving ground radio to new frequency to begin reversion";
            m_updateFailedDrones=failedDrones;
            failedDrones.clear();
            emit updateFailedDronesChanged();
            //some drones failed to get commands, but some succeeded
            if(_changeGroundRadioFrequency(desiredStdString,false))
            {
                qCDebug(MonarkManagerLog)<<"Ground radio on new frequency. Pinging drones";
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                for(size_t i=0;i<pingDroneResponses.size();++i)
                {
                    auto id=pingDroneResponses[i].first;
                    auto const& response=pingDroneResponses[i].second.get().second;
                    if(response.empty() || response.back().find(np_droneSuccessStr)== std::string::npos)
                    {
                        succeededDrones.erase(id);
                        m_updateInProgressDrones.erase(id);
                        emit updateInProgressDronesChanged();
                        m_updateFailedDrones.insert(id);
                        emit updateFailedDronesChanged();
                    }
                }
                auto const oldFrequency=std::to_string(mp_monarkSettings->groundFrequency()->rawValue().toUInt());
                if(succeededDrones.empty())
                {
                    qCDebug(MonarkManagerLog)<<"No drones pinged. Reverting ground radio";
                    //no drones were successfully changed, so revert the ground radio and you're done
                    _changeGroundRadioFrequency(oldFrequency,true);
                }
                else
                {
                    qCDebug(MonarkManagerLog)<<"Some drones pinged. Sending reversion commands to drones";
                    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> revertDroneResponses=_sendDroneFrequencyChangeCommands(oldFrequency,succeededDrones,failedDrones);
                    for(size_t i=0;i<revertDroneResponses.size();++i)
                    {
                        droneResponses[i].second.wait();
                    }
                    //revert the ground radio back
                    if(_changeGroundRadioFrequency(oldFrequency,true))
                    {
                        qCDebug(MonarkManagerLog)<<"Ground radio reverted. Pinging drones";
                        //try to ping the drones that we reverted
                        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> revertPingDroneResponses = _sendDronePingCommands(failedDrones);
                        _waitForPingResponses(revertPingDroneResponses,
                                              [this](int id){
                                                  m_updateFailedDrones.insert(id);
                                                  emit updateFailedDronesChanged();
                                              },
                                              [this](int id){
                                                  m_updateSuccessfulDrones.insert(id);
                                                  emit updateSuccessfulDronesChanged();
                                              }
                                              );
                    }
                    else
                    {
                        qCDebug(MonarkManagerLog)<<"Ground radio reversion failed.";
                        //unable to revert the ground radio back, so it is impossible to ping the drones to check if they changed
                        //instead, set those drones to failed (assumed) but set the ground radio to a success state
                        m_updateFailedDrones.insert(std::begin(failedDrones), std::end(failedDrones));
                        m_updateInProgressDrones.clear();
                        emit updateFailedDronesChanged();
                        emit updateInProgressDronesChanged();
                    }
                }

            }
            else
            {
                qCDebug(MonarkManagerLog)<<"Ground radio could not be changed to new frequency";
                //we can't ping the drones, so we assume they succeeded
                m_updateSuccessfulDrones=succeededDrones;
                m_updateInProgressDrones.clear();
                emit updateSuccessfulDronesChanged();
                emit updateInProgressDronesChanged();
            }
        }
        else
        {
            qCDebug(MonarkManagerLog)<<"No drones successfully received the command";
            //all drones failed to receive the command, so don't bother changing the ground radio
            m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
            emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
            m_updateFailedDrones=m_updateInProgressDrones;
            m_updateInProgressDrones.clear();
            emit beforeUpdateDronesChanged();
            emit updateInProgressDronesChanged();
            emit updateSuccessfulDronesChanged();
            emit updateFailedDronesChanged();
        }
        if(m_groundRadioUpdateState == (int)UpdateState::UpdateSuccessful)
        {
            qCDebug(MonarkManagerLog)<<"Setting frequency to "<<desiredFrequency.toUInt();
            mp_monarkSettings->groundFrequency()->setRawValue(desiredFrequency.toUInt());
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency.toStdString().c_str()<<")";
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
        _resetToBeforeUpdate();
        auto const desiredStdString=desiredEncryptionKey.toStdString();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses=_sendDroneEncryptionKeyChangeCommands(desiredStdString,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        std::set<int> failedDrones;
        std::set<int> succeededDrones;
        _waitForDroneResponses(droneResponses,succeededDrones,failedDrones);
        if(failedDrones.empty())
        {
            if(_changeGroundRadioEncryptionKey(desiredStdString))
            {
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                _waitForPingResponses(pingDroneResponses,
                                      [this](int id){
                                          m_updateSuccessfulDrones.insert(id);
                                          emit updateSuccessfulDronesChanged();
                                      },
                                      [this](int id){
                                          m_updateFailedDrones.insert(id);
                                          emit updateFailedDronesChanged();
                                      }
                                      );
                qCDebug(MonarkManagerLog)<<"All drones and ground radio succeeded";
            }
            else
            {
                 qCDebug(MonarkManagerLog)<<"Commands successfully sent to drones, but ground radio faile to change key";
                //we can't change the ground radio's frequency
                //therefore, we can't ping the drones to determine success
                //Assume success in those drones, but set the ground radio to failure
                m_updateSuccessfulDrones=succeededDrones;
                emit updateSuccessfulDronesChanged();
            }
        }
        else if(!succeededDrones.empty())
        {
            qCDebug(MonarkManagerLog)<<"Some drones succeeded, some failed. Setting ground radio encryption key";
            m_updateFailedDrones.insert(std::begin(failedDrones), std::end(failedDrones));
            emit updateFailedDronesChanged();
            if(_changeGroundRadioEncryptionKey(desiredStdString))
            {
                qCDebug(MonarkManagerLog)<<"Ground radio changed. Pinging drones";

                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                _waitForPingResponses(pingDroneResponses,
                                      [this](int id){
                                          m_updateSuccessfulDrones.insert(id);
                                          emit updateSuccessfulDronesChanged();
                                      },
                                      [this](int id){
                                          m_updateFailedDrones.insert(id);
                                          emit updateFailedDronesChanged();
                                      }
                                      );
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"Could not change ground radio encryption key";
                //we can't change the ground radio's frequency
                //therefore, we can't ping the drones to determine success
                //Assume success in those drones, but set all other drones and the ground radio to failure
                m_updateSuccessfulDrones=succeededDrones;
                emit updateSuccessfulDronesChanged();
            }
            m_updateInProgressDrones.clear();
            emit updateInProgressDronesChanged();
        }
        else
        {
            qCDebug(MonarkManagerLog)<<"All drones failed";
            //all drones failed to receive the command, so don't bother changing the ground radio
            m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
            emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
            m_updateFailedDrones=m_updateInProgressDrones;
            m_updateInProgressDrones.clear();
            emit beforeUpdateDronesChanged();
            emit updateInProgressDronesChanged();
            emit updateSuccessfulDronesChanged();
            emit updateFailedDronesChanged();
        }
        if(m_groundRadioUpdateState == (int)UpdateState::UpdateSuccessful)
        {
            mp_monarkSettings->encryptionKey()->setRawValue(desiredEncryptionKey);
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeEncryptionKey()";
    }
}
