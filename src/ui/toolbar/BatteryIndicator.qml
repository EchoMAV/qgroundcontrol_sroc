

/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
import QtQuick 2.11
import QtQuick.Layouts 1.11

import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.MultiVehicleManager 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette 1.0
import MAVLink 1.0

//-------------------------------------------------------------------------
//-- Battery Indicator
Item {
    id: _root
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: batteryIndicatorRow.width

    property bool showIndicator: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    function getEchoLinkBatteryVoltageColor() {
        if (!isNaN(QGroundControl.monarkManager.echoLinkBatteryVoltage)) {
            if (QGroundControl.monarkManager.echoLinkBatteryVoltage >= 11.1) {
                return qgcPal.colorGreen
            }
            if (QGroundControl.monarkManager.echoLinkBatteryVoltage >= 10.15) {
                return qgcPal.colorYellow
            }
        }
        return qgcPal.colorRed
    }

    Row {
        id: batteryIndicatorRow
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        Row {

            visible: QGroundControl.monarkManager.echoLinkBatteryVoltage >= 0

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            QGCColoredImage {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: height
                sourceSize.width: width
                source: "/qmlimages/battery_radio.svg"
                fillMode: Image.PreserveAspectFit
                color: getEchoLinkBatteryVoltageColor()
            }

            QGCLabel {
                text: (QGroundControl.monarkManager.echoLinkBatteryVoltage).toFixed(
                          1) + "V"
                font.pointSize: ScreenTools.mediumFontPointSize
                color: getEchoLinkBatteryVoltageColor()
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Repeater {
            model: _activeVehicle ? _activeVehicle.batteries : 0

            Loader {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                sourceComponent: batteryVisual

                property var battery: object
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, batteryPopup)
        }
    }

    Component {
        id: batteryVisual

        Row {
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            function getBatteryColor() {
                switch (battery.chargeState.rawValue) {
                case MAVLink.MAV_BATTERY_CHARGE_STATE_OK:
                    return qgcPal.text
                case MAVLink.MAV_BATTERY_CHARGE_STATE_LOW:
                    return qgcPal.colorOrange
                case MAVLink.MAV_BATTERY_CHARGE_STATE_CRITICAL:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_EMERGENCY:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_FAILED:
                case MAVLink.MAV_BATTERY_CHARGE_STATE_UNHEALTHY:
                    return qgcPal.colorRed
                default:
                    return qgcPal.text
                }
            }

            function getBatteryVoltageColor() {
                if (!isNaN(battery.voltage.rawValue)) {
                    if (battery.voltage.rawValue >= 22.81) {
                        return qgcPal.colorGreen
                    }
                    if (battery.voltage.rawValue >= 21.01) {
                        return qgcPal.colorYellow
                    }
                    if (battery.voltage.rawValue >= 19.81) {
                        return qgcPal.colorOrange
                    }
                }
                return qgcPal.colorRed
            }

            function getBatteryPercentageText() {
                if (!isNaN(battery.percentRemaining.rawValue)) {
                    if (battery.percentRemaining.rawValue > 98.9) {
                        return qsTr("100%")
                    } else {
                        return battery.percentRemaining.valueString + battery.percentRemaining.units
                    }
                } else if (!isNaN(battery.voltage.rawValue)) {
                    return battery.voltage.valueString + battery.voltage.units
                } else if (battery.chargeState.rawValue
                           !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return ""
            }

            function getBatteryVoltageText() {
                if (!isNaN(battery.voltage.rawValue)) {
                    return (battery.voltage.rawValue).toFixed(
                                1) + battery.voltage.units
                } else if (battery.chargeState.rawValue
                           !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED) {
                    return battery.chargeState.enumStringValue
                }
                return ""
            }

            QGCColoredImage {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: height
                sourceSize.width: width
                source: "/qmlimages/battery_quad.svg"
                fillMode: Image.PreserveAspectFit
                color: getBatteryVoltageColor()
            }

            QGCLabel {
                text: getBatteryVoltageText()
                font.pointSize: ScreenTools.mediumFontPointSize
                color: getBatteryVoltageColor()
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Component {
        id: batteryValuesAvailableComponent

        QtObject {
            property bool functionAvailable: battery.function.rawValue
            !== MAVLink.MAV_BATTERY_FUNCTION_UNKNOWN
            property bool temperatureAvailable: !isNaN(
                battery.temperature.rawValue)
            property bool currentAvailable: !isNaN(battery.current.rawValue)
            property bool mahConsumedAvailable: !isNaN(
                                                    battery.mahConsumed.rawValue)
            property bool timeRemainingAvailable: !isNaN(
                                                      battery.timeRemaining.rawValue)
            property bool chargeStateAvailable: battery.chargeState.rawValue
                                                !== MAVLink.MAV_BATTERY_CHARGE_STATE_UNDEFINED
        }
    }

    Component {
        id: batteryPopup

        Rectangle {
            width: mainLayout.width + mainLayout.anchors.margins * 2
            height: mainLayout.height + mainLayout.anchors.margins * 2
            radius: ScreenTools.defaultFontPixelHeight / 2
            color: qgcPal.window
            border.color: qgcPal.text

            ColumnLayout {
                id: mainLayout
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.top: parent.top
                anchors.right: parent.right
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.alignment: Qt.AlignCenter
                    text: qsTr("Battery Status")
                    font.family: ScreenTools.demiboldFontFamily
                }

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    ColumnLayout {
                        QGCLabel {
                            text: qsTr("EchoLink Voltage")
                        }
                        Repeater {
                            model: _activeVehicle ? _activeVehicle.batteries : 0

                            ColumnLayout {
                                spacing: 0

                                property var batteryValuesAvailable: nameAvailableLoader.item

                                Loader {
                                    id: nameAvailableLoader
                                    sourceComponent: batteryValuesAvailableComponent

                                    property var battery: object
                                }

                                //QGCLabel {
                                //    text: qsTr("Battery %1").arg(
                                //              object.id.rawValue)
                                //}
                                //QGCLabel {
                                //    text: qsTr("Charge State")
                                //    visible: batteryValuesAvailable.chargeStateAvailable
                                //}
                                //QGCLabel {
                                //    text: qsTr("Remaining")
                                //    visible: batteryValuesAvailable.timeRemainingAvailable
                                //}
                                //QGCLabel {
                                //    text: qsTr("Remaining")
                                //}
                                QGCLabel {
                                    text: qsTr("MONARK Voltage")
                                }
                                QGCLabel {
                                    text: qsTr("MONARK Consumed")
                                    visible: batteryValuesAvailable.mahConsumedAvailable
                                }
                                QGCLabel {
                                    text: qsTr("MONARK Temperature")
                                    visible: batteryValuesAvailable.temperatureAvailable
                                }
                                QGCLabel {
                                    text: qsTr("MONARK Function")
                                    visible: batteryValuesAvailable.functionAvailable
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        QGCLabel {
                            text: (QGroundControl.monarkManager.echoLinkBatteryVoltage).toFixed(
                                      1) + " V"
                        }
                        Repeater {
                            model: _activeVehicle ? _activeVehicle.batteries : 0

                            ColumnLayout {
                                spacing: 0

                                property var batteryValuesAvailable: valueAvailableLoader.item

                                Loader {
                                    id: valueAvailableLoader
                                    sourceComponent: batteryValuesAvailableComponent

                                    property var battery: object
                                }

                                //QGCLabel {
                                //    text: ""
                                //}
                                //QGCLabel {
                                //    text: object.chargeState.enumStringValue
                                //    visible: batteryValuesAvailable.chargeStateAvailable
                                //}
                                //QGCLabel {
                                //    text: object.timeRemainingStr.value
                                //    visible: batteryValuesAvailable.timeRemainingAvailable
                                //}
                                //QGCLabel {
                                //    text: object.percentRemaining.valueString
                                //          + " " + object.percentRemaining.units
                                //}
                                QGCLabel {
                                    text: (object.voltage.rawValue).toFixed(
                                              1) + " " + object.voltage.units
                                }
                                QGCLabel {
                                    text: object.mahConsumed.valueString + " "
                                          + object.mahConsumed.units
                                    visible: batteryValuesAvailable.mahConsumedAvailable
                                }
                                QGCLabel {
                                    text: object.temperature.valueString + " "
                                          + object.temperature.units
                                    visible: batteryValuesAvailable.temperatureAvailable
                                }
                                QGCLabel {
                                    text: object.function.enumStringValue
                                    visible: batteryValuesAvailable.functionAvailable
                                }
                                }
                                }
                                }
                                }
                                }
                                }
                                }
                                }
