#include "HerelinkCorePlugin.h"

#include "AutoConnectSettings.h"
#include "VideoSettings.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "MultiVehicleManager.h"
#include "JoystickManager.h"
#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"

#include <list>

QGC_LOGGING_CATEGORY(HerelinkCorePluginLog, "HerelinkCorePluginLog")

HerelinkCorePlugin::HerelinkCorePlugin(QGCApplication *app, QGCToolbox* toolbox)
    : QGCCorePlugin(app, toolbox)
{

}

void HerelinkCorePlugin::setToolbox(QGCToolbox* toolbox)
{
    QGCCorePlugin::setToolbox(toolbox);

    _herelinkOptions = new HerelinkOptions(this, nullptr);

    auto multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &HerelinkCorePlugin::_activeVehicleChanged);
}

bool HerelinkCorePlugin::overrideSettingsGroupVisibility(QString name)
{
    // Hide all AutoConnect settings
    return name != AutoConnectSettings::name;
}

bool HerelinkCorePlugin::adjustSettingMetaData(const QString& settingsGroup, FactMetaData& metaData)
{
    if (settingsGroup == AppSettings::settingsGroup) {
        //-- Default herelink fontsize of 10, it is a nice starting point
        if (metaData.name() == AppSettings::appFontPointSizeName) {
            uint32_t fontSize = 10;
            metaData.setRawDefaultValue(fontSize);
            // Show setting in ui
            return true;
        }
        //-- Default Palette Dark
        if (metaData.name() == AppSettings::indoorPaletteName) {
            QVariant outdoorPalette;
            outdoorPalette = 1;
            metaData.setRawDefaultValue(outdoorPalette);
            // Show setting in ui
            return true;
        }
    }
    if (settingsGroup == AutoConnectSettings::settingsGroup) {
        // We have to adjust the Herelink UDP autoconnect settings for the AirLink
        if (metaData.name() == AutoConnectSettings::udpListenPortName) {
            metaData.setRawDefaultValue(14551);
        } else if (metaData.name() == AutoConnectSettings::udpTargetHostIPName) {
            metaData.setRawDefaultValue(QStringLiteral("127.0.0.1"));
        } else if (metaData.name() == AutoConnectSettings::udpTargetHostPortName) {
            metaData.setRawDefaultValue(15552);
        } else {
            // Disable all the other autoconnect types
            const std::list<const char *> disabledAndHiddenSettings = {
                AutoConnectSettings::autoConnectPixhawkName,
                AutoConnectSettings::autoConnectSiKRadioName,
                AutoConnectSettings::autoConnectPX4FlowName,
                AutoConnectSettings::autoConnectRTKGPSName,
                AutoConnectSettings::autoConnectLibrePilotName,
                AutoConnectSettings::autoConnectNmeaPortName,
                AutoConnectSettings::autoConnectZeroConfName,
            };
            for (const char * disabledAndHiddenSetting : disabledAndHiddenSettings) {
                if (disabledAndHiddenSetting == metaData.name()) {
                    metaData.setRawDefaultValue(false);
                }
            }
        }
    } else if (settingsGroup == VideoSettings::settingsGroup) {
        if (metaData.name() == VideoSettings::rtspTimeoutName) {
            metaData.setRawDefaultValue(60);
        } else if (metaData.name() == VideoSettings::videoSourceName) {
            metaData.setRawDefaultValue(VideoSettings::videoSourceHerelinkAirUnit);
        }
    } else if (settingsGroup == AppSettings::settingsGroup) {
        if (metaData.name() == AppSettings::androidSaveToSDCardName) {
            metaData.setRawDefaultValue(true);
        }
    }

    return true; // Show all settings in ui
}

void HerelinkCorePlugin::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (activeVehicle) {
        QString herelinkButtonsJoystickName("gpio-keys");

        auto joystickManager = qgcApp()->toolbox()->joystickManager();
        if (joystickManager->activeJoystickName() != herelinkButtonsJoystickName) {
            if (!joystickManager->setActiveJoystickName(herelinkButtonsJoystickName)) {
                //qgcApp()->showAppMessage("Warning: Herelink buttton setup failed. Buttons will not work.");
                return;
            }
        }
        activeVehicle->setJoystickEnabled(true);
    }
}

// Same as original, only we set font size to medium by default for Herelink
void HerelinkCorePlugin::factValueGridCreateDefaultSettings(const QString& defaultSettingsGroup)
{
    HorizontalFactValueGrid factValueGrid(defaultSettingsGroup);


    factValueGrid.setFontSize(FactValueGrid::MediumFontSize);
#if 1
    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    int columnIndex=0;
    {
        QmlObjectListModel* column      = factValueGrid.columns()->value<QmlObjectListModel*>(columnIndex++);
        InstrumentValueData* value = column->value<InstrumentValueData*>(0);
        value->setFact("Vehicle", "FlightTime");
        value->setIcon("timer.svg");
        value->setText(value->fact()->shortDescription());
        value->setShowUnits(false);
    }
    {
        QmlObjectListModel* column      = factValueGrid.columns()->value<QmlObjectListModel*>(columnIndex++);
        InstrumentValueData* value = column->value<InstrumentValueData*>(0);
        value->setFact("Vehicle", "DistanceToHome");
        value->setIcon("drone_distance.svg");
        value->setText(value->fact()->shortDescription());
        value->setShowUnits(true);
    }
    {
        QmlObjectListModel* column      = factValueGrid.columns()->value<QmlObjectListModel*>(columnIndex++);
        InstrumentValueData* value = column->value<InstrumentValueData*>(0);
        value->setFact("Vehicle", "AltitudeRelative");
        value->setIcon("arrow-thick-up.svg");
        value->setText(value->fact()->shortDescription());
        value->setShowUnits(true);
    }
#else
    bool        includeFWValues = factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassFixedWing || factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassVTOL || factValueGrid.vehicleClass() == QGCMAVLink::VehicleClassAirship;
    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    factValueGrid.appendColumn();
    if (includeFWValues) {
        factValueGrid.appendColumn();
    }
    factValueGrid.appendRow();

    int                 rowIndex    = 0;
    QmlObjectListModel* column      = factValueGrid.columns()->value<QmlObjectListModel*>(0);

    InstrumentValueData* value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "AltitudeRelative");
    value->setIcon("arrow-thick-up.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "DistanceToHome");
    value->setIcon("bookmark copy 3.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    rowIndex    = 0;
    column      = factValueGrid.columns()->value<QmlObjectListModel*>(1);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "ClimbRate");
    value->setIcon("arrow-simple-up.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "GroundSpeed");
    value->setIcon("arrow-simple-right.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);


    if (includeFWValues) {
        rowIndex    = 0;
        column      = factValueGrid.columns()->value<QmlObjectListModel*>(2);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact("Vehicle", "AirSpeed");
        value->setText("AirSpd");
        value->setShowUnits(true);

        value = column->value<InstrumentValueData*>(rowIndex++);
        value->setFact("Vehicle", "ThrottlePct");
        value->setText("Thr");
        value->setShowUnits(true);
    }

    rowIndex    = 0;
    column      = factValueGrid.columns()->value<QmlObjectListModel*>(includeFWValues ? 3 : 2);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "FlightTime");
    value->setIcon("timer.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(false);

    value = column->value<InstrumentValueData*>(rowIndex++);
    value->setFact("Vehicle", "FlightDistance");
    value->setIcon("travel-walk.svg");
    value->setText(value->fact()->shortDescription());
    value->setShowUnits(true);
#endif
}
