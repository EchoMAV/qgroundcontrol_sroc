#include "MonarkManager.h"
#include "QGCApplication.h"
#include "Settings/SettingsManager.h"
#include "MonarkQRCodeProvider.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "qtandroidserialport/src/qserialport_android_p.h"
#include <QQmlEngine>
#include <QDesktopServices>
#include <sstream>
#include <future>
#include <QtAndroidExtras/QAndroidJniObject>
#include "QGCCorePlugin.h"
#include "QGCFileDownload.h"

QGC_LOGGING_CATEGORY(MonarkManagerLog, "MonarkManagerLog")

namespace
{
static constexpr inline char const*const np_sshUsername = "admin";
static constexpr inline char const*const np_srmDefaultIp="192.168.168.1";
static constexpr inline char const*const np_srmPairedIp="172.20.1.2";
static constexpr inline char const*const np_srocIp="172.20.1.1";
static constexpr inline char const*const np_groundRadioSuccessStr = "\r\nOK\r\n";
static constexpr inline char const*const np_droneSuccessStr1="'is_success': True";
static constexpr inline char const*const np_droneSuccessStr2="\"is_success\": true";
static constexpr inline uint16_t const n_defaultGroundTxPower= 20;
static constexpr inline uint16_t const n_defaultGroundFrequency = 2310;
static constexpr inline uint16_t const n_maxMonarkID=3;



std::unordered_map<QString, std::set<QString>> n_validMicrohardFrequencies =
    {
        {"pMDDL2450AES256",{"2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475"}},
        {"pMDDL1624AES256",{"1630","1635","1640","1645","1650","1655","1660","1665","1670","1675","1680","1685","1690","1695","1700","1705","1710","1715","1720","1785","1790","1795","1800","1805","1810","1815","1820","1825","1830","1835","1840","1845","2025","2030","2035","2040","2045","2050","2055","2060","2065","2070","2075","2080","2085","2090","2095","2100","2105",
                            "2205","2210","2215","2220","2225","2230","2235","2240","2245","2250","2255","2260","2265","2270","2275","2280","2285","2290","2295","2305","2310","2315","2320","2325","2330","2335","2340","2345","2350","2355","2360","2365","2370","2375","2380","2385","2405","2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475","2480","2485","2490","2495"}},
        {"pMDDL2280AES256",{"2205","2210","2215","2220","2225","2230","2235","2240","2245","2250","2255","2260","2265","2270","2275","2280","2285","2290","2295","2300","2305","2310","2315","2320","2325","2330","2335","2340","2345","2350","2355","2360","2365","2370","2375","2380","2385","2390","2395","2400","2405","2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475","2480","2485","2490","2495","2500","2505","2510","2515","2520","2525","2530","2535","2540","2545","2550","2555","2560","2565","2570","2575","2580","2585","2590","2595","2600","2605","2610","2615","2620","2625","2630","2635","2640","2645","2650","2655","2660","2665","2670","2675","2680","2685","2690","2695","2700","2705","2710","2715","2720","2725","2730","2735","2740","2745","2750","2755","2760","2765","2770","2775","2780","2785","2790","2795"}}
};

std::unordered_map<QString, std::pair<int,int>> n_validMicrohardPowers =
    {
        {"pMDDL2450AES256",{7,30}},
        {"pMDDL1624AES256",{7,32}},
        {"pMDDL2280AES256",{10,38}}
};



std::set<QString> _initializeValidEchoLinkFrequencies()
{
    //the intersection of all valid sets



    std::set<QString> intersection;
    auto itr1=std::begin(n_validMicrohardFrequencies);
    auto end1=std::end(n_validMicrohardFrequencies);
    if(itr1!=end1)
    {
        intersection=itr1->second;
        for(;++itr1!=end1;)
        {
            std::set<QString> tempIntersection;
            std::set_intersection(std::begin(itr1->second), std::end(itr1->second), std::begin(intersection), std::end(intersection), std::inserter(tempIntersection,std::end(tempIntersection)));
            intersection=std::move(tempIntersection);
        }
    }
    return intersection;



}


static inline std::set<QString> const n_defaultValidEchoLinkFreqs=_initializeValidEchoLinkFrequencies();

std::string _generateRandomKey()
{
    std::string validChars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> dist(0,validChars.length()-1);
    std::stringstream ss;
    for(int i=0;i<16;++i)
    {
        ss<<validChars[dist(generator)];
    }
    return ss.str();
}

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
        ss<<"EchoLink";
    }
    return QString::fromStdString(ss.str());
}

std::pair<bool,std::vector<std::string>> _sendFile(char const*const p_host, char const*const p_username, char const*const p_password, QFile& file, QString const& remoteLocation, bool toDrone, bool quiet=true, std::function<void(int, QString const&)> const& progressFunc=[](int, QString const&){})
{
    //TODO find a way to make the method timeout if you power off the drone
    qCDebug(MonarkManagerLog)<<"ENTER: _sendFile(p_host="<<p_host<<", p_username="<<p_username<<", file="<<file<<", remoteLocation="<<remoteLocation<<", toDrone="<<toDrone<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    sftp_session p_sftp=nullptr;
    sftp_file p_sftpFile=nullptr;
    char p_buffer[4096];
    bool success=false;
    std::vector<std::string> commandResponses;
    int percent = 0;
    size_t fileSize = file.size();
    size_t nextMilestone = size_t(double(fileSize)/100.0);
    size_t totalNumWritten=0;
    progressFunc(percent,"");
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_new failed";
            }
            progressFunc(-1, QString("ssh_new failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_new failed");
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            progressFunc(-1, QString("ssh_options_set failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_options_set failed");
            break;
        }
        long timeoutsec=1;
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_TIMEOUT, &timeoutsec);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            progressFunc(-1, QString("ssh_options_set failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_options_set failed");
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            progressFunc(-1, QString("ssh_connect failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_connect failed");
            break;
        }
        isConnected=true;
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, p_username, p_password);
            if(returnCode)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("ssh_userauth_password failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("ssh_userauth_password failed");
                break;
            }
            p_sftp= sftp_new(p_session);
            if(!p_sftp)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : sftp_new failed: "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("sftp_new failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("sftp_new failed");
                break;
            }
            returnCode =  sftp_init(p_sftp);
            if(returnCode)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : sftp_init failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("sftp_init failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("sftp_init failed");
                break;
            }
            p_sftpFile= sftp_open(p_sftp, remoteLocation.toStdString().c_str(),
                                   O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            if(!p_sftpFile)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : sftp_open failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("sftp_open failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("sftp_open failed");
                break;
            }
            if(!file.open(QFile::ReadOnly))
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : file.open failed";
                }
                commandResponses.push_back("file.open failed");
                progressFunc(-1, "file.open failed");
                break;
            }

            /*
            if(!ssh_handle_packets_termination(p_session,1000,_send_file_termination_callback, &progressFunc))
            {

                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : ssh_handle_packets_termination failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("ssh_handle_packets_termination failed: ")+ssh_get_error(p_session));
                progressFunc(-1, "ssh_handle_packets_termination failed");
            }
*/

            sftp_file_set_nonblocking(p_sftpFile);


            for(;;)
            {
                if(file.atEnd())
                {
                    success=true;
                    progressFunc(100,"");
                    file.close();
                    break;
                }
                auto const numRead=file.read(p_buffer,4096);
                if(numRead>0)
                {
                    auto const numWritten=sftp_write(p_sftpFile,p_buffer,numRead);
                    if(numWritten<numRead)
                    {
                        if(!quiet)
                        {
                            qCCritical(MonarkManagerLog)<<"_sendFile("<<p_host<<") : sftp_write failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                        }
                        progressFunc(-1, QString("sftp_write failed: ")+ssh_get_error(p_session));
                        progressFunc(-1, "sftp_write failed");
                        file.close();
                        break;
                    }
                    totalNumWritten+=numWritten;
                    if(totalNumWritten>=nextMilestone)
                    {
                        progressFunc(++percent,"");
                        nextMilestone = size_t(double(fileSize)/100.0 * double(percent+1));
                    }

                }
            }
        }
        if(commandResponses.empty())
        {
            commandResponses.push_back(toDrone?np_droneSuccessStr2:np_groundRadioSuccessStr);
        }
    }
    while(false);
    if(p_sftpFile)
    {
        sftp_close(p_sftpFile);
        p_sftpFile=nullptr;
    }
    if(p_sftp)
    {
        sftp_free(p_sftp);
        p_sftp=nullptr;
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
    qCDebug(MonarkManagerLog)<<"EXIT : _sendFile(p_host="<<p_host<<", p_username="<<p_username<<", file="<<file<<", remoteLocation="<<remoteLocation<<", toDrone="<<toDrone<<")";
    return std::make_pair(success,commandResponses);
}


