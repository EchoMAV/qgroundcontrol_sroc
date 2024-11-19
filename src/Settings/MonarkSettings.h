#pragma once

#include "SettingsGroup.h"

class MonarkSettings : public SettingsGroup
{
    Q_OBJECT
public:
    MonarkSettings(QObject* p_parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(networkID)
    DEFINE_SETTINGFACT(encryptionKey)
    DEFINE_SETTINGFACT(groundTxPower)
    DEFINE_SETTINGFACT(groundFrequency)

private slots:
    void _configChanged             (QVariant value);
};
