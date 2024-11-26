#pragma once

#include "SettingsGroup.h"

class ATAKVehicleManagerSettings : public SettingsGroup
{
    Q_OBJECT
public:
    ATAKVehicleManagerSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(atakServerHostAddress)
    DEFINE_SETTINGFACT(atakServerPort)
};
