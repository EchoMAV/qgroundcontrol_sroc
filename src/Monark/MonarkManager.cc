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
static constexpr inline uint16_t const n_defaultGroundFrequency = 2440;
static constexpr inline uint16_t const n_maxMonarkID=3;

std::unordered_map<QString, std::set<QString>> n_validMicrohardFrequencies =
    {
        {"pMDDL2450AES256",{"2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475"}},
        // DEFAULT
        // {"pMDDL1624AES256",{"1630","1635","1640","1645","1650","1655","1660","1665","1670","1675","1680","1685","1690","1695","1700","1705","1710",
        //     "1715","1720","1785","1790","1795","1800","1805","1810","1815","1820","1825","1830","1835","1840","1845","2025","2030","2035","2040","2045",
        //     "2050","2055","2060","2065","2070","2075","2080","2085","2090","2095","2100","2105", "2205","2210","2215","2220","2225","2230","2235","2240",
        //     "2245","2250","2255","2260","2265","2270","2275","2280","2285","2290","2295", "2305","2310","2315","2320","2325","2330","2335","2340","2345",
        //     "2350","2355","2360","2365","2370","2375","2380","2385", "2405", "2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460",
        //     "2465","2470","2475", "2480","2485","2490","2495"
        // }},
        // FULL LIST
        // {"pMDDL1624AES256", {
        //     "1627","1628","1629","1630","1631","1632","1633","1634","1635","1636","1637","1638","1639","1640","1641","1642","1643","1644","1645","1646",
        //     "1647","1648","1649","1650","1651","1652","1653","1654","1655","1656","1657","1658","1659","1660","1661","1662","1663","1664","1665","1666",
        //     "1667","1668","1669","1670","1671","1672","1673","1674","1675","1676","1677","1678","1679","1680","1681","1682","1683","1684","1685","1686",
        //     "1687","1688","1689","1690","1691","1692","1693","1694","1695","1696","1697","1698","1699","1700","1701","1702","1703","1704","1705","1706",
        //     "1707","1708","1709","1710","1711","1712","1713","1714","1715","1716","1717","1718","1719","1720","1721","1722","1723",
        
        //     "1782","1783","1784","1785","1786","1787","1788","1789","1790","1791","1792","1793","1794","1795","1796","1797","1798","1799","1800","1801",
        //     "1802","1803","1804","1805","1806","1807","1808","1809","1810","1811","1812","1813","1814","1815","1816","1817","1818","1819","1820","1821",
        //     "1822","1823","1824","1825","1826","1827","1828","1829","1830","1831","1832","1833","1834","1835","1836","1837","1838","1839","1840","1841",
        //     "1842","1843","1844","1845","1846","1847","1848",
        
        //     "2022","2023","2024","2025","2026","2027","2028","2029","2030","2031","2032","2033","2034","2035","2036","2037","2038","2039","2040","2041",
        //     "2042","2043","2044","2045","2046","2047","2048","2049","2050","2051","2052","2053","2054","2055","2056","2057","2058","2059","2060","2061",
        //     "2062","2063","2064","2065","2066","2067","2068","2069","2070","2071","2072","2073","2074","2075","2076","2077","2078","2079","2080","2081",
        //     "2082","2083","2084","2085","2086","2087","2088","2089","2090","2091","2092","2093","2094","2095","2096","2097","2098","2099","2100","2101",
        //     "2102","2103","2104","2105","2106","2107","2108",
        
        //     "2202","2203","2204","2205","2206","2207","2208","2209","2210","2211","2212","2213","2214","2215","2216","2217","2218","2219","2220","2221",
        //     "2222","2223","2224","2225","2226","2227","2228","2229","2230","2231","2232","2233","2234","2235","2236","2237","2238","2239","2240","2241",
        //     "2242","2243","2244","2245","2246","2247","2248","2249","2250","2251","2252","2253","2254","2255","2256","2257","2258","2259","2260","2261",
        //     "2262","2263","2264","2265","2266","2267","2268","2269","2270","2271","2272","2273","2274","2275","2276","2277","2278","2279","2280","2281",
        //     "2282","2283","2284","2285","2286","2287","2288","2289","2290","2291","2292","2293","2294","2295","2296","2297","2298",
        
        //     "2303","2304","2305","2306","2307","2308","2309","2310","2311","2312","2313","2314","2315","2316","2317","2318","2319","2320","2321","2322",
        //     "2323","2324","2325","2326","2327","2328","2329","2330","2331","2332","2333","2334","2335","2336","2337","2338","2339","2340","2341","2342",
        //     "2343","2344","2345","2346","2347","2348","2349","2350","2351","2352","2353","2354","2355","2356","2357","2358","2359","2360","2361","2362",
        //     "2363","2364","2365","2366","2367","2368","2369","2370","2371","2372","2373","2374","2375","2376","2377","2378","2379","2380","2381","2382",
        //     "2383","2384","2385","2386","2387","2388",
        
        //     "2402","2403","2404","2405","2406","2407","2408","2409","2410","2411","2412","2413","2414","2415","2416","2417","2418","2419","2420","2421",
        //     "2422","2423","2424","2425","2426","2427","2428","2429","2430","2431","2432","2433","2434","2435","2436","2437","2438","2439","2440","2441",
        //     "2442","2443","2444","2445","2446","2447","2448","2449","2450","2451","2452","2453","2454","2455","2456","2457","2458","2459","2460","2461",
        //     "2462","2463","2464","2465","2466","2467","2468","2469","2470","2471","2472","2473","2474","2475","2476","2477","2478","2479","2480","2481",
        //     "2482","2483","2484","2485","2486","2487","2488","2489","2490","2491","2492","2493","2494","2495","2496","2497","2498"
        // }},
        // WiFi Only
        {"pMDDL1624AES256",{
            "2405","2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475","2480","2485","2490","2495"
        }},
        {"pMDDL2280AES256",{"2205","2210","2215","2220","2225","2230","2235","2240","2245","2250","2255","2260","2265","2270","2275","2280","2285","2290","2295",
                            "2300",
                            "2305","2310","2315","2320","2325","2330","2335","2340","2345","2350","2355","2360","2365","2370","2375","2380","2385",
                            "2390","2395","2400","2405",
                            "2410","2415","2420","2425","2430","2435","2440","2445","2450","2455","2460","2465","2470","2475",
                            "2480","2485","2490","2495",
                            "2500","2505","2510","2515","2520","2525","2530","2535","2540","2545","2550","2555","2560","2565","2570","2575","2580","2585","2590","2595","2600","2605","2610","2615","2620","2625","2630","2635","2640","2645","2650","2655","2660","2665","2670","2675","2680","2685","2690","2695","2700","2705","2710","2715","2720","2725","2730","2735","2740","2745","2750","2755","2760","2765","2770","2775","2780","2785","2790","2795"}}
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

QString _generateRandomKey()
{
    QString validChars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> dist(0,validChars.length()-1);
    QString result;
    QTextStream ss(&result,QIODevice::ReadWrite|QIODevice::Append);
    for(int i=0;i<16;++i)
    {
        ss<<validChars[dist(generator)];
    }
    return result;
}

QString _getDroneIPAddress(int monarkID)
{
    return "172.20.3."+QString::number(monarkID);
}

QString _convertSetToString(std::set<int> const& set, bool const includeGroundRadio)
{
    QString result;
    QTextStream ss(&result,QIODevice::ReadWrite|QIODevice::Append);
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
    return result;
}

std::pair<bool,std::vector<QString>> _sendFile(QString const& host, QString const& username, QString const& password, QFile& file, QString const& remoteLocation, bool toDrone, bool quiet=true, std::function<void(int, QString const&)> const& progressFunc=[](int, QString const&){})
{
    //TODO find a way to make the method timeout if you power off the drone
    qCDebug(MonarkManagerLog)<<"ENTER: _sendFile(host="<<host<<", username="<<username<<", file="<<file<<", remoteLocation="<<remoteLocation<<", toDrone="<<toDrone<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    sftp_session p_sftp=nullptr;
    sftp_file p_sftpFile=nullptr;
    char p_buffer[4096];
    bool success=false;
    std::vector<QString> commandResponses;
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
                qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : ssh_new failed";
            }
            progressFunc(-1, QString("ssh_new failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_new failed");
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, host.toUtf8().data());
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
                qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
                qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            progressFunc(-1, QString("ssh_connect failed: ")+ssh_get_error(p_session));
            commandResponses.push_back("ssh_connect failed");
            break;
        }
        isConnected=true;
        if(!password.isEmpty())
        {
            returnCode = ssh_userauth_password(p_session, username.toUtf8().data(), password.toUtf8().data());
            if(returnCode)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : sftp_new failed: "<<ssh_get_error(p_session);
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
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : sftp_init failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("sftp_init failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("sftp_init failed");
                break;
            }
            p_sftpFile= sftp_open(p_sftp, remoteLocation.toUtf8().data(),
                                   O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            if(!p_sftpFile)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : sftp_open failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                }
                progressFunc(-1, QString("sftp_open failed: ")+ssh_get_error(p_session));
                commandResponses.push_back("sftp_open failed");
                break;
            }
            if(!file.open(QFile::ReadOnly))
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : file.open failed";
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
                            qCCritical(MonarkManagerLog)<<"_sendFile("<<host<<") : sftp_write failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
    qCDebug(MonarkManagerLog)<<"EXIT : _sendFile(host="<<host<<", username="<<username<<", file="<<file<<", remoteLocation="<<remoteLocation<<", toDrone="<<toDrone<<")";
    return std::make_pair(success,commandResponses);
}