std::pair<bool,std::vector<std::string>> _sendCommands(char const*const p_host, char const*const p_username, char const*const p_password, std::vector<std::string> const*const p_commands, bool toDrone, bool quiet=true)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendCommands(p_host="<<p_host<<", p_username="<<p_username<<", toDrone="<<toDrone<<")";
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
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_new failed";
            }
            commandResponses.push_back("ssh_new failed");
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, p_host);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            commandResponses.push_back("ssh_options_set failed");
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            commandResponses.push_back("ssh_connect failed");
            break;
        }
        isConnected=true;
        if(p_password)
        {
            returnCode = ssh_userauth_password(p_session, p_username, p_password);
            if(returnCode)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                commandResponses.push_back("ssh_userauth_password failed");
                break;
            }
            if(p_commands && !p_commands->empty())
            {
                p_channel=ssh_channel_new(p_session);
                if(!p_channel)
                {
                    if(!quiet)
                    {
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_new failed");
                    break;
                }
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    if(!quiet)
                    {
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_open_session failed");
                    break;
                }
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    if(!quiet)
                    {
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<p_host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_request_shell failed");
                    break;
                }
                commandsSent=true;
                size_t commandIndex=0;
                for(;;)
                {
                    std::string responseStr;
                    auto const& command=(*p_commands)[commandIndex];
                    bool writeLog=false;
                    if(!quiet)
                    {
                        if(toDrone?(command.find("export NEWEK=") == std::string::npos):(command.find("AT+MWVENCRYPT") == std::string::npos && command.find("AT+MSPWD") == std::string::npos))
                        {
                            //don't put passwords in the logs
                            qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") command="<<command.c_str();
                            writeLog=true;
                        }
                    }
                    //command is sent
                    ssh_channel_write(p_channel,command.c_str(), command.size());
                    bool commandSuccess=false;
                    if(command.find("export")==0)
                    {
                        //no response is expected
                        commandSuccess=true;
                        responseStr=np_droneSuccessStr2;
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
                            if(ssh_channel_poll_timeout(p_channel,2000,0)>0)
                            {
                                int numBytesRead = ssh_channel_read(p_channel, p_bufferItr, sizeof(p_buffer) - (p_bufferItr - (&p_buffer[0])),0);
                                p_bufferItr+=numBytesRead;
                                if(numBytesRead>0)
                                {
                                    responseStr=  std::string(p_buffer,p_bufferItr - (&p_buffer[0]));
                                    if(!quiet && writeLog)
                                    {
                                        //don't put passwords in the logs
                                        qCDebug(MonarkManagerLog)<<"_sendCommands("<<p_host<<") responseStr="<<responseStr.c_str();
                                    }
                                    if(toDrone)
                                    {
                                        if(responseStr.find(np_droneSuccessStr1) != std::string::npos || responseStr.find(np_droneSuccessStr2) != std::string::npos)
                                        {
                                            commandSuccess=true;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        if(responseStr.find(np_groundRadioSuccessStr) != std::string::npos)
                                        {
                                            commandSuccess=true;
                                            break;
                                        }
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
                    }
                    commandResponses.push_back(responseStr);

                    if(++commandIndex==p_commands->size() || !commandSuccess)
                    {
                        break;
                    }
                }
            }
        }
        if(commandResponses.empty() && (!p_commands || p_commands->empty()))
        {
            commandResponses.push_back(toDrone?np_droneSuccessStr2:np_groundRadioSuccessStr);
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
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendCommands(p_host="<<p_host<<", p_username="<<p_username<<", toDrone="<<toDrone<<")";
    return std::make_pair(commandsSent,commandResponses);
}

void _waitForDroneResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>>& droneResponses, std::set<int>& succeededDrones, std::set<int>& failedDrones)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _waitForDroneResponses()";
    for(size_t i=0;i<droneResponses.size();++i)
    {
        auto id=droneResponses[i].first;
        auto const& response=droneResponses[i].second.get().second;
        if(response.empty() || (response.back().find(np_droneSuccessStr1)== std::string::npos && response.back().find(np_droneSuccessStr2)== std::string::npos))
        {
            failedDrones.insert(id);
        }
        else
        {
            succeededDrones.insert(id);
        }
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : _waitForDroneResponses()";
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneFrequencyChangeCommands(std::string const& desiredFrequency, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency.c_str()<<")";
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
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency.c_str()<<")";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneEncryptionKeyChangeCommands(std::string const& desiredEncryptionKey, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneEncryptionKeyChangeCommands()";
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
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneEncryptionKeyChangeCommands()";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDroneTxPowerChangeCommands(std::string const& desiredTxPower, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneTxPowerChangeCommands(desiredTxPower="<<desiredTxPower.c_str()<<")";
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
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneTxPowerChangeCommands(desiredTxPower="<<desiredTxPower.c_str()<<")";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> _sendDronePingCommands(std::set<int> const& inputSet, int numSeconds=30)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDronePingCommands()";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses;
    for(auto itr = std::begin(inputSet);;)
    {
        if(itr==std::end(inputSet))
        {
            break;
        }
        auto id = *itr;
        ++itr;
        pingDroneResponses.push_back(std::make_pair(id,std::async(std::launch::async,[id, numSeconds](){
                                                        auto ip=_getDroneIPAddress(id);
                                                        std::this_thread::sleep_for(std::chrono::seconds(2));
                                                        std::vector<std::string> pingCommand;
                                                        pingCommand.push_back("microhard --action=info --monark_id="+std::to_string(id)+"\n");
                                                        auto const startTime = std::chrono::system_clock::now();
                                                        std::pair<bool,std::vector<std::string>> response;
                                                        for(;;)
                                                        {
                                                            response = _sendCommands(ip.c_str(),"monark", "monark", &pingCommand, true);
                                                            //for(auto responseStr: response.second)
                                                            //{
                                                            //    qCDebug(MonarkManagerLog)<<"returnStr = '"<<responseStr.c_str()<<"'";
                                                            //}
                                                            if(!response.second.empty() && (response.second.back().find(np_droneSuccessStr1)!= std::string::npos || response.second.back().find(np_droneSuccessStr2)!= std::string::npos)
                                                                )
                                                            {
                                                                break;
                                                            }
                                                            if((std::chrono::system_clock::now()-startTime) > std::chrono::seconds(numSeconds))
                                                            {
                                                                break;
                                                            }
                                                        }
                                                        return response;
                                                    })));
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendDronePingCommands()";
    return pingDroneResponses;
}

bool _shouldUpdate(QString const& currentVersionString, QString const& newVersionString)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _shouldUpdate(currentVersionString="<<currentVersionString<<", newVersionString="<<newVersionString<<")";
    QRegularExpression regExp("(\\d+)\\.(\\d+)\\.(\\d+)");
    auto const currentMatch = regExp.match(currentVersionString);
    auto const stableMatch = regExp.match(newVersionString);
    bool shouldUpdate=false;
    if (   currentMatch.hasMatch() && currentMatch.lastCapturedIndex() == 3
        && stableMatch.hasMatch()  && stableMatch.lastCapturedIndex()  == 3) {
        auto const currentMajorVersion = currentMatch.captured(1).toInt();
        auto const currentMinorVersion = currentMatch.captured(2).toInt();
        auto const currentBuildVersion = currentMatch.captured(3).toInt();
        auto const stableMajorVersion = stableMatch.captured(1).toInt();
        auto const stableMinorVersion = stableMatch.captured(2).toInt();
        auto const stableBuildVersion = stableMatch.captured(3).toInt();
        shouldUpdate= (currentMajorVersion < stableMajorVersion || (currentMajorVersion == stableMajorVersion && (currentMinorVersion < stableMinorVersion || (currentMinorVersion == stableMinorVersion && currentBuildVersion < stableBuildVersion))));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : _shouldUpdate(currentVersionString="<<currentVersionString<<", newVersionString="<<newVersionString<<") -> return "<<shouldUpdate;
    return shouldUpdate;
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
    m_taskQueueCondition.wakeAll();
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
    , m_allDrones{}
    , m_beforeUpdateDrones{}
    , m_updateInProgressDrones{}
    , m_updateSuccessfulDrones{}
    , m_updateFailedDrones{}
    , m_groundRadioUpdateState{(int)UpdateState::BeforeUpdate}
    , m_monarkState{(int)MonarkState::BeforeScan}
    , m_monarkStateMut{}
    , m_monarkStateCondition{}
    //, m_newDroneId{0}
    //, m_newSysId{0}
    , m_openPorts{}
    //, mp_echoLinkPort{nullptr}
    , m_newGcsVersion{}
    , m_newGcsDescription{}
    , m_newGcsURL{}
    , m_newGcsReleaseDate{}
    , m_newDroneVersion{}
    , m_newDroneDescription{}
    , m_newDroneURL{}
    , m_newDroneReleaseDate{}
    , m_echoLinkBatteryVoltage{-1}
    , m_echoLinkBatteryVoltageTimer{}
    , m_paired{false}
    , m_droneRadioModels{}
    , m_groundRadioModel{}
    , m_monarkUpdatePushPercent{0}
    , m_monarkUpdatePushError{}
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::MonarkManager()()";
    //connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &MonarkManager::validFrequenciesChanged);
    _echoLinkBatteryVoltageTimerHandler();
    m_echoLinkBatteryVoltageTimer.setSingleShot(false);
    m_echoLinkBatteryVoltageTimer.setInterval(1000*45);
    connect(&m_echoLinkBatteryVoltageTimer, &QTimer::timeout, this, &MonarkManager::_echoLinkBatteryVoltageTimerHandler);
    m_echoLinkBatteryVoltageTimer.start(45*1000);
    mp_slotHandler->start();


    _checkForUpdates();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::MonarkManager()()";
}

void MonarkManager::_setEchoLinkRadioModel()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_setEchoLinkRadioModel()";
    m_groundRadioModel.clear();
    _openSerialConnectionToGcsRadio();
    std::string command="AT+GETRADIOINFO\r\n";
    std::atomic_bool finished=false;
    QString result;
    std::vector<std::future<std::string>> jobs;
    for(auto p_openPort : m_openPorts)
    {
        jobs.push_back(std::async(std::launch::async,[&command, p_openPort,&finished]()-> std::string{
            qCDebug(MonarkManagerLog)<<"Requesting radio info from "<<p_openPort->portName();
            p_openPort->write(command.c_str(),command.size());
            p_openPort->waitForBytesWritten(10000);
            std::stringstream ss;
            auto const start = std::chrono::system_clock::now();
            for(;;)
            {
                if(finished)
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(finished)
                {
                    break;
                }
                if(p_openPort->waitForReadyRead(2000))
                {
                    ss<<QString(p_openPort->readAll()).toStdString();
                    auto data = ss.str();
                    qCDebug(MonarkManagerLog)<<"_setEchoLinkRadioModel data='"<<data.c_str()<<"'";
                    //for(auto c:data)
                    //{
                    //    qCDebug(MonarkManagerLog)<<int(c)<<"="<<c;
                    //}

                    if(data.find("ERROR: Command Not Recognized")!= std::string::npos)
                    {
                        finished=true;
                        //default to the 1624
                        return "pMDDL1624AES256";
                    }
                    else
                    {
                        auto beginIndex=data.find("Image");
                        if(beginIndex!=std::string::npos)
                        {
                            beginIndex=data.find(": ", beginIndex);
                            if(beginIndex!= std::string::npos)
                            {
                                beginIndex+=2;
                                auto endIndex=data.find("\r",beginIndex);
                                if(endIndex!=std::string::npos)
                                {
                                    finished=true;
                                    qCDebug(MonarkManagerLog)<<"Got model from "<<p_openPort->portName();
                                    return data.substr(beginIndex,endIndex-beginIndex);
                                }
                            }


                            auto endIndex=data.find("\r",beginIndex);
                            if(endIndex!=std::string::npos)
                            {
                                finished=true;
                                qCDebug(MonarkManagerLog)<<"Got voltage from "<<p_openPort->portName();
                                return data.substr(beginIndex,endIndex-beginIndex);
                            }
                        }
                    }


                }
                if(std::chrono::system_clock::now() - start > std::chrono::seconds(5))
                {
                    break;
                }
            }
            return "";
        }));
    }
    for(auto&& job: jobs)
    {
        auto str = job.get();
        if(!str.empty())
        {
            result=QString::fromStdString(str);
        }
    }
    if(result.isEmpty())
    {
        m_groundRadioModel="pMDDL1624AES256";

    }
    else
    {
        m_groundRadioModel=result;
    }

    emit minMaxPowersChanged();
    emit validFrequenciesChanged();

    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_setEchoLinkRadioModel() m_groundRadioModel="<<m_groundRadioModel;
}

void MonarkManager::_echoLinkBatteryVoltageTimerHandler()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_echoLinkBatteryVoltageTimerHandler()";
    _openSerialConnectionToGcsRadio();
    std::string command="AT+GETVOLTAGE\r\n";
    std::atomic_bool finished=false;
    QString result;
    std::vector<std::future<std::string>> jobs;
    for(auto p_openPort : m_openPorts)
    {
        jobs.push_back(std::async(std::launch::async,[&command, p_openPort,&finished]()-> std::string{
            qCDebug(MonarkManagerLog)<<"Requesting voltage from "<<p_openPort->portName();
            p_openPort->write(command.c_str(),command.size());
            p_openPort->waitForBytesWritten(10000);
            std::stringstream ss;
            auto const start = std::chrono::system_clock::now();
            for(;;)
            {
                if(finished)
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(finished)
                {
                    break;
                }
                if(p_openPort->waitForReadyRead(2000))
                {
                    ss<<QString(p_openPort->readAll()).toStdString();
                    auto data = ss.str();
                    //qCDebug(MonarkManagerLog)<<"data='"<<data.c_str()<<"'";
                    //for(auto c:data)
                    //{
                    //    qCDebug(MonarkManagerLog)<<int(c)<<"="<<c;
                    //}

                    if(data.find("ERROR: Command Not Recognized")!= std::string::npos)
                    {
                        finished=true;
                        return "";
                    }
                    else
                    {
                        auto beginIndex=data.find("\n");
                        if(beginIndex!=std::string::npos)
                        {
                            ++beginIndex;
                            auto endIndex=data.find("\r",beginIndex);
                            if(endIndex!=std::string::npos)
                            {
                                finished=true;
                                qCDebug(MonarkManagerLog)<<"Got voltage from "<<p_openPort->portName();
                                return data.substr(beginIndex,endIndex-beginIndex);
                            }
                        }
                    }


                }
                if(std::chrono::system_clock::now() - start > std::chrono::seconds(5))
                {
                    break;
                }
            }
            return "";
        }));
    }

    for(auto&& job: jobs)
    {
        auto str = job.get();
        if(!str.empty())
        {
            result=QString::fromStdString(str);
        }
    }


    if(finished)
    {
        if(!result.isEmpty())
        {
            bool okay=false;
            float newVoltage=result.toFloat(&okay);
            if(okay && newVoltage == newVoltage)
            {
                if(newVoltage != m_echoLinkBatteryVoltage)
                {
                    m_echoLinkBatteryVoltage=newVoltage;
                    emit echoLinkBatteryVoltageChanged();
                }
            }
            else if(m_echoLinkBatteryVoltage!=-1)
            {
                m_echoLinkBatteryVoltage=-1;
                emit echoLinkBatteryVoltageChanged();
            }
        }
        else if(m_echoLinkBatteryVoltage!=-1)
        {
            m_echoLinkBatteryVoltage=-1;
            emit echoLinkBatteryVoltageChanged();
        }
    }
    else
    {
        //this probably indicates that the battery is dead, since we didn't even get back "Command not recognized"
        if(0 != m_echoLinkBatteryVoltage)
        {
            m_echoLinkBatteryVoltage=0;
            emit echoLinkBatteryVoltageChanged();
        }
    }


    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_echoLinkBatteryVoltageTimerHandler()";
}

void MonarkManager::_checkForUpdates()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_checkForUpdates()";
    QString const versionCheckJSONFileURL = "https://echomav.com/versioning/monark.json";
    QGCFileDownload* p_download = new QGCFileDownload(this);
    connect(p_download, &QGCFileDownload::downloadComplete, this, &MonarkManager::_gcsVersionCheck);
    p_download->download(versionCheckJSONFileURL);
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_checkForUpdates()";
}

void MonarkManager::_renameFirmwareFile(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_renameFirmwareFile(localFile="<<localFile<<", errorMsg="<<errorMsg<<")";
    if(errorMsg.isEmpty())
    {
        auto const index=m_newDroneURL.lastIndexOf("/");
        if(index>=0)
        {
            QFile firmwareFile = QDir(qgcApp()->toolbox()->settingsManager()->appSettings()->firmwareSavePath()).absoluteFilePath(m_newDroneURL.mid(index+1));
            if(firmwareFile.exists())
            {
                firmwareFile.remove();
            }
            QFile::rename(localFile,QFileInfo(firmwareFile).absoluteFilePath());
        }
    }
    else
    {
        qCCritical(MonarkManagerLog) << "Download firmware file failed: " << errorMsg;
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_renameFirmwareFile(localFile="<<localFile<<", errorMsg="<<errorMsg<<")";
}

void MonarkManager::_gcsVersionCheck(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_gcsVersionCheck(localFile="<<localFile<<", errorMsg="<<errorMsg<<")";
    if(errorMsg.isEmpty())
    {
        QFile versionFile(localFile);
        if (versionFile.open(QIODevice::ReadOnly)) {
            QTextStream textStream(&versionFile);
            auto const updateFileJSON=textStream.readAll();
            auto const jsonResponse = QJsonDocument::fromJson(updateFileJSON.toUtf8());
            auto const documentObject=jsonResponse.object();
            auto const monarkGcsObject=documentObject["monark_gcs"].toObject();
            m_newGcsVersion = monarkGcsObject["version"].toString();
            emit newGcsVersionChanged();
            m_newGcsDescription = monarkGcsObject["description"].toString();
            emit newGcsDescriptionChanged();
            m_newGcsURL = monarkGcsObject["url"].toString();
            emit newGcsURLChanged();
            m_newGcsReleaseDate = monarkGcsObject["release_date"].toString();
            emit newGcsReleaseDateChanged();
            auto const monarkCpuObject=documentObject["monark_firmware"].toObject();
            m_newDroneVersion = monarkCpuObject["version"].toString();
            emit newDroneVersionChanged();
            m_newDroneDescription = monarkCpuObject["description"].toString();
            emit newDroneDescriptionChanged();
            m_newDroneURL = monarkCpuObject["url"].toString();
            emit newDroneURLChanged();
            m_newDroneReleaseDate = monarkCpuObject["release_date"].toString();
            emit newDroneReleaseDateChanged();
            auto const currentVersionString = QGCApplication::applicationVersion();
            if(_shouldUpdate(currentVersionString,m_newGcsVersion))
            {
                emit displayGcsUpdateMessage();

            }
            {
                auto const index=m_newDroneURL.lastIndexOf("/");
                if(index>=0)
                {
                    QFile firmwareFile = QDir(qgcApp()->toolbox()->settingsManager()->appSettings()->firmwareSavePath()).absoluteFilePath(m_newDroneURL.mid(index+1));
                    if(firmwareFile.exists())
                    {
                        firmwareFile.remove();
                    }

                    qCDebug(MonarkManagerLog)<<"starting firmware download from "<<m_newDroneURL;
                    QGCFileDownload* p_download = new QGCFileDownload(this);
                    connect(p_download, &QGCFileDownload::downloadComplete, this, &MonarkManager::_renameFirmwareFile);
                    p_download->download(m_newDroneURL);
                }
            }
        }
    }
    else
    {
        qCCritical(MonarkManagerLog) << "Download update check file failed: " << errorMsg;
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_gcsVersionCheck(localFile="<<localFile<<", errorMsg="<<errorMsg<<")";
}

int MonarkManager::minimumPower() const
{
    int minPower=10;
    auto groundPowerItr=n_validMicrohardPowers.find(m_groundRadioModel);
    if(groundPowerItr!=std::end(n_validMicrohardPowers))
    {
        minPower=groundPowerItr->second.first;
    }

    for(auto radioModel: m_droneRadioModels)
    {
        if(auto dronePowersItr=n_validMicrohardPowers.find(radioModel.second); dronePowersItr!= std::end(n_validMicrohardPowers))
        {
            minPower=std::max(dronePowersItr->second.first,minPower);
        }
    }
    return minPower;
}
int MonarkManager::maximumPower() const
{
    int maxPower=30;
    auto groundPowerItr=n_validMicrohardPowers.find(m_groundRadioModel);
    if(groundPowerItr!=std::end(n_validMicrohardPowers))
    {
        maxPower=groundPowerItr->second.second;
    }
    for(auto radioModel: m_droneRadioModels)
    {
        if(auto dronePowersItr=n_validMicrohardPowers.find(radioModel.second); dronePowersItr!= std::end(n_validMicrohardPowers))
        {
            maxPower=std::min(dronePowersItr->second.second,maxPower);
        }
    }
    return maxPower;
}

QStringList MonarkManager::validFrequencies       () const
{
    std::set<QString> intersection;
    {
        auto groundFreqItr=n_validMicrohardFrequencies.find(m_groundRadioModel);
        if(groundFreqItr==std::end(n_validMicrohardFrequencies))
        {
            intersection=n_defaultValidEchoLinkFreqs;
        }
        else
        {
            intersection=groundFreqItr->second;
        }
    }

    for(auto radioModel: m_droneRadioModels)
    {
        if(auto droneFreqsItr=n_validMicrohardFrequencies.find(radioModel.second); droneFreqsItr!= std::end(n_validMicrohardFrequencies))
        {
            auto const& droneFreqs=droneFreqsItr->second;
            std::set<QString> tmpIntersection;
            std::set_intersection(std::begin(intersection), std::end(intersection), std::begin(droneFreqs), std::end(droneFreqs), std::inserter(tmpIntersection,std::end(tmpIntersection)));
            intersection=std::move(tmpIntersection);
        }
    }
    QStringList returnValue;
    for(auto str: intersection)
    {
        returnValue.append(str);
    }
    return returnValue;
}

void MonarkManager::_openSerialConnectionToGcsRadio()
{
    for (auto const& port : QSerialPortInfo::availablePorts())
    {
        m_openPorts.emplace_back(new QSerialPort(this));
        auto& serialPort=*m_openPorts.back();
        serialPort.setPort(port);
        if (serialPort.open(QIODevice::ReadWrite))
        {
            qCDebug(MonarkManagerLog) << "Active Port:" << serialPort.portName();
            if(!serialPort.setBaudRate(QSerialPort::Baud115200))
            {
                qCWarning(MonarkManagerLog) << "Failed to set baud rate";
            }
            if(!serialPort.setDataBits(QSerialPort::Data8))
            {
                qCWarning(MonarkManagerLog) << "Failed to set data bits";
            }
            if(!serialPort.setParity(QSerialPort::NoParity))
            {
                qCWarning(MonarkManagerLog) << "Failed to set parity";
            }
            if(!serialPort.setStopBits(QSerialPort::OneStop))
            {
                qCWarning(MonarkManagerLog) << "Failed to set stop bits";
            }
            if(!serialPort.setFlowControl(QSerialPort::NoFlowControl))
            {
                qCWarning(MonarkManagerLog) << "Failed to set flow control";
            }
        }
        else
        {
            m_openPorts.pop_back();
            qCDebug(MonarkManagerLog) << "Port" << serialPort.portName() << "is not active.";
        }
    }
}

bool MonarkManager::_checkGoodSerialConnection()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_checkGoodSerialConnection()";
    bool result=false;
    _openSerialConnectionToGcsRadio();
    std::string command="AT\r\n";
    std::atomic_bool finished=false;

    std::vector<std::future<bool>> jobs;
    for(auto p_openPort : m_openPorts)
    {
        jobs.push_back(std::async(std::launch::async,[&command, p_openPort,&finished]()-> bool{
            qCDebug(MonarkManagerLog)<<"checking serial connection from "<<p_openPort->portName();
            p_openPort->write(command.c_str(),command.size());
            p_openPort->waitForBytesWritten(10000);
            std::stringstream ss;
            auto const start = std::chrono::system_clock::now();
            for(;;)
            {
                if(finished)
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(finished)
                {
                    break;
                }
                if(p_openPort->waitForReadyRead(2000))
                {
                    ss<<QString(p_openPort->readAll()).toStdString();
                    auto data = ss.str();
                    qCDebug(MonarkManagerLog)<<"_checkGoodSerialConnection data="<<data.c_str();
                    auto beginIndex=data.find("OK");
                    if(beginIndex!=std::string::npos)
                    {
                        return true;
                    }
                }
                if(std::chrono::system_clock::now() - start > std::chrono::seconds(5))
                {
                    break;
                }
            }
            return false;
        }));
    }

    for(auto&& job: jobs)
    {
        auto jobResult = job.get();
        if(jobResult)
        {
            result=jobResult;
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_checkGoodSerialConnection() result="<<result;
    return result;
}

std::string MonarkManager::_getEncryptionKeyFromGcsRadio()
{
    std::string result;
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_getEncryptionKeyFromGcsRadio()";
    _openSerialConnectionToGcsRadio();
    std::string command="AT+GETENCRYPTION\r\n";
    std::atomic_bool finished=false;

    std::vector<std::future<std::string>> jobs;

    for(auto p_openPort : m_openPorts)
    {
        jobs.push_back(std::async(std::launch::async,[&command, p_openPort,&finished]()-> std::string{
            qCDebug(MonarkManagerLog)<<"Requesting password from "<<p_openPort->portName();
            p_openPort->write(command.c_str(),command.size());
            p_openPort->waitForBytesWritten(10000);
            std::stringstream ss;
            auto const start = std::chrono::system_clock::now();
            for(;;)
            {
                if(finished)
                {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(finished)
                {
                    break;
                }
                if(p_openPort->waitForReadyRead(2000))
                {
                    ss<<QString(p_openPort->readAll()).toStdString();
                    auto data = ss.str();
                    qCDebug(MonarkManagerLog)<<"_getEncryptionKeyFromGcsRadio data="<<data.c_str();
                    auto beginIndex=data.find("Password: ");
                    if(beginIndex!=std::string::npos)
                    {
                        beginIndex+=10;
                        auto endIndex=data.find("\r\r\n",beginIndex);
                        if(endIndex!=std::string::npos)
                        {
                            finished=true;
                            qCDebug(MonarkManagerLog)<<"Got password from "<<p_openPort->portName();
                            return data.substr(beginIndex,endIndex-beginIndex);
                        }
                    }
                }
                if(std::chrono::system_clock::now() - start > std::chrono::seconds(5))
                {
                    break;
                }
            }
            return "";
        }));
    }

    for(auto&& job: jobs)
    {
        auto str = job.get();
        if(!str.empty())
        {
            result=str;
        }
    }

    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_getEncryptionKeyFromGcsRadio()";
    return result;
}

bool MonarkManager::_changeGroundRadioEncryptionKey(std::string const& desiredKey)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioEncryptionKey()";

#if 1
    auto const oldState=m_groundRadioUpdateState;
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    switch(oldState)
    {
    case (int)UpdateState::BeforeUpdate:
        emit beforeUpdateDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateFailed:
        emit updateFailedDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateSuccessful:
        emit updateSuccessfulDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    default:
        break;
    }


    _openSerialConnectionToGcsRadio();
    std::atomic_bool finished=false;
    bool result;
    std::vector<std::future<bool>> jobs;
    for(auto p_openPort : m_openPorts)
    {
        jobs.push_back(std::async(std::launch::async,[&desiredKey,p_openPort,&finished]()-> bool{
            std::vector<std::string> groundRadioCommands;
            groundRadioCommands.emplace_back("AT+SETENCRYPTION="+desiredKey+"\r\n");
            groundRadioCommands.emplace_back("AT+SETNEWPASSWORD="+desiredKey+"\r\n");
            qCDebug(MonarkManagerLog)<<"Sending password to "<<p_openPort->portName();

            int commandSuccess=0;
            for(auto const& command: groundRadioCommands)
            {
                p_openPort->write(command.c_str(),command.size());
                p_openPort->waitForBytesWritten(10000);
                std::stringstream ss;
                auto const start = std::chrono::system_clock::now();
                for(;;)
                {
                    if(finished)
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    if(finished)
                    {
                        break;
                    }
                    if(p_openPort->waitForReadyRead(2000))
                    {
                        ss<<QString(p_openPort->readAll()).toStdString();
                        auto data = ss.str();
                        qCDebug(MonarkManagerLog)<<"port='"<<p_openPort->portName()<<"' command='"<<command.c_str()<<"' data='"<<data.c_str()<<"'";

                        if(data.find("OK")!=std::string::npos)
                        {
                            ++commandSuccess;
                            break;
                        }
                    }
                    if(std::chrono::system_clock::now() - start > std::chrono::seconds(5))
                    {
                        break;
                    }
                }
            }
            if(commandSuccess>0)
            {
                finished=true;
            }
            return commandSuccess>=2;

        }));
    }
    for(auto&& job: jobs)
    {
        auto jobResult = job.get();
        if(jobResult)
        {
            result=true;
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT: MonarkManager::_changeGroundRadioEncryptionKey() result="<<result;

    return result;

#else
    //TODO send these commands over a serial connection
    //TODO AT+SETENCRYPTION=password
    //TODO AT+SETNEWPASSWORD=password
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioEncryptionKey(reversion="<<reversion<<")";
    auto const oldState=m_groundRadioUpdateState;
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    switch(oldState)
    {
    case (int)UpdateState::BeforeUpdate:
        emit beforeUpdateDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateFailed:
        emit updateFailedDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateSuccessful:
        emit updateSuccessfulDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    default:
        break;
    }
    auto const oldKey=reversion?desiredKey:currentEncryptionKey;
    auto const newKey=reversion?currentEncryptionKey:desiredKey;

    std::vector<std::string> groundRadioCommands;
    groundRadioCommands.emplace_back("AT+MWVENCRYPT=2,"+newKey+"\n");
    groundRadioCommands.emplace_back("AT+MSPWD="+newKey+","+newKey+"\n");
    groundRadioCommands.emplace_back("AT&W\n");
    auto const& response = _sendCommands(np_srmPairedIp,"admin", oldKey.c_str(), &groundRadioCommands, false);
    if(response.second.empty() || response.second.back().find(np_groundRadioSuccessStr)== std::string::npos)
    {
        m_groundRadioUpdateState=reversion?(int) UpdateState::UpdateSuccessful:(int) UpdateState::UpdateFailed;
    }
    else
    {
        m_groundRadioUpdateState=reversion?(int)UpdateState::UpdateFailed:(int) UpdateState::UpdateSuccessful;
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    if(m_groundRadioUpdateState==(int)UpdateState::UpdateFailed)
    {
        emit updateFailedDronesChanged();
    }
    else
    {
        emit updateSuccessfulDronesChanged();
    }
    emit updateInProgressDronesChanged();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioEncryptionKey(reversion="<<reversion<<")";
    return response.first;
#endif
}

/*
void MonarkManager::_sendEncryptionKeyToGcsRadio(char const*const p_password)
{
    //TODO get rid of this
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_sendEncryptionKeyToGcsRadio()";
    _openSerialConnectionToGcsRadio();
    std::string command="AT+SETADMIN="+std::string{p_password}+"\r";

    for(auto p_openPort: m_openPorts)
    {
        auto const numBytesWritten=p_openPort->write(command.c_str(),command.size());
        p_openPort->flush();
        while(p_openPort->bytesToWrite()>0)
        {
            p_openPort->waitForBytesWritten(100);
        }
        qCDebug(MonarkManagerLog) << "Wrote "<<numBytesWritten<<" bytes to port "<<p_openPort->portName();
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_sendEncryptionKeyToGcsRadio()";
}
*/

void MonarkManager::_setMonarkState(MonarkState monarkState)
{
    bool changed=false;
    {
        std::lock_guard<std::mutex> lock(m_monarkStateMut);
        if((int)monarkState!=m_monarkState)
        {
            changed=true;
            m_monarkState=(int)monarkState;
        }
    }
    if(changed)
    {
        m_monarkStateCondition.notify_all();
        emit monarkStateChanged(m_monarkState);
    }
}

MonarkManager::~MonarkManager() {
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::~MonarkManager()()";
    mp_slotHandler->shutdown();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::~MonarkManager()()";
}

void MonarkManager::setToolbox(QGCToolbox *const p_toolbox)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::setToolbox()";
    QGCTool::setToolbox(p_toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<MonarkManager> ("QGroundControl.MonarkManager", 1, 0, "MonarkManager", "Reference only");
    mp_monarkSettings = p_toolbox->settingsManager()->monarkSettings();
    mp_monarkQRCodeProvider = p_toolbox->monarkQRCodeProvider();
    startScanning();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::setToolbox()";
}

QString MonarkManager::allDrones() const
{
    if(m_allDrones.empty())
    {
        return "None";
    }
    else
    {
        return _convertSetToString(m_allDrones,false);
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
        auto password = _getEncryptionKeyFromGcsRadio();

        bool sendPassword=false;
        if(password.empty())
        {
            sendPassword=true;
            password = this->mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        }
        else
        {
            this->mp_monarkSettings->encryptionKey()->setCookedValue(QString::fromStdString(password));
            mp_monarkSettings->onSaveSettings();
        }
        auto pingPairedResponseFuture = std::async(std::launch::async,[&password](){
            return _sendCommands(np_srmPairedIp, "admin", password.c_str(), nullptr, false);
        });
        _setEchoLinkRadioModel();

        auto const pingDefaultResponse=_sendCommands(np_srmDefaultIp, "admin", nullptr, nullptr, false).second;
        if(!pingDefaultResponse.empty() && pingDefaultResponse.back() == np_groundRadioSuccessStr)
        {
            qCDebug(MonarkManagerLog)<<"ScanSuccessPairingRequired";
            scanningResult=MonarkState::ScanSuccessPairingRequired;
            this->mp_monarkSettings->encryptionKey()->setCookedValue("");
            this->mp_monarkSettings->groundFrequency()->setCookedValue(n_defaultGroundFrequency);
            this->mp_monarkSettings->groundTxPower()->setCookedValue(n_defaultGroundTxPower);
            m_paired=false;
            _initializeNetworkId();
            _initializeTxPower();
            _initializeFrequency();
        }
        else
        {
            auto pingPairedResponse = pingPairedResponseFuture.get().second;
            if(!pingPairedResponse.empty() && pingPairedResponse.back() ==np_groundRadioSuccessStr)
            {
                m_droneRadioModels.clear();
                m_allDrones.clear();
                m_beforeUpdateDrones.clear();
                m_updateInProgressDrones.clear();
                m_updateSuccessfulDrones.clear();
                m_updateFailedDrones.clear();
                std::vector<std::future<std::vector<std::string>>> dronePingResponses;


                for(auto i=1;i<=n_maxMonarkID;++i)
                {
                    dronePingResponses.push_back(std::async(std::launch::async,[i](){
                        //auto const ip = "172.20.2."+std::to_string(i);
                        //return _sendCommands(ip.c_str(), "admin", nullptr, nullptr, true).second;

                        std::vector<std::string> dronePingCommands;
                        dronePingCommands.push_back("microhard --action=info --monark_id="+std::to_string(i)+"\n");
                        auto const ip = _getDroneIPAddress(i);
                        return _sendCommands(ip.c_str(), "monark", "monark", &dronePingCommands, true).second;
                    }));
                }

                qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                scanningResult=MonarkState::ScanSuccessAndPaired;
                if(sendPassword)
                {
                    _changeGroundRadioEncryptionKey(password);
                }
                m_paired=true;
                _initializeNetworkId();
                _initializeTxPower();
                _initializeFrequency();
                for(auto i=1;i<=n_maxMonarkID;++i)
                {
                    auto response = dronePingResponses[i-1].get();
                    //qCDebug(MonarkManagerLog)<<"START drone response from ID "<<(i);
                    //for(auto str: response)
                    //{
                    //    qCDebug(MonarkManagerLog)<<"'"<<str.c_str()<<"'";
                    //}
                    //qCDebug(MonarkManagerLog)<<"END   drone response from ID "<<(i);
                    if(!response.empty() && _parseInfoJsonResponse(QString::fromStdString(response.back())))
                    {
                        m_beforeUpdateDrones.insert(i);
                        m_allDrones.insert(i);
                    }
                }
                emit allDronesChanged();
                emit beforeUpdateDronesChanged();
                emit updateInProgressDronesChanged();
                emit updateSuccessfulDronesChanged();
                emit updateFailedDronesChanged();
                emit validFrequenciesChanged();
                emit minMaxPowersChanged();
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

void MonarkManager::refreshDroneList()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){refreshDroneList();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::refreshDroneList()";
        auto oldAllDrones=m_allDrones;
        m_allDrones.clear();
        m_droneRadioModels.clear();
        std::vector<std::future<std::vector<std::string>>> dronePingResponses;
        for(auto i=1;i<=n_maxMonarkID;++i)
        {
            dronePingResponses.push_back(std::async(std::launch::async,[i](){
                //auto const ip = "172.20.2."+std::to_string(i);
                //return _sendCommands(ip.c_str(), "admin", nullptr, nullptr, true).second;

                std::vector<std::string> dronePingCommands;
                dronePingCommands.push_back("microhard --action=info --monark_id="+std::to_string(i)+"\n");
                auto const ip = _getDroneIPAddress(i);
                return _sendCommands(ip.c_str(), "monark", "monark", &dronePingCommands, true).second;
            }));
        }

        for(auto i=1;i<=n_maxMonarkID;++i)
        {
            auto response = dronePingResponses[i-1].get();
            //qCDebug(MonarkManagerLog)<<"START drone response from ID "<<(i);
            //for(auto str: response)
            //{
            //    qCDebug(MonarkManagerLog)<<"'"<<str.c_str()<<"'";
            //}
            //qCDebug(MonarkManagerLog)<<"END   drone response from ID "<<(i);

            if(!response.empty() &&_parseInfoJsonResponse(QString::fromStdString(response.back())))
            {


                if(m_allDrones.insert(i).second)
                {
                    emit allDronesChanged();
                }
            }
        }
        emit allDronesChanged();
        emit validFrequenciesChanged();
        emit minMaxPowersChanged();


        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::refreshDroneList()";
    }
}

bool MonarkManager::_parseInfoJsonResponse(QString json)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_parseInfoJsonResponse()";
    //auto firstIndex=json.indexOf("{");
    //auto lastIndex=json.lastIndexOf("}");
    //if(firstIndex>=0 && lastIndex>firstIndex)
    //{
    //    json=json.mid(firstIndex,lastIndex-firstIndex);
    //}

    //json.replace("'", "\"");


    qCDebug(MonarkManagerLog)<<"json="<<json;


    QJsonParseError error;
    auto const document = QJsonDocument::fromJson(json.toUtf8(), &error);
    bool goodResponse=false;
    if(document.isNull())
    {
        qCCritical(MonarkManagerLog)<<"Unable to parse JSON: "<<error.errorString()<<", offset="<<error.offset;
        //it's probably an older version that isn't putting out up-to-spec JSON.
        goodResponse=json.contains(np_droneSuccessStr1);
    }
    else if(document["is_success"].toBool(false))
    {
        goodResponse=true;
        if(auto const message = document["message"]; message.isObject())
        {
            if( auto const monarkIDJson = message["monark_id"]; monarkIDJson.isDouble())
            {
                if(auto const monarkId=monarkIDJson.toInt(0);monarkId>0)
                {
                    if(auto const droneRadioModelJson=message["product"]; droneRadioModelJson.isString())
                    {
                        m_droneRadioModels[monarkId] = droneRadioModelJson.toString();

                    }
                }
            }
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_parseInfoJsonResponse()";
    return goodResponse;
}

void MonarkManager::removeDrone(int monarkID)
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this,monarkID](){removeDrone(monarkID);});
    //}
    //else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::removeDrone(monarkID="<<monarkID<<")";
        m_droneRadioModels.erase(monarkID);
        emit validFrequenciesChanged();
        emit minMaxPowersChanged();
        m_allDrones.erase(monarkID);
        emit allDronesChanged();
        m_beforeUpdateDrones.erase(monarkID);
        emit beforeUpdateDronesChanged();
        m_updateInProgressDrones.erase(monarkID);
        emit updateInProgressDronesChanged();
        m_updateSuccessfulDrones.erase(monarkID);
        emit updateSuccessfulDronesChanged();
        m_updateFailedDrones.erase(monarkID);
        emit updateFailedDronesChanged();
        //if(monarkID==m_newDroneId)
        //{
        //    m_newDroneId=0;
        //    emit newDroneIdChanged();
        //}
        /*
        if(monarkID==m_newSysId)
        {
            m_newSysId=0;
            emit newSysIdChanged();
        }
        */
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::removeDrone(monarkID="<<monarkID<<")";
    }
}

void MonarkManager::_initializeNetworkId()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeNetworkId(paired="<<m_paired<<")";
    std::string macAddress="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MNEMAC\n");
    auto encryptionKey=m_paired?mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", encryptionKey.c_str(), &commands, false).second;
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
        constexpr size_t const networkIDLength=4;
        if(macAddress.length()>networkIDLength)
        {
            macAddress=macAddress.substr(macAddress.length()-networkIDLength,networkIDLength);
        }
    }
    if(!macAddress.empty())
    {
        this->mp_monarkSettings->networkID()->setCookedValue("MONARK-"+QString::fromStdString(macAddress));
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeNetworkId(paired="<<m_paired<<")";
}

void MonarkManager::_initializeFrequency()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeFrequency(paired="<<paired<<")";
    std::string frequency="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MWFREQ\n");
    auto encryptionKey=m_paired?mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", encryptionKey.c_str(), &commands, false).second;
    for(auto const& responseStr: response)
    {
        qCDebug(MonarkManagerLog)<<"Got response "<<responseStr.c_str();
    }
    if(!response.empty() && response.back().find(np_groundRadioSuccessStr) != std::string::npos)
    {
        auto freqStart = response.back().find_last_of(':');
        if(freqStart != std::string::npos)
        {
            freqStart=response.back().find_first_of(' ',freqStart);
            if(freqStart != std::string::npos)
            {
                auto const freqEnd = response.back().find_first_of(' ', freqStart+1);
                if(freqEnd!=std::string::npos)
                {
                    ++freqStart;
                    frequency = response.back().substr(freqStart, freqEnd-freqStart);
                    qCDebug(MonarkManagerLog)<<"frequency= "<<frequency.c_str();
                }
            }
        }
    }
    if(!frequency.empty())
    {
        this->mp_monarkSettings->groundFrequency()->setCookedValue(QString::fromStdString(frequency));
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeFrequency(paired="<<paired<<")";
}

void MonarkManager::_initializeTxPower()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeTxPower(paired="<<paired<<")";
    std::string txPower="";
    std::vector<std::string> commands;
    commands.emplace_back("AT+MWTXPOWER\n");
    auto encryptionKey=m_paired?mp_monarkSettings->encryptionKey()->rawValueString().toStdString(): np_sshUsername;//mp_monarkSettings->getOldEncryptionKey().toStdString():np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", encryptionKey.c_str(), &commands, false).second;
    for(auto const& responseStr: response)
    {
        qCDebug(MonarkManagerLog)<<"Got response "<<responseStr.c_str();
    }
    if(!response.empty() && response.back().find(np_groundRadioSuccessStr) != std::string::npos)
    {
        auto freqStart = response.back().find_last_of(':');
        if(freqStart != std::string::npos)
        {
            freqStart=response.back().find_first_of(' ',freqStart);
            if(freqStart != std::string::npos)
            {
                auto const freqEnd = response.back().find_first_of(' ', freqStart+1);
                if(freqEnd!=std::string::npos)
                {
                    ++freqStart;
                    txPower = response.back().substr(freqStart, freqEnd-freqStart);
                    qCDebug(MonarkManagerLog)<<"txPower= "<<txPower.c_str();
                }
            }
        }
    }
    if(!txPower.empty())
    {
        this->mp_monarkSettings->groundTxPower()->setCookedValue(QString::fromStdString(txPower));
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeTxPower(paired="<<paired<<")";
}

void MonarkManager::gotoBeforePairNewDrone()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoBeforePairNewDrone();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoBeforePairNewDrone()";
        _setMonarkState(MonarkState::BeforePairNewDrone);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoBeforePairNewDrone()";
    }
}

void MonarkManager::_resetToBeforeUpdate()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_resetToBeforeUpdate()";
    for(auto id:m_allDrones)
    {
        m_beforeUpdateDrones.insert(id);
    }
    m_updateInProgressDrones.clear();
    m_updateSuccessfulDrones.clear();
    m_updateFailedDrones.clear();
    m_groundRadioUpdateState=(int)UpdateState::BeforeUpdate;
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit beforeUpdateDronesChanged();
    emit updateInProgressDronesChanged();
    emit updateFailedDronesChanged();
    emit updateSuccessfulDronesChanged();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_resetToBeforeUpdate()";
}

void MonarkManager::gotoChangeEncryptionKey()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoChangeEncryptionKey();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeEncryptionKey()";
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeEncryptionKey);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeEncryptionKey()";
    }
}

