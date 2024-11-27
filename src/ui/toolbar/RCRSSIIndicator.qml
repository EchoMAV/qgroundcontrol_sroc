/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.SettingsManager       1.0


//-------------------------------------------------------------------------
//-- RC RSSI Indicator
Item {
    id:             _root
    width:          rssiRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: _activeVehicle.supportsRadio

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rcRSSIAvailable:   true //activeVehicle ? activeVehicle.mavlinkLossPercent >= 0 && activeVehicle.mavlinkLossPercent <= 100 : false
    property var    _rssiSource:    QGroundControl.settingsManager.appSettings.rssiRadioSelect.rawValue

    function getRssiMaxColor() {
        if(_activeVehicle.RadioRSSI.length > 0) {
            if(_activeVehicle.RadioRSSIMax > 50) {
                return qgcPal.text
            }
            if(_activeVehicle.RadioRSSIMax > 35) {
                return qgcPal.colorOrange
            }
            if(_activeVehicle.RadioRSSIMax >= 0) {
                return qgcPal.colorRed
            }

        }
        return qgcPal.text
    }

    function getRssiToolbarText() {

        if (!_activeVehicle)
            return "--"

        switch (_rssiSource)
        {
            case 0: // packet
                return (100 - _activeVehicle.mavlinkLossPercent.toFixed(0)) + "%"
            case 1:  //mpu5
            case 2:  //doodle
                if (_activeVehicle.RadioRSSI.length > 1)  //a mesh detectected, return worst case neightbor signal
                    return _activeVehicle.RadioRSSIMin + "% (Mesh)"
                else if (_activeVehicle.RadioRSSIMax != -255) //single Radio detected, return max which will be the single value in this case
                    return _activeVehicle.RadioRSSIMax + "%"
                else
                    return "--"
        }
    }

    function getDetailHeader()
    {
        if (!_activeVehicle)
            return "N/A, No data available";
        switch (_rssiSource)
        {
            case 0: // packet
                return "Mavlink Packet Loss Ratio"
            case 1: //mpu5
                return "MPU5 RSSI Status"
            case 2: //doodle
                return "Doodle RSSI Status"
            default:
                return "Unknown"
        }
    }

    Component {
        id: rcRSSIInfo

        Rectangle {
            width:  rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           getDetailHeader()
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Row {
                    id: mavPacket
                    visible: (_rssiSource === 0) ? true : false
                    QGCLabel { text: _activeVehicle ? (100 - _activeVehicle.mavlinkLossPercent.toFixed(0) + "% (" + _activeVehicle.mavlinkLossPercent.toFixed(0) + "% Packet Loss)") : 0 }
                }

                // MPU5 Single Radio Row with no data
                Row {
                    id: singleRssiMPU5NoData
                    visible: (_activeVehicle.RadioRSSI.length === 0 && _rssiSource === 1) ? true : false
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 0) ? qsTr("Error: No Data Available, Check Settings") : "" }
                }
                // MPU5 Single Radio Row
                Row {
                    id: singleRssiMPU5
                    visible: (_activeVehicle.RadioRSSI.length === 1 && _rssiSource === 1) ? true : false
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? _activeVehicle.RadioRSSI[0].qmac + ", " + _activeVehicle.RadioRSSI[0].qip + ", " : "" }
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? _activeVehicle.RadioRSSI[0].qsignal + " dBm " : ""}
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? "(" +_activeVehicle.RadioRSSI[0].qpercentage + "%)" : "" }

                }
                // MPU5 Single Radio Row with no data
                Row {
                    id: singleRssiDoodleNoData
                    visible: (_activeVehicle.RadioRSSI.length === 0 && _rssiSource === 2) ? true : false
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 0) ? qsTr("Error: No Data Available, Check Settings") : "" }
                }
                // Doodle Single Radio Row (doodle does not provide IP)
                Row {
                    id: singleRssiDoodle
                    visible: (_activeVehicle.RadioRSSI.length === 1 && _rssiSource === 2) ? true : false
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? _activeVehicle.RadioRSSI[0].qmac + ", " : "" }
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? _activeVehicle.RadioRSSI[0].qsignal + " dBm " : ""}
                    QGCLabel { text : (_activeVehicle.RadioRSSI.length === 1) ? "(" +_activeVehicle.RadioRSSI[0].qpercentage + "%)" : "" }

                }

                //MPU5 Multi Radio
                Repeater {
                    id:     rssiListMPU5
                    model:  _activeVehicle.RadioRSSI
                    Row {
                        visible: (_activeVehicle.RadioRSSI.length > 1 && _rssiSource === 1) ? true : false
                        QGCLabel { text : modelData.qip + ", " }
                        QGCLabel { text : "(" + modelData.qmac + "), " }
                        QGCLabel { text : modelData.qsignal + " dBm " }
                        QGCLabel { text : "(" + modelData.qpercentage + "%)" }
                    }
                }
                //Doodle Multi Radio
                Repeater {
                    id:     rssiListDoodle
                    model:  _activeVehicle.RadioRSSI
                    Row {
                        visible: (_activeVehicle.RadioRSSI.length > 1 && _rssiSource === 2) ? true : false
                        QGCLabel { text : modelData.qmac + ", " }
                        QGCLabel { text : modelData.qsignal + " dBm " }
                        QGCLabel { text : "(" + modelData.qpercentage + "%)" }
                    }
                }
            }
        }
    }

    Row {
        id:             rssiRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth

        /*QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/RC.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _rcRSSIAvailable ? 1 : 0.5
            color:              qgcPal.buttonText
        }
        */

        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   parent.height * 0.5
            percent:                _activeVehicle ? ((_activeVehicle.RadioRSSIMin != 255) ? _activeVehicle.RadioRSSIMin : (100 - _activeVehicle.mavlinkLossPercent.toFixed(0))) : 0

        }
        QGCLabel {
            id:             rssiLabel2
            text:           getRssiToolbarText()
            color:          getRssiMaxColor()
            font.pointSize:         ScreenTools.mediumFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }


    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, rcRSSIInfo)
        }
    }
}