std::pair<bool,std::vector<QString>> _sendCommands(QString const& host, QString const& username, QString const& password, std::vector<QString> const*const p_commands, bool toDrone, bool quiet=true)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendCommands(host="<<host<<", username="<<username<<", toDrone="<<toDrone<<")";
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    ssh_channel p_channel = nullptr;
    char p_buffer[4096];
    bool commandsSent=false;
    std::vector<QString> commandResponses;
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_new failed";
            }
            commandResponses.push_back("ssh_new failed");
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, host.toUtf8().data());
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_options_set failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            commandResponses.push_back("ssh_options_set failed");
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            if(!quiet)
            {
                qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_connect failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
            }
            commandResponses.push_back("ssh_connect failed");
            break;
        }
        isConnected=true;
        if(!password.isEmpty())
        {
            returnCode = ssh_userauth_password(p_session, username.toUtf8().data(), password.toUtf8().data());
            if(returnCode)
            {
                if(!quiet)
                {
                    qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_userauth_password failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
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
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_channel_new failed: "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_new failed");
                    break;
                }
                returnCode = ssh_channel_open_session(p_channel);
                if(returnCode)
                {
                    if(!quiet)
                    {
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_channel_open_session failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_open_session failed");
                    break;
                }
                returnCode = ssh_channel_request_shell(p_channel);
                if(returnCode)
                {
                    if(!quiet)
                    {
                        qCCritical(MonarkManagerLog)<<"_sendCommands("<<host<<") : ssh_channel_request_shell failed: rc="<<returnCode<<": "<<ssh_get_error(p_session);
                    }
                    commandResponses.push_back("ssh_channel_request_shell failed");
                    break;
                }
                commandsSent=true;
                decltype(p_commands->size()) commandIndex=0;
                for(;;)
                {
                    QString responseStr;
                    auto const& command=(*p_commands)[commandIndex];
                    bool writeLog=false;
                    if(!quiet)
                    {
                        qCDebug(MonarkManagerLog)<<"_sendCommands("<<host<<") command="<<command;
                        writeLog=true;
                    }
                    //command is sent
                    ssh_channel_write(p_channel,command.toUtf8().data(), command.size());
                    bool commandSuccess=false;
                    if(command.indexOf("export")==0)
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
                                    responseStr=  QString::fromUtf8(p_buffer,p_bufferItr - (&p_buffer[0]));
                                    if(!quiet && writeLog)
                                    {
                                        //don't put passwords in the logs
                                        qCDebug(MonarkManagerLog)<<"_sendCommands("<<host<<") responseStr="<<responseStr;
                                    }
                                    if(toDrone)
                                    {
                                        if(responseStr.indexOf(np_droneSuccessStr1) >=0 || responseStr.indexOf(np_droneSuccessStr2) >=0)
                                        {
                                            commandSuccess=true;
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        if(responseStr.indexOf(np_groundRadioSuccessStr) >=0)
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
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendCommands(host="<<host<<", username="<<username<<", toDrone="<<toDrone<<")";
    return std::make_pair(commandsSent,commandResponses);
}

void _waitForDroneResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>>& droneResponses, std::set<int>& succeededDrones, std::set<int>& failedDrones)
{
    qCDebug(MonarkManagerLog)<<"ENTER: _waitForDroneResponses()";
    for(decltype(droneResponses.size()) i=0;i<droneResponses.size();++i)
    {
        auto id=droneResponses[i].first;
        auto const& response=droneResponses[i].second.get().second;
        if(response.empty() || (response.back().indexOf(np_droneSuccessStr1) < 0 && response.back().indexOf(np_droneSuccessStr2)< 0 ))
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

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> _sendDroneFrequencyChangeCommands(QString const& desiredFrequency, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency<<")";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses;
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
                                                    std::vector<QString> droneCommands;
                                                    droneCommands.push_back("microhard --action=update --frequency="+desiredFrequency+" --monark_id="+QString::number(id)+"\n");
                                                    return _sendCommands(ip,"monark", "monark", &droneCommands,true,false);
                                                })));
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_sendDroneFrequencyChangeCommands(desiredFrequency="<<desiredFrequency<<")";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> _sendDroneEncryptionKeyChangeCommands(QString const& desiredEncryptionKey, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDroneEncryptionKeyChangeCommands()";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses;
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
                                                    std::vector<QString> droneCommands;
                                                    droneCommands.push_back("export NEWEK="+desiredEncryptionKey+"\n");
                                                    droneCommands.push_back("microhard --action=update_encryption_key --monark_id="+QString::number(id)+"\n");
                                                    return _sendCommands(ip,"monark", "monark", &droneCommands,true);
                                                })));
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : _sendDroneEncryptionKeyChangeCommands()";
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> _sendDroneTxPowerChangeCommands(QString const& desiredTxPower, std::set<int>& beforeSet, std::set<int>& inProgressSet)
{
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses;
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
                                                    std::vector<QString> droneCommands;
                                                    droneCommands.push_back("microhard --action=update --tx_power="+desiredTxPower+" --monark_id="+QString::number(id)+"\n");
                                                    return _sendCommands(ip,"monark", "monark", &droneCommands,true);
                                                })));
    }
    return droneResponses;
}

