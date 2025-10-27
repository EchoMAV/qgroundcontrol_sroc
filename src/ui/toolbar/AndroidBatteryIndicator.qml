

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
//-- Android Battery Indicator
Item {
    id: _root

    Timer {
        interval: 10000 // 10 seconds
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            QGroundControl.updateAndroidBatteryVoltage()
        }
    }

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: androidBatteryIndicatorRow.width

    property bool showIndicator: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Row {
        id: androidBatteryIndicatorRow
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        Repeater {
            model: _activeVehicle ? _activeVehicle.batteries : 0

            Loader {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                sourceComponent: batteryVisual

                property var battery: object

                onLoaded: {
                    QGroundControl.updateAndroidBatteryVoltage()
                }
            }
        }
    }

    Component {
        id: batteryVisual

        Row {
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            function getBatteryVoltageColor() {
                if (!isNaN(QGroundControl.androidBatteryVoltage)) {
                    if (QGroundControl.androidBatteryVoltage >= 70) {
                        return qgcPal.colorGreen
                    }
                    if (QGroundControl.androidBatteryVoltage >= 50) {
                        return qgcPal.colorYellow
                    }
                    if (QGroundControl.androidBatteryVoltage >= 20) {
                        return qgcPal.colorOrange
                    }
                }
                return qgcPal.colorRed
            }

            QGCColoredImage {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: height
                sourceSize.width: width
                source: "/qmlimages/android_device.svg"
                fillMode: Image.PreserveAspectFit
                color: getBatteryVoltageColor()
            }

            QGCLabel {
                text: QGroundControl.androidBatteryVoltage + "%"
                font.pointSize: ScreenTools.mediumFontPointSize
                color: getBatteryVoltageColor()
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
