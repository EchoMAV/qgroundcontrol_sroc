#include "MonarkSettings.h"

Q_DECLARE_LOGGING_CATEGORY(MonarkManagerLog)


DECLARE_SETTINGGROUP(Monark, "Monark")
{
    qmlRegisterUncreatableType<MonarkSettings>("QGroundControl.SettingsManager", 1, 0, "MonarkSettings", "Reference only");

    onSaveSettings();
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, networkID)
{
    if (!_networkIDFact) {
        _networkIDFact = _createSettingsFact(networkIDName);
        connect(_networkIDFact, &Fact::valueChanged, this, &MonarkSettings::_networkIdChanged);
    }
    return _networkIDFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, encryptionKey)
{
    if (!_encryptionKeyFact) {
        _encryptionKeyFact = _createSettingsFact(encryptionKeyName);
        connect(_encryptionKeyFact, &Fact::valueChanged, this, &MonarkSettings::_encryptionKeyChanged);
    }
    return _encryptionKeyFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, groundTxPower)
{
    if (!_groundTxPowerFact) {
        _groundTxPowerFact = _createSettingsFact(groundTxPowerName);
        connect(_groundTxPowerFact, &Fact::valueChanged, this, &MonarkSettings::_groundTxPowerChanged);
    }
    return _groundTxPowerFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, groundFrequency)
{
    if (!_groundFrequencyFact) {
        _groundFrequencyFact = _createSettingsFact(groundFrequencyName);
        connect(_groundFrequencyFact, &Fact::valueChanged, this, &MonarkSettings::_groundFrequencyChanged);
    }
    return _groundFrequencyFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(MonarkSettings, monarkID)
{
    if (!_monarkIDFact) {
        _monarkIDFact = _createSettingsFact(monarkIDName);
        connect(_monarkIDFact, &Fact::valueChanged, this, &MonarkSettings::_monarkIDChanged);
    }
    return _monarkIDFact;
}

void MonarkSettings::onSaveSettings()
{
    m_oldNetworkId=networkID()->cookedValueString();
    m_oldEncryptionKey=encryptionKey()->cookedValueString();
    m_oldGroundTxPower=groundTxPower()->cookedValue().toUInt();
    m_oldGroundFrequency=groundFrequency()->cookedValue().toUInt();
}

bool MonarkSettings::networkIdDirty() {return m_oldNetworkId!= networkID()->cookedValueString(); }
bool MonarkSettings::encryptionKeyDirty() {return m_oldEncryptionKey!= encryptionKey()->cookedValueString(); }
bool MonarkSettings::groundTxPowerDirty() {return m_oldGroundTxPower != groundTxPower()->cookedValue().toUInt();}
bool MonarkSettings::groundFreqencyDirty(){return m_oldGroundFrequency != groundFrequency()->cookedValue().toUInt();}


void MonarkSettings::_networkIdChanged(QVariant newVal)
{
//TODO

}
void MonarkSettings::_encryptionKeyChanged(QVariant newVal)
{
//TODO


}
void MonarkSettings::_groundTxPowerChanged(QVariant newVal)
{
//TODO

}
void MonarkSettings::_groundFrequencyChanged(QVariant newVal)
{
//TODO
}
void MonarkSettings::_monarkIDChanged(QVariant newVal)
{
    //TODO
}