std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> _sendDronePingCommands(std::set<int> const& inputSet, int numSeconds=30)
{
    //qCDebug(MonarkManagerLog)<<"ENTER: _sendDronePingCommands()";
    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> pingDroneResponses;
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
                                                        std::vector<QString> pingCommand;
                                                        pingCommand.push_back("microhard --action=info --monark_id="+QString::number(id)+"\n");
                                                        auto const startTime = std::chrono::system_clock::now();
                                                        std::pair<bool,std::vector<QString>> response;
                                                        for(;;)
                                                        {
                                                            response = _sendCommands(ip,"monark", "monark", &pingCommand, true);
                                                            if(!response.second.empty() && (response.second.back().indexOf(np_droneSuccessStr1) >=0 || response.second.back().indexOf(np_droneSuccessStr2)>=0)
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
    , mp_refreshDroneSlotHandler{std::make_unique<MonarkManagerWorkerWorker>()}
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
    , m_newGcsVersion{}
    , m_newGcsDescription{}
    , m_newGcsURL{}
    , m_newGcsReleaseDate{}
    , m_newDroneVersion{}
    , m_newDroneDescription{}
    , m_newDroneURL{}
    , m_newDroneReleaseDate{}
    , m_paired{false}
    , m_droneRadioModels{}
    , m_groundRadioModel{}
    , m_monarkUpdatePushPercent{0}
    , m_monarkUpdatePushError{}
    , m_serialMut{}
    , mp_serialPort(new QSerialPort(this))
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::MonarkManager()()";
    //connect(qgcApp()->toolbox()->corePlugin(), &QGCCorePlugin::showAdvancedUIChanged, this, &MonarkManager::validFrequenciesChanged);
    mp_slotHandler->start();
    mp_refreshDroneSlotHandler->start();
    //_checkForUpdates();
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::MonarkManager()()";
}

void MonarkManager::_findEchoLinkDevice()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_findEchoLinkDevice()";
    std::lock_guard<decltype(m_serialMut)> lock(m_serialMut);
    bool foundDevice=false;
    for (auto const& portInfo : QSerialPortInfo::availablePorts())
    {
        qCDebug(MonarkManagerLog)<<"_findEchoLinkDevice found portInfo"
                                  <<"\n\tportName        ="<<portInfo.portName()
                                  <<"\n\tdescription     ="<<portInfo.description()
                                  <<"\n\tmanufacturer    ="<<portInfo.manufacturer()
                                  <<"\n\tproductIdentifier="<<portInfo.productIdentifier()
                                  <<"\n\tvendorIdentifier ="<<portInfo.vendorIdentifier()
                                  <<"\n\tserialNumber     ="<<portInfo.serialNumber()
                                  <<"\n\tsystemLocation   ="<<portInfo.systemLocation()
                                  <<"\n\tisBusy           ="<<portInfo.isBusy()
                                  <<"\n\tisNull           ="<<portInfo.isNull();
        if(portInfo.vendorIdentifier()!=0x4b4)
        {
            continue;
        }
        if(portInfo.productIdentifier()!=0xf232)
        {
            continue;
        }
        if(mp_serialPort->isOpen())
        {
            qCDebug(MonarkManagerLog)<<"Serial port was open. Closing";
            mp_serialPort->close();
        }
        mp_serialPort->setPort(portInfo);
        if (mp_serialPort->open(QIODevice::ReadWrite))
        {
            qCDebug(MonarkManagerLog) << "Active Port:" << mp_serialPort->portName();
            if(!mp_serialPort->setBaudRate(QSerialPort::Baud115200))
            {
                qCWarning(MonarkManagerLog) << "Failed to set baud rate";
            }
            if(!mp_serialPort->setDataBits(QSerialPort::Data8))
            {
                qCWarning(MonarkManagerLog) << "Failed to set data bits";
            }
            if(!mp_serialPort->setParity(QSerialPort::NoParity))
            {
                qCWarning(MonarkManagerLog) << "Failed to set parity";
            }
            if(!mp_serialPort->setStopBits(QSerialPort::OneStop))
            {
                qCWarning(MonarkManagerLog) << "Failed to set stop bits";
            }
            if(!mp_serialPort->setFlowControl(QSerialPort::NoFlowControl))
            {
                qCWarning(MonarkManagerLog) << "Failed to set flow control";
            }
            mp_serialPort->clear();
            mp_serialPort->write("AT\r\n");
            mp_serialPort->waitForBytesWritten(10000);
            auto start = std::chrono::system_clock::now();
            QString data;
            QTextStream ss(&data,QIODevice::ReadWrite|QIODevice::Append);
            for(;;)
            {
                if(mp_serialPort->waitForReadyRead(2000))
                {
                    ss<<mp_serialPort->readAll();
                    if(data.indexOf("OK\r") >=0 || data.indexOf("ERROR: ") >=0)
                    {
                        qCDebug(MonarkManagerLog)<<"_findEchoLinkDevice returning"
                                                  <<"\n\tportName        ="<<portInfo.portName()
                                                  <<"\n\tdescription     ="<<portInfo.description()
                                                  <<"\n\tmanufacturer    ="<<portInfo.manufacturer()
                                                  <<"\n\tproductIdentifier="<<portInfo.productIdentifier()
                                                  <<"\n\tvendorIdentifier ="<<portInfo.vendorIdentifier()
                                                  <<"\n\tserialNumber     ="<<portInfo.serialNumber()
                                                  <<"\n\tsystemLocation   ="<<portInfo.systemLocation()
                                                  <<"\n\tisBusy           ="<<portInfo.isBusy()
                                                  <<"\n\tisNull           ="<<portInfo.isNull();
                        foundDevice=true;
                        break;
                    }
                }
                if(std::chrono::system_clock::now()-start>std::chrono::seconds(5))
                {
                    break;
                }
            }
        }
        if(foundDevice)
        {
            break;
        }
    }
    if(!foundDevice)
    {
        qCCritical(MonarkManagerLog)<<"Found no echolink device";
        if(mp_serialPort->isOpen())
        {
            qCDebug(MonarkManagerLog)<<"Serial port was open. Closing";
            mp_serialPort->close();
        }
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_findEchoLinkDevice()";
}

std::pair<bool,QString> MonarkManager::_runEchoLinkSSHCommand(QString const& command, bool configured, std::function<std::pair<bool,QString>(QString const&)>&& func, int numSeconds)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_runEchoLinkSSHCommand(command="<<command<<", configured="<<configured<<" numSeconds="<<numSeconds<<")";
    std::pair<bool,QString> result=std::make_pair(false,"");
    int returnCode=0;
    ssh_session p_session = nullptr;
    bool isConnected=false;
    ssh_channel p_channel = nullptr;
    char p_buffer[4096];
    QString username="admin";
    QString host = configured?np_srmPairedIp:np_srmDefaultIp;
    QString password=configured?"monarkmonark":"admin";
    do
    {
        p_session = ssh_new();
        if(!p_session)
        {
            result.second=("ssh_new failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        returnCode=ssh_options_set(p_session, SSH_OPTIONS_HOST, host.toUtf8().data());
        if(returnCode)
        {
            result.second=("ssh_options_set failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        returnCode=ssh_connect(p_session);
        if(returnCode)
        {
            result.second=("ssh_connect failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        isConnected=true;
        returnCode = ssh_userauth_password(p_session, username.toUtf8().data(), password.toUtf8().data());
        if(returnCode)
        {
            result.second=("ssh_userauth_password failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        p_channel=ssh_channel_new(p_session);
        if(!p_channel)
        {
            result.second=("ssh_channel_new failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        returnCode = ssh_channel_open_session(p_channel);
        if(returnCode)
        {
            result.second=("ssh_channel_open_session failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        returnCode = ssh_channel_request_shell(p_channel);
        if(returnCode)
        {
            result.second=("ssh_channel_request_shell failed");
            qCCritical(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
            break;
        }
        //command is sent
        ssh_channel_write(p_channel,command.toUtf8().data(), command.size());
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
                    auto resultAttempt=func(QString::fromUtf8(p_buffer,p_bufferItr - (&p_buffer[0])));
                    if(resultAttempt.first)
                    {
                        result.first=true;
                        result.second=resultAttempt.second;
                        qCDebug(MonarkManagerLog) << "MonarkManager::_runEchoLinkSSHCommand result="<<result.second;
                        break;
                    }
                }
            }
            if(std::chrono::system_clock::now() - start > std::chrono::seconds(numSeconds))
            {
                break;
            }
            if(size_t(p_bufferItr - (&p_buffer[0])) >= sizeof(p_buffer))
            {
                break;
            }
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
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_runEchoLinkSSHCommand(command="<<command<<", configured="<<configured<<" numSeconds="<<numSeconds<<")";
    return result;
}

std::pair<bool,QString> MonarkManager::_runEchoLinkSerialCommand(QString const& command, std::function<std::pair<bool,QString>(QString const&)>&& func, int numSeconds)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_runEchoLinkSerialCommand(command="<<command<<")";
    std::pair<bool,QString> result=std::make_pair(false,"");
    if(!mp_serialPort->isOpen())
    {
        _findEchoLinkDevice();
    }
    if(mp_serialPort->isOpen())
    {
        std::unique_lock<decltype(m_serialMut)> lock(m_serialMut);
        qCDebug(MonarkManagerLog)<<"Running command '"<<command<<"' on device "<<mp_serialPort->portName();
        mp_serialPort->clear();
        mp_serialPort->write(command.toUtf8());
        mp_serialPort->waitForBytesWritten(10000);
        QString data;
        QTextStream ss(&data,QIODevice::ReadWrite|QIODevice::Append);
        auto const start = std::chrono::system_clock::now();
        for(;;)
        {
            if(mp_serialPort->waitForReadyRead(2000))
            {
                ss<<mp_serialPort->readAll();
                auto resultAttempt=func(data);
                if(resultAttempt.first)
                {
                    result.first=true;
                    result.second=resultAttempt.second;
                    break;
                }
            }
            if(std::chrono::system_clock::now() - start > std::chrono::seconds(numSeconds))
            {
                qCCritical(MonarkManagerLog)<<"On device="<<mp_serialPort->portName()<<" command '"<<command<<"' no valid response was found. responseStr="<<data;
                break;
            }
        }

        mp_serialPort->clear();
        lock.unlock();
    }
    else
    {
        qCCritical(MonarkManagerLog)<<"Could not run command '"<<command<<"' because no EchoLink was found";
    }
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_runEchoLinkSerialCommand(command="<<command<<")";
    return result;
}

QString MonarkManager::_getEncryptionKeyOverSSH()
{
    auto result=_runEchoLinkSSHCommand("AT+MWVENCRYPT\r\n",true,[](QString const& data)->std::pair<bool,QString>{
        qCDebug(MonarkManagerLog)<<"MonarkManager::_getEncryptionKeyOverSSH: data="<<data;
        auto beginIndex=data.indexOf("Password: ");
        if(beginIndex>=0)
        {
            beginIndex+=10;
            auto endIndex=data.indexOf("\r\n",beginIndex);
            if(endIndex>=0)
            {
                qCDebug(MonarkManagerLog)<<"AT+MWVENCRYPT succeeded beginIndex="<<beginIndex<<" endIndex="<<endIndex<<" data='"<<data<<"'";
                return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex));
            }
        }
        else
        {
            auto errorIndex=data.indexOf("ERROR:");
            if(errorIndex>=0)
            {
                qCCritical(MonarkManagerLog)<<"AT+MWVENCRYPT failed errorIndex="<<errorIndex<<" data='"<<data<<"'";
                return std::make_pair(true,"");
            }
        }
        return std::make_pair(false,"");
    });
    return result.second;
}

void MonarkManager::_setEchoLinkRadioModel()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_setEchoLinkRadioModel()";
     m_groundRadioModel.clear();

#if 1
    auto result=_runEchoLinkSSHCommand("AT+MSSYSI\r\n",m_paired,[this](QString const& data)->std::pair<bool,QString>{
        auto errorIndex=data.indexOf("ERROR:");
        if(errorIndex>=0)
        {

            qCCritical(MonarkManagerLog)<<"AT+MSSYSI failed. Defaulting to pMDDL1624AES256. errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
            return std::make_pair(true,"pMDDL1624AES256");
        }
        auto beginIndex=data.indexOf("Image");
        if(beginIndex>=0)
        {
            beginIndex=data.indexOf(": ", beginIndex);
            if(beginIndex>=0)
            {
                beginIndex+=2;
                auto endIndex=data.indexOf("\r",beginIndex);
                if(endIndex>=0)
                {
                    qCDebug(MonarkManagerLog)<<"AT+MSSYSI succeeded beginIndex="<<beginIndex<<", endIndex="<<endIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                    return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex).trimmed());
                }
            }
            auto endIndex=data.indexOf("\r",beginIndex);
            if(endIndex>=0)
            {
                qCDebug(MonarkManagerLog)<<"AT+MSSYSI succeeded beginIndex="<<beginIndex<<", endIndex="<<endIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex).trimmed());
            }
        }
        return std::make_pair(false,"");
     });
#else

    auto result=_runEchoLinkSerialCommand("AT+GETRADIOINFO\r\n",[this](QString const& data)->std::pair<bool,QString>{
        auto errorIndex=data.indexOf("ERROR:");
        if(errorIndex>=0)
        {

            qCCritical(MonarkManagerLog)<<"AT+GETRADIOINFO failed. Defaulting to pMDDL1624AES256. errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
            return std::make_pair(true,"pMDDL1624AES256");
        }
        auto beginIndex=data.indexOf("Image");
        if(beginIndex>=0)
        {
            beginIndex=data.indexOf(": ", beginIndex);
            if(beginIndex>=0)
            {
                beginIndex+=2;
                auto endIndex=data.indexOf("\r",beginIndex);
                if(endIndex>=0)
                {
                    qCDebug(MonarkManagerLog)<<"AT+GETRADIOINFO succeeded beginIndex="<<beginIndex<<", endIndex="<<endIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                    return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex));
                }
            }
            auto endIndex=data.indexOf("\r",beginIndex);
            if(endIndex>=0)
            {
                qCDebug(MonarkManagerLog)<<"AT+GETRADIOINFO succeeded beginIndex="<<beginIndex<<", endIndex="<<endIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex));
            }
        }
        return std::make_pair(false,"");
    });
#endif
    if(result.first)
    {
        m_groundRadioModel=result.second;
        qCDebug(MonarkManagerLog)<<"Radio model is "<<m_groundRadioModel;
    }
    else
    {
        qCCritical(MonarkManagerLog)<<"No valid response from echolink on command AT+GETRADIOINFO. Defaulting to pMDDL1624AES256";
        m_groundRadioModel="pMDDL1624AES256";
    }
    emit minMaxPowersChanged();
    emit validFrequenciesChanged();
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_setEchoLinkRadioModel()";
}

void MonarkManager::_sendPasswordOverSerial(QString const& password)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_sendPasswordOverSerial()";
#if 1
    (void)_runEchoLinkSerialCommand("AT+SETADMIN="+password+"\r\n",[this](QString const& data)->std::pair<bool,QString>{
        auto okIndex=data.indexOf("OK\r");
        if(okIndex>=0)
        {
            qCDebug(MonarkManagerLog)<<"AT+SETADMIN succeeded okIndex="<<okIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
            return std::make_pair(true,"SUCCESS");
        }
        auto errorIndex=data.indexOf("ERROR:");
        if(errorIndex>=0)
        {
            qCCritical(MonarkManagerLog)<<"AT+SETADMIN failed errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
            return std::make_pair(true,"FAIL");
        }
        else
        {

            return std::make_pair(false,"");
        }
    }, 20);
#endif
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_sendPasswordOverSerial()";
}
bool MonarkManager::_changeGroundRadioEncryptionKey(QString const& currentEncryptionKey, QString const& desiredKey, bool reversion)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioEncryptionKey()";
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
    bool result=false;
    auto const newKey=reversion?currentEncryptionKey:desiredKey;

