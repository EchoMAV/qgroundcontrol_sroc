#include "atakvehiclemanagersettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(ATAKVehicleManager, "ATAKVehicleManager")
{
    qmlRegisterUncreatableType<ATAKVehicleManagerSettings>("QGroundControl.SettingsManager", 1, 0, "ADSBVehicleManagerSettings", "Reference only");
}

DECLARE_SETTINGSFACT(ATAKVehicleManagerSettings, atakServerHostAddress)
DECLARE_SETTINGSFACT(ATAKVehicleManagerSettings, atakServerPort)