void MonarkManager::gotoChangeFrequencies()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoChangeFrequencies();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeFrequencies()";
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeFrequencies);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeFrequencies()";
    }
}

void MonarkManager::gotoChangeTxPower()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoChangeTxPower();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoChangeTxPower()";
        _resetToBeforeUpdate();
        _setMonarkState(MonarkState::ChangeTxPower);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoChangeTxPower()";
    }
}

void MonarkManager::gotoResetUnpairMonark()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoResetUnpairMonark();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoResetUnpairMonark()";
        _setMonarkState(MonarkState::ResetUnpairMonark);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoResetUnpairMonark()";
    }
}

void MonarkManager::gotoScanSuccessAndPaired()
{
    //if(mp_slotHandler->needDispatch())
    //{
    //    mp_slotHandler->dispatch([this](){gotoScanSuccessAndPaired();});
    //}
    //else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoScanSuccessAndPaired()";
        _setMonarkState(MonarkState::ScanSuccessAndPaired);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoScanSuccessAndPaired()";
    }
}

void MonarkManager::gotoDetectionFailed()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::gotoDetectionFailed()";
    _setMonarkState(MonarkState::DetectionFailed);
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::gotoDetectionFailed()";
}


void MonarkManager::tryDroneUpdate(int monarkId)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this,monarkId](){tryDroneUpdate(monarkId);});
    }
    else
    {
        auto const needsUpdate=_checkIfDroneNeedsUpdate(monarkId);
        if(needsUpdate)
        {

            emit displayMonarkUpdateMessage(monarkId);
        }
    }
}
bool MonarkManager::_checkIfDroneNeedsUpdate(int monarkId)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_checkIfDroneNeedsUpdate(monarkId="<<monarkId<<")";
    auto const p_vehicles=qgcApp()->toolbox()->multiVehicleManager()->vehicles();
    auto const numVehicles=p_vehicles->count();
    for(int i=0;i<numVehicles;++i)
    {
        if(((Vehicle*)p_vehicles->get(i))->armed())
        {
            return false;
        }
    }
    bool needsUpdate=false;
    if(monarkId>0)
    {
        std::vector<std::string> commands;
        commands.push_back("monark-updater --version\n");
        std::string ip = _getDroneIPAddress(monarkId);

        auto const& response = _sendCommands(ip.c_str(),"monark", "monark", &commands, true,false).second;
        if(!response.empty())
        {
            auto const currentVersionString = response.back();
            needsUpdate= _shouldUpdate(QString::fromStdString(currentVersionString),m_newDroneVersion);
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_checkIfDroneNeedsUpdate(monarkId="<<monarkId<<") -> return "<<needsUpdate;
    return needsUpdate;
}

void MonarkManager::pushMonarkDownload(int monarkID)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this,monarkID](){pushMonarkDownload(monarkID);});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::pushMonarkDownload(monarkID="<<monarkID<<")";
        auto const index=m_newDroneURL.lastIndexOf("/");
        qCDebug(MonarkManagerLog)<<"m_newDroneURL: "<<m_newDroneURL;

        if(index>=0)
        {
            QFile firmwareFile = QDir(qgcApp()->toolbox()->settingsManager()->appSettings()->firmwareSavePath()).absoluteFilePath(m_newDroneURL.mid(index+1));
            if(firmwareFile.exists())
            {
                m_monarkUpdatePushError.clear();
                emit monarkUpdatePushErrorChanged();
                std::string ip = _getDroneIPAddress(monarkID);
                _sendFile(ip.c_str(),"monark", "monark",firmwareFile,"/home/monark/monark-updates.zip",true,false,[this](int percent,QString const& errorMessage){
                    qCCritical(MonarkManagerLog)<<"Monark update progress: "<<percent<<"%";
                    if(percent<0)
                    {
                        qCCritical(MonarkManagerLog)<<"Update Push Failed. Error message: "<<errorMessage;
                        m_monarkUpdatePushError=errorMessage;
                        emit monarkUpdatePushErrorChanged();
                    }
                    else
                    {
                        m_monarkUpdatePushPercent=percent;
                        emit monarkUpdatePushPercentChanged();
                    }

                });
                std::vector<std::string> droneCommands;
                droneCommands.push_back("microhard --action=reboot_rpi\n");
                _sendCommands(ip.c_str(),"monark", "monark", &droneCommands, true,false);
            }
        }
        //auto const p_vehicle = qgcApp()->toolbox()->multiVehicleManager()->getVehicleById(monarkID);
        //if(p_vehicle)
        //{
        //    p_vehicle->rebootVehicle();
        //}
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::pushMonarkDownload(monarkID="<<monarkID<<")";
    }
}
/*
void MonarkManager::showRestartMessage()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::showRestartMessage()";
    //auto const needsUpdate=_checkIfDroneNeedsUpdate(m_newSysId);
    ///if(needsUpdate)
    //{
    //    emit displayMonarkUpdateMessage(m_newSysId);
    //}
    //else
    {
        emit displayRestartMessage();
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::showRestartMessage()";
}


void MonarkManager::restartApplication()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::restartApplication()";
    //wait 5 seconds for the sysid command to process
    std::this_thread::sleep_for(std::chrono::seconds(5));
    //auto *const p_vehicle=qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    //auto *const p_vehicle = qgcApp()->toolbox()->multiVehicleManager()->getVehicleById(m_newSysId);
    //if(p_vehicle)
    //{
    //     p_vehicle->rebootVehicle();
    //}
    //wait 5 seconds for the reboot command to process
    //std::this_thread::sleep_for(std::chrono::seconds(5));

    auto const program = QGCApplication::_app->arguments()[0];
    QStringList arguments = QGCApplication::_app->arguments().mid(1);
    QGCApplication::_app->quit();
    QProcess::startDetached(program, arguments);


    // QAndroidJniObject::callStaticObjectMethod("org/mavlink/qgroundcontrol/QGCActivity", "restartApp",
    //                                                                     "()V;");
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::restartApplication()";

}
*/