#if 1
    std::vector<QString> commands;
    commands.push_back("AT+MWVENCRYPT=2,"+newKey+"\n");
    commands.push_back("AT&W\n");
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &commands, false).second;
    result=!response.empty() && response.back().indexOf(np_groundRadioSuccessStr)>=0;
#else

    auto const result1=_runEchoLinkSerialCommand("AT+SETENCRYPTION=2,"+newKey+"\r\n",[this](QString const& data)->std::pair<bool,QString>{
        auto okIndex=data.indexOf("OK\r");
        if(okIndex>=0)
        {
            qCDebug(MonarkManagerLog)<<"AT+SETENCRYPTION succeeded okIndex="<<okIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
            return std::make_pair(true,"SUCCESS");
        }
        auto errorIndex=data.indexOf("ERROR:");
        if(errorIndex>=0)
        {
            qCCritical(MonarkManagerLog)<<"AT+SETENCRYPTION failed errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
             return std::make_pair(true,"FAIL");
        }
        return std::make_pair(false,"");
    }, 20);
    if(result1.first && result1.second=="SUCCESS")
    {
        auto const result2=_runEchoLinkSerialCommand("AT+SETNEWPASSWORD="+newKey+"\r\n",[this](QString const& data)->std::pair<bool,QString>{
            auto okIndex=data.indexOf("OK\r");
            if(okIndex>=0)
            {#if 0
                qCDebug(MonarkManagerLog)<<"AT+SETNEWPASSWORD succeeded okIndex="<<okIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,"SUCCESS");
            }
            auto errorIndex=data.indexOf("ERROR:");
            if(errorIndex>=0)
            {
                qCCritical(MonarkManagerLog)<<"AT+SETNEWPASSWORD failed errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,"FAIL");
            }
            return std::make_pair(false,"");
        }, 20);
        if(result2.first && result2.second=="SUCCESS")
        {
            result=true;
        }
    }
