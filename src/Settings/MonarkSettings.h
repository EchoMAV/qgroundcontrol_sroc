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
    DEFINE_SETTINGFACT(monarkID)

    bool networkIdDirty() ;
    bool encryptionKeyDirty() ;
    bool groundTxPowerDirty() ;
    bool groundFreqencyDirty() ;

    QString getOldEncryptionKey() const{ return m_oldEncryptionKey;}


public slots:
    void onSaveSettings();

private slots:
    void _networkIdChanged             (QVariant value);
    void _encryptionKeyChanged         (QVariant value);
    void _groundTxPowerChanged         (QVariant value);
    void _groundFrequencyChanged       (QVariant value);
    void _monarkIDChanged              (QVariant value);

private:
    QString m_oldNetworkId="";
    QString m_oldEncryptionKey="";
    uint32_t m_oldGroundTxPower=0;
    uint32_t m_oldGroundFrequency=0;
};