void MonarkManager::openGcsDownload()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::openGcsDownload()";
    qCDebug(MonarkManagerLog)<<"opening URL "<<m_newGcsURL;
    QDesktopServices::openUrl(m_newGcsURL);
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::openGcsDownload()";
}

void MonarkManager::saveFlutterManagementSettings(QString const& frequency)
{
    if(mp_slotHandler->needDispatch())
    {
        auto cachedFrequency=frequency;
        mp_slotHandler->dispatch([this,cachedFrequency](){saveFlutterManagementSettings(cachedFrequency);});
    }
    else
    {
        _setMonarkState(MonarkState::SaveSettingsInProgress);
        std::vector<std::string> commands;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();

        bool settingsChanged=false;
        if(encryptionKey.empty())
        {
            encryptionKey=_generateRandomKey();
            settingsChanged=true;

        }
        qCDebug(MonarkManagerLog)<<"password="<<encryptionKey.c_str();


        using namespace std::string_literals;
        auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString().toStdString();
        mp_monarkSettings->groundFrequency()->setCookedValue(frequency);
        auto networkId=mp_monarkSettings->networkID()->cookedValueString().toStdString();
        commands.emplace_back("AT+MWRADIO=1\n");
        commands.emplace_back("AT+MWDISTANCE=8047\n"); //acceptable RF distance 5 miles
        commands.emplace_back("AT+MWTXPOWER="+txPower+"\n");
        commands.emplace_back("AT+MWFREQ="+frequency.toStdString()+"\n");
        commands.emplace_back("AT+MWNETWORKID="+networkId+"\n");
        //commands.emplace_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
        //commands.emplace_back("AT+MSPWD="+encryptionKey+","+encryptionKey+"\n");
        commands.emplace_back("AT+MWVMODE=0\n");
        commands.emplace_back("AT+MNLAN=LAN,EDIT,0,"s+np_srmPairedIp+",255.255.0.0,0\n");
        commands.emplace_back("AT+MNLANDHCP=LAN,1,"s+np_srocIp+",1,0\n");
        commands.emplace_back("AT&W\n");

        auto saveResult=MonarkState::SaveSettingsFailed;
        auto const& response = _sendCommands(np_srmDefaultIp,"admin", np_sshUsername, &commands, false).second;
        if(!response.empty() && response.back().find(np_groundRadioSuccessStr)!= std::string::npos)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            // also send the encryption key to the radio microcontroller over serial so it can reset the microhard natively.
            if(settingsChanged)
            {
                mp_monarkSettings->encryptionKey()->setCookedValue(QString::fromStdString(encryptionKey));
                mp_monarkSettings->onSaveSettings();
            }
            _changeGroundRadioEncryptionKey(encryptionKey);
        }
        _setMonarkState(saveResult);
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::saveFlutterManagementSettings(frequency="<<frequency<<")";
    }
}