#endif

    if(result)
    {
        m_groundRadioUpdateState=reversion?(int)UpdateState::UpdateFailed:(int) UpdateState::UpdateSuccessful;
    }
    else
    {
        m_groundRadioUpdateState=reversion?(int) UpdateState::UpdateSuccessful:(int) UpdateState::UpdateFailed;
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
    qCDebug(MonarkManagerLog)<<"EXIT: MonarkManager::_changeGroundRadioEncryptionKey() result="<<result;
    return result;
}

QString MonarkManager::_getEncryptionKeyOverSerial()
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_getEncryptionKeyOverSerial()";
#if 1
    //auto const result=_runEchoLinkSerialCommand("AT+MWVENCRYPT\r\n",[this](QString const& data)->std::pair<bool,QString>{
    auto const result=_runEchoLinkSerialCommand("AT+GETENCRYPTION\r\n",[this](QString const& data)->std::pair<bool,QString>{

        qCDebug(MonarkManagerLog)<<"MonarkManager::_getEncryptionKeyOverSerial() data="<<data;
        auto beginIndex=data.indexOf("Password: ");
        if(beginIndex>=0)
        {
            beginIndex+=10;
            auto endIndex=data.indexOf("\r\r\n",beginIndex);
            if(endIndex>=0)
            {
                qCDebug(MonarkManagerLog)<<"AT+GETENCRYPTION succeeded beginIndex="<<beginIndex<<" endIndex="<<endIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,data.mid(beginIndex,endIndex-beginIndex));
            }
        }
        else
        {
            auto errorIndex=data.indexOf("ERROR:");
            if(errorIndex>=0)
            {
                qCCritical(MonarkManagerLog)<<"AT+GETENCRYPTION failed errorIndex="<<errorIndex<<" port='"<<mp_serialPort->portName()<<" data='"<<data<<"'";
                return std::make_pair(true,"");
            }
        }
        return std::make_pair(false,"");
    });
#endif
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_getEncryptionKeyOverSerial()";
    return result.second;
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
    mp_refreshDroneSlotHandler->shutdown();
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
#if 1
        auto pingPairedResponseFuture = std::async(std::launch::async,[](){

            return _sendCommands(np_srmPairedIp, "admin", "monarkmonark", nullptr, false);
        });
        auto const pingDefaultResponse=_sendCommands(np_srmDefaultIp, "admin", nullptr, nullptr, false).second;
        if(!pingDefaultResponse.empty() && pingDefaultResponse.back() == np_groundRadioSuccessStr)
        {
            qCDebug(MonarkManagerLog)<<"ScanSuccessPairingRequired";
            scanningResult=MonarkState::ScanSuccessPairingRequired;
            //send monarkmonark over serial
            _sendPasswordOverSerial("monarkmonark");
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
            if(!pingPairedResponse.empty())
            {
                bool ok=false;
                QString encryptionKey;
                if(pingPairedResponse.back() == np_groundRadioSuccessStr)
                {
                    encryptionKey=_getEncryptionKeyOverSSH();
                    if(!encryptionKey.isEmpty())
                    {
                        ok=true;
                    }
                }
                if(!ok)
                {
                    encryptionKey = _getEncryptionKeyOverSerial();
                    if(!encryptionKey.isEmpty())
                    {
                        std::vector<QString> commands;
                        commands.push_back("AT+MSPWD=monarkmonark,monarkmonark\n");
                        commands.push_back("AT&W\n");
                        auto const& response = _sendCommands(np_srmPairedIp,"admin", encryptionKey, &commands, false).second;
                        if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr)>=0)
                        {
                            ok=true;
                        }
                    }
                }
                if(ok)
                {
                    _sendPasswordOverSerial("monarkmonark");
                    this->mp_monarkSettings->encryptionKey()->setCookedValue(encryptionKey);
                    mp_monarkSettings->onSaveSettings();
                    m_droneRadioModels.clear();
                    m_allDrones.clear();
                    m_beforeUpdateDrones.clear();
                    m_updateInProgressDrones.clear();
                    m_updateSuccessfulDrones.clear();
                    m_updateFailedDrones.clear();
                    std::vector<std::future<std::vector<QString>>> dronePingResponses;
                    for(auto i=1;i<=n_maxMonarkID;++i)
                    {
                        dronePingResponses.push_back(std::async(std::launch::async,[i](){
                            std::vector<QString> dronePingCommands;
                            dronePingCommands.push_back("microhard --action=info --monark_id="+QString::number(i)+"\n");
                            auto const ip = _getDroneIPAddress(i);
                            return _sendCommands(ip, "monark", "monark", &dronePingCommands, true).second;
                        }));
                    }

                    qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                    scanningResult=MonarkState::ScanSuccessAndPaired;
                    m_paired=true;
                    _initializeNetworkId();
                    _initializeTxPower();
                    _initializeFrequency();
                    for(auto i=1;i<=n_maxMonarkID;++i)
                    {
                        auto response = dronePingResponses[i-1].get();
                        if(!response.empty())
                        {
                            auto const& backResponse=response.back();
                            if(!backResponse.startsWith("ssh_") || !backResponse.endsWith(" failed"))
                            {
                                if(_parseInfoJsonResponse(backResponse))
                                {
                                    m_beforeUpdateDrones.insert(i);
                                    m_allDrones.insert(i);
                                }
                            }
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
                    qgcApp()->showAppMessage("Cannot authenticate with EchoLink. Restart application and try again");
                }

            }
            else
            {
                 qCDebug(MonarkManagerLog)<<"ScanFailedNotDetected";
            }

        }
