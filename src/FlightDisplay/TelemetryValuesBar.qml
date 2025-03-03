

/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
import QtQuick 2.12
import QtQuick.Layouts 1.12

import QGroundControl 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Vehicle 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controllers 1.0

Rectangle {
    id: telemetryPanel
    height: telemetryLayout.height + (_toolsMargin * 2)
    width: telemetryLayout.width + (_toolsMargin * 2)
    color: qgcPal.window
    opacity: 0.95
    radius: ScreenTools.defaultFontPixelWidth / 2

    property bool bottomMode: true

    GPSUnitsController {
        id: gpsUnitsController
    }

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _gcsPosition: QGroundControl.qgcPositionManger.gcsPosition
    property string _activeVehicleMGRS: _activeVehicle
                                        && _activeVehicle.coordinate.isValid ? gpsUnitsController.convertToMGRS(_activeVehicle.coordinate) : "---------"

    property string _gcsMGRS: _gcsPosition.isValid ? gpsUnitsController.convertToMGRS(
                                                         _gcsPosition) : "---------"

    function toggleDetails() {
        if (!detailGrid.visible)
            detailGrid.visible = true
        else
            detailGrid.visible = false
    }

    ColumnLayout {
        id: telemetryLayout
        anchors.margins: _toolsMargin
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        // Details Panel (vehicle lat/lon, cursor lat/lon)
        GridLayout {

            id: detailGrid
            visible: false
            Layout.fillWidth: true
            rowSpacing: ScreenTools.defaultFontPixelHeight
            columnSpacing: ScreenTools.defaultFontPixelWidth * 2
            rows: 2
            columns: 3
            Layout.bottomMargin: ScreenTools.defaultFontPixelHeight * .5
            QGCLabel {
                text: qsTr("Vehicle: ")
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                Layout.fillWidth: false
            }
            QGCLabel {
                text: _activeVehicleMGRS
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                Layout.fillWidth: false
            }
            QGCColoredImage {
                Layout.alignment: Qt.AlignLeft
                source: "/res/content_copy.svg"
                mipmap: true
                width: ScreenTools.defaultFontPixelHeight
                height: width
                sourceSize.width: width
                color: qgcPal.text
                fillMode: Image.PreserveAspectFit
                QGCMouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        textEdit3.text = _activeVehicleMGRS
                        textEdit3.selectAll()
                        textEdit3.copy()
                    }
                    TextEdit {
                        id: textEdit3
                        visible: false
                    }
                }
            }
            QGCLabel {
                text: qsTr("GCS: ")
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                Layout.fillWidth: false
            }
            QGCLabel {
                text: _gcsMGRS
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                Layout.fillWidth: false
            }
            QGCColoredImage {
                Layout.alignment: Qt.AlignLeft
                source: "/res/content_copy.svg"
                mipmap: true
                width: ScreenTools.defaultFontPixelHeight
                height: width
                sourceSize.width: width
                color: qgcPal.text
                fillMode: Image.PreserveAspectFit
                QGCMouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        textEdit4.text = _gcsMGRS
                        textEdit4.selectAll()
                        textEdit4.copy()
                    }
                    TextEdit {
                        id: textEdit4
                        visible: false
                    }
                }
            }
        }

        GridLayout {

            id: superVoloTelemGrid
            visible: true
            columns: 2
            rows: 1
            rowSpacing: ScreenTools.defaultFontPixelWidth
            columnSpacing: ScreenTools.defaultFontPixelWidth * 2
            HorizontalFactValueGrid {
                id: valueArea
                //userSettingsGroup: telemetryBarUserSettingsGroup
                defaultSettingsGroup: telemetryBarDefaultSettingsGroup
            }
            RowLayout {
                QGCColoredImage {
                    source: "/res/menu.svg"
                    mipmap: true
                    width: ScreenTools.defaultFontPixelHeight
                    height: width
                    sourceSize.width: width
                    color: qgcPal.text
                    fillMode: Image.PreserveAspectFit
                    QGCMouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            toggleDetails()
                        }
                    }
                }
            }
        }
    }
}