void MonarkManager::resetActiveVehicle()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){resetActiveVehicle();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::resetActiveVehicle()";
        auto const activeVehicleId = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->id();
        if(activeVehicleId>0)
        {
            auto const ip=_getDroneIPAddress(activeVehicleId);
            std::vector<std::string> droneCommands;
            droneCommands.push_back("microhard --action=reset --monark_id="+std::to_string(activeVehicleId)+"\n");
            auto const response= _sendCommands(ip.c_str(),"monark", "monark", &droneCommands,true,false).second;
            if(!response.empty() && (response.back().find(np_droneSuccessStr1)!= std::string::npos || response.back().find(np_droneSuccessStr2)!= std::string::npos))
            {
                m_droneRadioModels.erase(activeVehicleId);
                emit validFrequenciesChanged();
                emit minMaxPowersChanged();
                m_allDrones.erase(activeVehicleId);
                emit allDronesChanged();
                m_beforeUpdateDrones.erase(activeVehicleId);
                emit beforeUpdateDronesChanged();
                m_updateInProgressDrones.erase(activeVehicleId);
                emit updateInProgressDronesChanged();
                m_updateSuccessfulDrones.erase(activeVehicleId);
                emit updateSuccessfulDronesChanged();
                m_updateFailedDrones.erase(activeVehicleId);
                emit updateFailedDronesChanged();
                //if(m_newDroneId==activeVehicleId)
                //{
                //    m_newDroneId=0;
                //    emit newDroneIdChanged();
                //}
                /*
                if(m_newSysId==activeVehicleId)
                {
                    m_newSysId=0;
                    emit newSysIdChanged();
                }
                */
            }

        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::resetActiveVehicle()";
    }
}

