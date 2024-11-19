#include "MonarkSettings.h"

DECLARE_SETTINGGROUP(Monark, "Monark")
{
    qmlRegisterUncreatableType<MonarkSettings>("QGroundControl.SettingsManager", 1, 0, "MonarkSettings", "Reference only");
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, networkID)
{
    if (!_networkIDFact) {
        _networkIDFact = _createSettingsFact(networkIDName);
        connect(_networkIDFact, &Fact::valueChanged, this, &MonarkSettings::_configChanged);
    }
    return _networkIDFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, encryptionKey)
{
    if (!_encryptionKeyFact) {
        _encryptionKeyFact = _createSettingsFact(encryptionKeyName);
        connect(_encryptionKeyFact, &Fact::valueChanged, this, &MonarkSettings::_configChanged);
    }
    return _encryptionKeyFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, groundTxPower)
{
    if (!_groundTxPowerFact) {
        _groundTxPowerFact = _createSettingsFact(groundTxPowerName);
        connect(_groundTxPowerFact, &Fact::valueChanged, this, &MonarkSettings::_configChanged);
    }
    return _groundTxPowerFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, groundFrequency)
{
    if (!_groundFrequencyFact) {
        _groundFrequencyFact = _createSettingsFact(groundFrequencyName);
        connect(_groundFrequencyFact, &Fact::valueChanged, this, &MonarkSettings::_configChanged);
    }
    return _groundFrequencyFact;
}


void MonarkSettings::_configChanged(QVariant)
{
    //TODO do something here?
}