#else
        auto password = _getEncryptionKeyOverSerial();


        bool sendPassword=false;
        if(password.isEmpty())
        {
            sendPassword=true;
            password = this->mp_monarkSettings->encryptionKey()->cookedValueString();
        }
        else
        {
            this->mp_monarkSettings->encryptionKey()->setCookedValue(password);
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"found password "<<password;
        auto pingPairedResponseFuture = std::async(std::launch::async,[&password](){

            return _sendCommands(np_srmPairedIp, "admin", password, nullptr, false);
        });


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
                std::vector<std::future<std::vector<QString>>> dronePingResponses;


                for(auto i=1;i<=n_maxMonarkID;++i)
                {
                    dronePingResponses.push_back(std::async(std::launch::async,[i](){
                        std::vector<QString> dronePingCommands;
                        dronePingCommands.push_back("microhard --action=info --monark_id="+QString::number(i)+"\n");
                        auto const ip = _getDroneIPAddress(i);
                        return _sendCommands(ip, "monark", "monark", &dronePingCommands, true).second;
                    }));
                }

                qCDebug(MonarkManagerLog)<<"ScanSuccessAndPaired";
                scanningResult=MonarkState::ScanSuccessAndPaired;
                if(sendPassword)
                {
                    _sendPasswordOverSerial( password);

                }
                m_paired=true;
                _initializeNetworkId();
                _initializeTxPower();
                _initializeFrequency();
                for(auto i=1;i<=n_maxMonarkID;++i)
                {
                    auto response = dronePingResponses[i-1].get();
                    if(!response.empty())
                    {
                        auto const& backResponse=response.back();
                        if(!backResponse.startsWith("ssh_") || !backResponse.endsWith(" failed"))
                        {
                            if(_parseInfoJsonResponse(backResponse))
                            {
                                m_beforeUpdateDrones.insert(i);
                                m_allDrones.insert(i);
                            }
                        }
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
#endif
        _setMonarkState(scanningResult);
        _setEchoLinkRadioModel();


        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::startScanning()";
    }
}

void MonarkManager::refreshDroneList()
{
    if(mp_refreshDroneSlotHandler->needDispatch())
    {
        mp_refreshDroneSlotHandler->dispatch([this](){refreshDroneList();});
    }
    else
    {
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::refreshDroneList()";
        auto oldAllDrones=m_allDrones;
        m_allDrones.clear();
        m_droneRadioModels.clear();
        std::vector<std::future<std::vector<QString>>> dronePingResponses;
        for(auto i=1;i<=n_maxMonarkID;++i)
        {
            dronePingResponses.push_back(std::async(std::launch::async,[i](){
                std::vector<QString> dronePingCommands;
                dronePingCommands.push_back("microhard --action=info --monark_id="+QString::number(i)+"\n");
                auto const ip = _getDroneIPAddress(i);
                return _sendCommands(ip, "monark", "monark", &dronePingCommands, true,false).second;
            }));
        }

        for(auto i=1;i<=n_maxMonarkID;++i)
        {
            auto response = dronePingResponses[i-1].get();
            if(!response.empty())
            {
                auto const& backResponse=response.back();
                if(!backResponse.startsWith("ssh_") || !backResponse.endsWith(" failed"))
                {
                    if(_parseInfoJsonResponse(backResponse))
                    {
                        if(m_allDrones.insert(i).second)
                        {
                            emit allDronesChanged();
                        }
                    }
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
        this->refreshDroneList();
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::removeDrone(monarkID="<<monarkID<<")";
    }
}

void MonarkManager::_initializeNetworkId()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeNetworkId(paired="<<m_paired<<")";
    QString macAddress;
    std::vector<QString> commands;
    commands.push_back("AT+MNEMAC\n");
    auto encryptionKey=m_paired?mp_monarkSettings->getOldEncryptionKey():np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &commands, false).second;
    if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr) >=0)
    {
        auto macStart = response.back().indexOf('"',0);
        auto macEnd = response.back().indexOf('"',macStart+1);
        if(macStart >=0 && macEnd>=0 && macStart != macEnd)
        {
            ++macStart;
            macAddress = response.back().mid(macStart, macEnd-macStart);
            macAddress.remove(":");
        }
        constexpr int const networkIDLength=4;
        if(macAddress.length()>networkIDLength)
        {
            macAddress=macAddress.mid(macAddress.length()-networkIDLength,networkIDLength);
        }
    }
    if(!macAddress.isEmpty())
    {
        this->mp_monarkSettings->networkID()->setCookedValue("MONARK-"+macAddress);
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeNetworkId(paired="<<m_paired<<")";
}

void MonarkManager::_initializeFrequency()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeFrequency(paired="<<paired<<")";
    QString frequency="";
    std::vector<QString> commands;
    commands.push_back("AT+MWFREQ\n");
    auto encryptionKey=m_paired?mp_monarkSettings->getOldEncryptionKey():np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &commands, false).second;
    if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr) >=0)
    {
        auto freqStart = response.back().lastIndexOf(':');
        if(freqStart >=0)
        {
            freqStart=response.back().indexOf(' ',freqStart);
            if(freqStart>=0)
            {
                auto const freqEnd = response.back().indexOf(' ', freqStart+1);
                if(freqEnd>=0)
                {
                    ++freqStart;
                    frequency = response.back().mid(freqStart, freqEnd-freqStart);
                    qCDebug(MonarkManagerLog)<<"frequency= "<<frequency;
                }
            }
        }
    }
    if(!frequency.isEmpty())
    {
        this->mp_monarkSettings->groundFrequency()->setCookedValue(frequency);
    }
    //qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_initializeFrequency(paired="<<paired<<")";
}