void MonarkManager::rebootActiveVehicle()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){rebootActiveVehicle();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::rebootActiveVehicle()";
        auto const p_activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        if(p_activeVehicle)
        {
            p_activeVehicle->rebootVehicle();
        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::rebootActiveVehicle()";
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
        auto monarkID = mp_monarkSettings->monarkID()->cookedValue().toUInt();
        /*
        m_newSysId=monarkID;
        emit newSysIdChanged();
        */
        _setMonarkState(MonarkState::ShowQRCode);
        auto detectionResult=MonarkState::DetectionFailed;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
        std::string ip = _getDroneIPAddress(monarkID);
        auto const startTime = std::chrono::system_clock::now();
        std::vector<std::string> commands;
        commands.push_back("microhard --action=info --monark_id="+std::to_string(monarkID)+"\n");
        //commands.push_back("monark-updater --version\n");
        for(;;)
        {
            if((std::chrono::system_clock::now()-startTime) > std::chrono::seconds(210))
            {
                break;
            }
            if(MonarkManagerLog().isDebugEnabled())
            {
                qgcApp()->toolbox()->audioOutput()->say("Beep");
            }
            auto const& response = _sendCommands(ip.c_str(),"monark", "monark", &commands, true, false).second;
            //for(auto const& responseStr: response)
            //{
            //    //TODO remove
            //    qCDebug(MonarkManagerLog)<<"returnStr = '"<<responseStr.c_str()<<"'";
            //}
            if(!response.empty() && _parseInfoJsonResponse(QString::fromStdString(response.back())))
            {
                qCDebug(MonarkManagerLog)<<"found drone";
                m_allDrones.insert(monarkID);
                emit allDronesChanged();
                m_beforeUpdateDrones.insert(monarkID);
                emit beforeUpdateDronesChanged();
                detectionResult=MonarkState::ScanSuccessAndPaired;
                //m_newDroneId=monarkID;
                //emit newDroneIdChanged();
                qgcApp()->showAppMessage(QString("MONARK-")+monarkID+" has been added.");
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
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency.c_str()<<", reversion="<<reversion<<")";
    auto const oldState=m_groundRadioUpdateState;
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    switch(oldState)
    {
    case (int)UpdateState::BeforeUpdate:
        emit beforeUpdateDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateFailed:
        emit updateFailedDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateSuccessful:
        emit updateSuccessfulDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    default:
        break;
    }
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
    if(m_groundRadioUpdateState==(int)UpdateState::UpdateFailed)
    {
        emit updateFailedDronesChanged();
    }
    else
    {
        emit updateSuccessfulDronesChanged();
    }
    emit updateInProgressDronesChanged();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency.c_str()<<", reversion="<<reversion<<")";
    return response.first;
}


bool MonarkManager::_changeGroundRadioTxPower(std::string const& desiredPower)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioTxPower(desiredPower="<<desiredPower.c_str()<<")";
    auto const oldState=m_groundRadioUpdateState;
    m_groundRadioUpdateState=(int) UpdateState::UpdateInProgress;
    switch(oldState)
    {
    case (int)UpdateState::BeforeUpdate:
        emit beforeUpdateDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateFailed:
        emit updateFailedDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    case (int)UpdateState::UpdateSuccessful:
        emit updateSuccessfulDronesChanged();
        emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
        emit updateInProgressDronesChanged();
        break;
    default:
        break;
    }
    auto const currentEncryptionKey= mp_monarkSettings->encryptionKey()->cookedValueString().toStdString();
    std::vector<std::string> groundRadioCommands;
    groundRadioCommands.emplace_back("AT+MWTXPOWER="+desiredPower+"\n");
    groundRadioCommands.emplace_back("AT&W\n");
    auto const& response = _sendCommands(np_srmPairedIp,"admin", currentEncryptionKey.c_str(), &groundRadioCommands, false,false);
    if(response.second.empty() || response.second.back().find(np_groundRadioSuccessStr)== std::string::npos)
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
        emit updateFailedDronesChanged();

    }
    else
    {
        m_groundRadioUpdateState=(int) UpdateState::UpdateSuccessful;
        emit updateSuccessfulDronesChanged();
    }
    emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
    emit updateInProgressDronesChanged();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioTxPower(desiredPower="<<desiredPower.c_str()<<")";
    return response.first;
}

void MonarkManager::_waitForPingResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>>& pingDroneResponses, std::function<void(int)> const& responseGoodFunc, std::function<void(int)> const& responseBadFunc)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_waitForPingResponses()";
    for(size_t i=0;i<pingDroneResponses.size();++i)
    {
        auto id=pingDroneResponses[i].first;
        auto const& response=pingDroneResponses[i].second.get().second;
        m_updateInProgressDrones.erase(id);
        emit updateInProgressDronesChanged();
        if(response.empty() || (response.back().find(np_droneSuccessStr1)== std::string::npos && response.back().find(np_droneSuccessStr2)== std::string::npos))
        {
            responseBadFunc(id);
        }
        else
        {
            responseGoodFunc(id);
        }
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_waitForPingResponses()";
}

void MonarkManager::changeTxPower(QString const& desiredTxPower)
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this, desiredTxPower](){changeTxPower(desiredTxPower);});
    }
    else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower.toStdString().c_str()<<")";
        _resetToBeforeUpdate();
        auto const desiredStdString=desiredTxPower.toStdString();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> droneResponses=_sendDroneTxPowerChangeCommands(desiredStdString,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        _changeGroundRadioTxPower(desiredStdString);
        _waitForPingResponses(droneResponses,
                              //("'is_success': True, 'message': {tx_power': '"+desiredTxPower.toStdString()+"'").c_str(),
                              [this](int id){
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
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower.toStdString().c_str()<<")";
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
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency.toStdString().c_str()<<")";
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
                qCDebug(MonarkManagerLog)<<"EchoLink frequency change successful";
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
                        qCDebug(MonarkManagerLog)<<"All drones failed to change frequencies. Reverting EchoLink";
                        //all of the drones failed to update, so revert the EchoLink
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
                        //revert the EchoLink back
                        if(_changeGroundRadioFrequency(oldFrequency,true))
                        {
                            qCDebug(MonarkManagerLog)<<"EchoLink reverted. Pinging drones";
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
                            qCDebug(MonarkManagerLog)<<"EchoLink reversion failed";
                            //unable to revert the EchoLink back, so it is impossible to ping the drones to check if they changed
                            //instead, set those drones to failed (assumed) but set the EchoLink to a success state
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
                qCDebug(MonarkManagerLog)<<"Commands sent to drones, but EchoLink could not be changed";
                //all the commands were successfully sent, but we can't change the EchoLink frequency
                //therefore, it is impossible to ping the drones for success
                //assume success on the drones, but set the EchoLink state to failure
                m_updateSuccessfulDrones=succeededDrones;
                m_updateInProgressDrones.clear();
                emit updateSuccessfulDronesChanged();
                emit updateInProgressDronesChanged();
            }
        }
        else if(!succeededDrones.empty())
        {
            qCDebug(MonarkManagerLog)<<"Failed to send commands to some drones, but not all. Moving EchoLink to new frequency to begin reversion";
            m_updateFailedDrones=failedDrones;
            failedDrones.clear();
            emit updateFailedDronesChanged();
            //some drones failed to get commands, but some succeeded
            if(_changeGroundRadioFrequency(desiredStdString,false))
            {
                qCDebug(MonarkManagerLog)<<"EchoLink on new frequency. Pinging drones";
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                for(size_t i=0;i<pingDroneResponses.size();++i)
                {
                    auto id=pingDroneResponses[i].first;
                    auto const& response=pingDroneResponses[i].second.get().second;
                    if(response.empty() || (response.back().find(np_droneSuccessStr1)== std::string::npos && response.back().find(np_droneSuccessStr2)== std::string::npos))
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
                    qCDebug(MonarkManagerLog)<<"No drones pinged. Reverting EchoLink";
                    //no drones were successfully changed, so revert the EchoLink and you're done
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
                    //revert the EchoLink back
                    if(_changeGroundRadioFrequency(oldFrequency,true))
                    {
                        qCDebug(MonarkManagerLog)<<"EchoLink reverted. Pinging drones";
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
                        qCDebug(MonarkManagerLog)<<"EchoLink reversion failed.";
                        //unable to revert the EchoLink back, so it is impossible to ping the drones to check if they changed
                        //instead, set those drones to failed (assumed) but set the EchoLink to a success state
                        m_updateFailedDrones.insert(std::begin(failedDrones), std::end(failedDrones));
                        m_updateInProgressDrones.clear();
                        emit updateFailedDronesChanged();
                        emit updateInProgressDronesChanged();
                    }
                }

            }
            else
            {
                qCDebug(MonarkManagerLog)<<"EchoLink could not be changed to new frequency";
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
            //all drones failed to receive the command, so don't bother changing the EchoLink
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
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency.toStdString().c_str()<<")";
    }
}

void MonarkManager::changeEncryptionKey()
{
    if(mp_slotHandler->needDispatch())
    {
        mp_slotHandler->dispatch([this](){changeEncryptionKey();});
    }
    else
    {
        //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeEncryptionKey()";
        _resetToBeforeUpdate();
        if(_checkGoodSerialConnection())
        {
            auto const currentEncryptionKey=            mp_monarkSettings->encryptionKey()->rawValueString().toStdString();
            auto const desiredStdString=_generateRandomKey();
            qCDebug(MonarkManagerLog)<<"current password="<<currentEncryptionKey.c_str();
            qCDebug(MonarkManagerLog)<<"new password="<<desiredStdString.c_str();

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
                    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones,60);
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
                    if(m_updateSuccessfulDrones.empty())
                    {
                        if(m_updateFailedDrones.empty())
                        {
                            qCDebug(MonarkManagerLog)<<"EchoLink succeeded";
                        }
                        else
                        {
                            qCDebug(MonarkManagerLog)<<"All drones failed. Reverting EchoLink";
                            if(_changeGroundRadioEncryptionKey(currentEncryptionKey))
                            {
                                qCDebug(MonarkManagerLog)<<"EchoLink reversion successful";
                            }
                            else
                            {
                                qCDebug(MonarkManagerLog)<<"EchoLink reversion failed";
                            }
                        }
                    }
                    else
                    {
                        if(m_updateFailedDrones.empty())
                        {
                            qCDebug(MonarkManagerLog)<<"All drones and EchoLink succeeded";
                        }
                        else
                        {
                            qCDebug(MonarkManagerLog)<<"Some drones failed. Re-pairing will be required";
                        }
                    }
                }
                else
                {
                    qCDebug(MonarkManagerLog)<<"Commands successfully sent to drones, but EchoLink failed to change key";
                    //we can't change the EchoLink's frequency
                    //therefore, we can't ping the drones to determine success
                    //Assume success in those drones, but set the EchoLink to failure
                    m_updateSuccessfulDrones=succeededDrones;
                    emit updateSuccessfulDronesChanged();
                }
            }
            else if(!succeededDrones.empty())
            {
                qCDebug(MonarkManagerLog)<<"Some drones succeeded, some failed. Setting EchoLink encryption key";
                m_updateFailedDrones.insert(std::begin(failedDrones), std::end(failedDrones));
                emit updateFailedDronesChanged();
                if(_changeGroundRadioEncryptionKey(desiredStdString))
                {
                    qCDebug(MonarkManagerLog)<<"EchoLink changed. Pinging drones";

                    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<std::string>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones, 60);
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
                    if(m_updateSuccessfulDrones.empty())
                    {
                        if(m_updateFailedDrones.empty())
                        {
                            qCDebug(MonarkManagerLog)<<"EchoLink succeeded";
                        }
                        else
                        {
                            qCDebug(MonarkManagerLog)<<"All drones failed. Reverting EchoLink";
                            if(_changeGroundRadioEncryptionKey(currentEncryptionKey))
                            {
                                qCDebug(MonarkManagerLog)<<"EchoLink reversion successful";
                            }
                            else
                            {
                                qCDebug(MonarkManagerLog)<<"EchoLink reversion failed";
                            }
                        }
                    }
                    else
                    {
                        if(m_updateFailedDrones.empty())
                        {
                            qCDebug(MonarkManagerLog)<<"All drones and EchoLink succeeded";
                        }
                        else
                        {
                            qCDebug(MonarkManagerLog)<<"Some drones failed. Re-pairing will be required";
                        }
                    }
                }
                else
                {
                    qCDebug(MonarkManagerLog)<<"Could not change EchoLink encryption key";
                    //we can't change the EchoLink's frequency
                    //therefore, we can't ping the drones to determine success
                    //Assume success in those drones, but set all other drones and the EchoLink to failure
                    m_updateSuccessfulDrones=succeededDrones;
                    emit updateSuccessfulDronesChanged();
                }
                m_updateInProgressDrones.clear();
                emit updateInProgressDronesChanged();
            }
            else
            {
                qCDebug(MonarkManagerLog)<<"All drones failed";
                //all drones failed to receive the command, so don't bother changing the EchoLink
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
                mp_monarkSettings->encryptionKey()->setRawValue(QString::fromStdString(desiredStdString));
                mp_monarkSettings->onSaveSettings();
               // _sendEncryptionKeyToGcsRadio(desiredStdString.c_str());
            }
        }
        else
        {
            qCDebug(MonarkManagerLog)<<"Bad serial connection";
            //TODO show user
            m_groundRadioUpdateState=(int) UpdateState::UpdateFailed;
            emit groundRadioUpdateStateChanged(m_groundRadioUpdateState);
            m_updateFailedDrones=m_beforeUpdateDrones;
            m_beforeUpdateDrones.clear();
            emit beforeUpdateDronesChanged();
            emit updateInProgressDronesChanged();
            emit updateSuccessfulDronesChanged();
            emit updateFailedDronesChanged();
        }
        //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeEncryptionKey()";
    }
}