void MonarkManager::_initializeTxPower()
{
    //qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_initializeTxPower(paired="<<paired<<")";
    QString txPower="";
    std::vector<QString> commands;
    commands.push_back("AT+MWTXPOWER\n");
    auto encryptionKey=m_paired?mp_monarkSettings->encryptionKey()->rawValueString(): np_sshUsername;
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &commands, false).second;
    if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr)>=0)
    {
        auto freqStart = response.back().lastIndexOf(':');
        if(freqStart>=0)
        {
            freqStart=response.back().indexOf(' ',freqStart);
            if(freqStart >=0)
            {
                auto const freqEnd = response.back().indexOf(' ', freqStart+1);
                if(freqEnd>=0)
                {
                    ++freqStart;
                    txPower = response.back().mid(freqStart, freqEnd-freqStart);
                    qCDebug(MonarkManagerLog)<<"txPower= "<<txPower;
                }
            }
        }
    }
    if(!txPower.isEmpty())
    {
        this->mp_monarkSettings->groundTxPower()->setCookedValue(txPower);
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
        std::vector<QString> commands;
        commands.push_back("monark-updater --version\n");
        QString ip = _getDroneIPAddress(monarkId);

        auto const& response = _sendCommands(ip,"monark", "monark", &commands, true,false).second;
        if(!response.empty())
        {
            auto const currentVersionString = response.back();
            needsUpdate= _shouldUpdate(currentVersionString,m_newDroneVersion);
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
                QString ip = _getDroneIPAddress(monarkID);
                _sendFile(ip,"monark", "monark",firmwareFile,"/home/monark/monark-updates.zip",true,false,[this](int percent,QString const& errorMessage){
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
                std::vector<QString> droneCommands;
                droneCommands.push_back("microhard --action=reboot_rpi\n");
                _sendCommands(ip,"monark", "monark", &droneCommands, true,false);
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
        std::vector<QString> commands;
#if 1
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString();

        bool settingsChanged=false;
        if(encryptionKey.isEmpty())
        {
            encryptionKey=_generateRandomKey();
            settingsChanged=true;

        }
        auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString();
        mp_monarkSettings->groundFrequency()->setCookedValue(frequency);
        auto networkId=mp_monarkSettings->networkID()->cookedValueString();
        commands.push_back("AT+MWRADIO=1\n");
        commands.push_back("AT+MWDISTANCE=12000\n"); //acceptable RF distance 12 km
        commands.push_back("AT+MWTXPOWER="+txPower+"\n");
        commands.push_back("AT+MWFREQ="+frequency+"\n");
        commands.push_back("AT+MWNETWORKID="+networkId+"\n");
        commands.push_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
        commands.push_back("AT+MSPWD=monarkmonark,monarkmonark\n");
        commands.push_back("AT+MWVMODE=0\n"); // master
        commands.push_back("AT+MWBAND=1\n"); // 4 MHz
        commands.push_back("AT+MWVRATE=6\n"); // QPSK FEC 3/4
        commands.push_back("AT+MWMCASTRT=1\n"); //QPSK FEC 3/4
        commands.push_back(QString("AT+MNLAN=LAN,EDIT,0,")+np_srmPairedIp+",255.255.0.0,0\n");
        commands.push_back(QString("AT+MNLANDHCP=LAN,1,")+np_srocIp+",1,0\n");
        commands.push_back("AT&W\n");
        auto saveResult=MonarkState::SaveSettingsFailed;
        auto const& response = _sendCommands(np_srmDefaultIp,"admin", np_sshUsername, &commands, false).second;
        if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr)>=0)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            // also send the encryption key to the radio microcontroller over serial so it can reset the microhard natively.
            if(settingsChanged)
            {
                mp_monarkSettings->encryptionKey()->setCookedValue(encryptionKey);
                mp_monarkSettings->onSaveSettings();
            }
            //_sendPasswordOverSerial(encryptionKey);

        }
#else
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString();

        bool settingsChanged=false;
        if(encryptionKey.isEmpty())
        {
            encryptionKey=_generateRandomKey();
            settingsChanged=true;

        }
        auto txPower=mp_monarkSettings->groundTxPower()->cookedValueString();
        mp_monarkSettings->groundFrequency()->setCookedValue(frequency);
        auto networkId=mp_monarkSettings->networkID()->cookedValueString();
        commands.push_back("AT+MWRADIO=1\n");
        commands.push_back("AT+MWDISTANCE=12000\n"); //acceptable RF distance 12 km
        commands.push_back("AT+MWTXPOWER="+txPower+"\n");
        commands.push_back("AT+MWFREQ="+frequency+"\n");
        commands.push_back("AT+MWNETWORKID="+networkId+"\n");
        commands.push_back("AT+MWVENCRYPT=2,"+encryptionKey+"\n");
        commands.push_back("AT+MSPWD="+encryptionKey+","+encryptionKey+"\n");
        commands.push_back("AT+MWVMODE=0\n"); // master
        commands.push_back("AT+MWBAND=1\n"); // 4 MHz
        commands.push_back("AT+MWVRATE=6\n"); // QPSK FEC 3/4
        commands.push_back("AT+MWMCASTRT=1\n"); //QPSK FEC 3/4
        commands.push_back(QString("AT+MNLAN=LAN,EDIT,0,")+np_srmPairedIp+",255.255.0.0,0\n");
        commands.push_back(QString("AT+MNLANDHCP=LAN,1,")+np_srocIp+",1,0\n");
        commands.push_back("AT&W\n");

        auto saveResult=MonarkState::SaveSettingsFailed;
        auto const& response = _sendCommands(np_srmDefaultIp,"admin", np_sshUsername, &commands, false).second;
        if(!response.empty() && response.back().indexOf(np_groundRadioSuccessStr)>=0)
        {
            saveResult=MonarkState::ScanSuccessAndPaired;
            // also send the encryption key to the radio microcontroller over serial so it can reset the microhard natively.
            if(settingsChanged)
            {
                mp_monarkSettings->encryptionKey()->setCookedValue(encryptionKey);
                mp_monarkSettings->onSaveSettings();
            }
            _sendPasswordOverSerial(encryptionKey);

        }
#endif
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
            std::vector<QString> droneCommands;
            droneCommands.push_back("microhard --action=reset --monark_id="+QString::number(activeVehicleId)+"\n");
            auto const response= _sendCommands(ip,"monark", "monark", &droneCommands,true,false).second;
            if(!response.empty() && (response.back().indexOf(np_droneSuccessStr1)>=0 || response.back().indexOf(np_droneSuccessStr2)>=0))
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
        _setMonarkState(MonarkState::ShowQRCode);
        auto detectionResult=MonarkState::DetectionFailed;
        auto encryptionKey=mp_monarkSettings->encryptionKey()->cookedValueString();
        auto ip = _getDroneIPAddress(monarkID);
        auto const startTime = std::chrono::system_clock::now();
        std::vector<QString> commands;
        commands.push_back("microhard --action=info --monark_id="+QString::number(monarkID)+"\n");
        for(;;)
        {
            if((std::chrono::system_clock::now()-startTime) > std::chrono::seconds(210))
            {
                break;
            }
            //if(MonarkManagerLog().isDebugEnabled())
            //{
            //    qgcApp()->toolbox()->audioOutput()->say("Beep");
            //}
            auto const& response = _sendCommands(ip,"monark", "monark", &commands, true, false).second;
            if(!response.empty())
            {
                auto const& backResponse=response.back();
                if(!backResponse.startsWith("ssh_") || !backResponse.endsWith(" failed"))
                {
                    if(_parseInfoJsonResponse(backResponse))
                    {
                        qCDebug(MonarkManagerLog)<<"found drone";
                        m_allDrones.insert(monarkID);
                        emit allDronesChanged();
                        m_beforeUpdateDrones.insert(monarkID);
                        emit beforeUpdateDronesChanged();
                        detectionResult=MonarkState::ScanSuccessAndPaired;
                        qgcApp()->showAppMessage(QString("MONARK-")+monarkID+" has been added.");
                        break;
                    }
                }
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

bool MonarkManager::_changeGroundRadioFrequency(QString const& desiredFrequency, bool reversion)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency<<", reversion="<<reversion<<")";

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
    std::vector<QString> groundRadioCommands;
    groundRadioCommands.push_back("AT+MWFREQ="+desiredFrequency+"\n");
    groundRadioCommands.push_back("AT&W\n");
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &groundRadioCommands, false);
    if(response.second.empty() || response.second.back().indexOf(np_groundRadioSuccessStr)<0)
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
    qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::_changeGroundRadioFrequency(desiredFrequency="<<desiredFrequency<<", reversion="<<reversion<<") result="<<response.first;
    return response.first;
}



bool MonarkManager::_changeGroundRadioTxPower(QString const& desiredPower)
{
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
    std::vector<QString> groundRadioCommands;
    groundRadioCommands.push_back("AT+MWTXPOWER="+desiredPower+"\n");
    groundRadioCommands.push_back("AT&W\n");
    auto const& response = _sendCommands(m_paired?np_srmPairedIp:np_srmDefaultIp,"admin", m_paired?"monarkmonark":"admin", &groundRadioCommands, false,false);
    if(response.second.empty() || response.second.back().indexOf(np_groundRadioSuccessStr)<0)
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
    return response.first;
}

void MonarkManager::_waitForPingResponses(std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>>& pingDroneResponses, std::function<void(int)> const& responseGoodFunc, std::function<void(int)> const& responseBadFunc)
{
    qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::_waitForPingResponses()";
    for(decltype(pingDroneResponses.size()) i=0;i<pingDroneResponses.size();++i)
    {
        auto id=pingDroneResponses[i].first;
        auto const& response=pingDroneResponses[i].second.get().second;
        m_updateInProgressDrones.erase(id);
        emit updateInProgressDronesChanged();
        if(response.empty() || (response.back().indexOf(np_droneSuccessStr1)<0 && response.back().indexOf(np_droneSuccessStr2)<0))
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
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower<<")";
        _resetToBeforeUpdate();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses=_sendDroneTxPowerChangeCommands(desiredTxPower,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        _changeGroundRadioTxPower(desiredTxPower);
        _waitForPingResponses(droneResponses,
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeTxPower(desiredTxPower="<<desiredTxPower<<")";
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
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency<<")";
        _resetToBeforeUpdate();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses=_sendDroneFrequencyChangeCommands(desiredFrequency,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        std::set<int> failedDrones;
        std::set<int> succeededDrones;
        _waitForDroneResponses(droneResponses,succeededDrones,failedDrones);
        if(failedDrones.empty())
        {
            if(_changeGroundRadioFrequency(desiredFrequency,false))
            {
                qCDebug(MonarkManagerLog)<<"EchoLink frequency change successful";
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones,60);
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
                    auto const oldFrequency=QString::number(mp_monarkSettings->groundFrequency()->rawValue().toUInt());
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
                        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> revertDroneResponses=_sendDroneFrequencyChangeCommands(oldFrequency,m_updateSuccessfulDrones,m_updateInProgressDrones);
                        emit updateSuccessfulDronesChanged();
                        emit updateInProgressDronesChanged();
                        for(decltype(revertDroneResponses.size()) i=0;i<revertDroneResponses.size();++i)
                        {
                            droneResponses[i].second.wait();
                        }
                        //revert the EchoLink back
                        if(_changeGroundRadioFrequency(oldFrequency,true))
                        {
                            qCDebug(MonarkManagerLog)<<"EchoLink reverted. Pinging drones";
                            //try to ping the drones that we reverted
                            std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> revertPingDroneResponses = _sendDronePingCommands(m_updateInProgressDrones);
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
            if(_changeGroundRadioFrequency(desiredFrequency,false))
            {
                qCDebug(MonarkManagerLog)<<"EchoLink on new frequency. Pinging drones";
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones);
                for(decltype(pingDroneResponses.size()) i=0;i<pingDroneResponses.size();++i)
                {
                    auto id=pingDroneResponses[i].first;
                    auto const& response=pingDroneResponses[i].second.get().second;
                    if(response.empty() || (response.back().indexOf(np_droneSuccessStr1)<0 && response.back().indexOf(np_droneSuccessStr2)<0))
                    {
                        succeededDrones.erase(id);
                        m_updateInProgressDrones.erase(id);
                        emit updateInProgressDronesChanged();
                        m_updateFailedDrones.insert(id);
                        emit updateFailedDronesChanged();
                    }
                }
                auto const oldFrequency=QString::number(mp_monarkSettings->groundFrequency()->rawValue().toUInt());
                if(succeededDrones.empty())
                {
                    qCDebug(MonarkManagerLog)<<"No drones pinged. Reverting EchoLink";
                    //no drones were successfully changed, so revert the EchoLink and you're done
                    _changeGroundRadioFrequency(oldFrequency,true);
                }
                else
                {
                    qCDebug(MonarkManagerLog)<<"Some drones pinged. Sending reversion commands to drones";
                    std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> revertDroneResponses=_sendDroneFrequencyChangeCommands(oldFrequency,succeededDrones,failedDrones);
                    for(decltype(revertDroneResponses.size()) i=0;i<revertDroneResponses.size();++i)
                    {
                        droneResponses[i].second.wait();
                    }
                    //revert the EchoLink back
                    if(_changeGroundRadioFrequency(oldFrequency,true))
                    {
                        qCDebug(MonarkManagerLog)<<"EchoLink reverted. Pinging drones";
                        //try to ping the drones that we reverted
                        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> revertPingDroneResponses = _sendDronePingCommands(failedDrones);
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
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeFrequencies(desiredFrequency="<<desiredFrequency<<")";

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
        qCDebug(MonarkManagerLog)<<"ENTER: MonarkManager::changeEncryptionKey()";
        _resetToBeforeUpdate();
        auto const currentEncryptionKey=            mp_monarkSettings->encryptionKey()->rawValueString();
        auto const desiredStdString=_generateRandomKey();
        std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> droneResponses=_sendDroneEncryptionKeyChangeCommands(desiredStdString,m_beforeUpdateDrones,m_updateInProgressDrones);
        emit beforeUpdateDronesChanged();
        emit updateInProgressDronesChanged();
        std::set<int> failedDrones;
        std::set<int> succeededDrones;
        _waitForDroneResponses(droneResponses,succeededDrones,failedDrones);

        if(failedDrones.empty())
        {

            if(_changeGroundRadioEncryptionKey(currentEncryptionKey, desiredStdString, false))
            {
                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones,60);
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
                        if(_changeGroundRadioEncryptionKey(currentEncryptionKey, desiredStdString, true))
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
            if(_changeGroundRadioEncryptionKey(currentEncryptionKey, desiredStdString, false))
            {
                qCDebug(MonarkManagerLog)<<"EchoLink changed. Pinging drones";

                std::vector<std::pair<int,std::future<std::pair<bool,std::vector<QString>>>>> pingDroneResponses = _sendDronePingCommands(succeededDrones, 60);
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
                        if(_changeGroundRadioEncryptionKey(currentEncryptionKey, desiredStdString, true))
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
            mp_monarkSettings->encryptionKey()->setRawValue(desiredStdString);
            mp_monarkSettings->onSaveSettings();
        }
        qCDebug(MonarkManagerLog)<<"EXIT : MonarkManager::changeEncryptionKey()";
    }
}
