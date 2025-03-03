/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Attitude Instrument
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick              2.3
import QtGraphicalEffects   1.0
import QtQuick.Layouts  1.11
import QtQuick.Dialogs      1.2


import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.SettingsManager 1.0


Item {
    id: root

    property bool showPitch:    true
    property var  vehicle:      null
    property real size
    property bool showHeading:  false  //this is now in compass widget

    property real _rollAngle:   vehicle ? vehicle.roll.rawValue  : 0
    property real _pitchAngle:  vehicle ? vehicle.pitch.rawValue : 0
    property var  _unitsSettings:     QGroundControl.settingsManager.unitsSettings



    width:  size
    height: size

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Item {
        id:             instrument
        anchors.fill:   parent
        visible:        false

        //----------------------------------------------------
        //-- Artificial Horizon
        QGCArtificialHorizon {
            rollAngle:          _rollAngle
            pitchAngle:         _pitchAngle
            anchors.fill:       parent
        }
        //----------------------------------------------------
        //-- Pointer
        Image {
            id:                 pointer
            source:             "/qmlimages/attitudePointer.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
        }
        //----------------------------------------------------
        //-- Instrument Dial
        Image {
            id:                 instrumentDial
            source:             "/qmlimages/attitudeDial.svg"
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            anchors.fill:       parent
            sourceSize.height:  parent.height
            transform: Rotation {
                origin.x:       root.width  / 2
                origin.y:       root.height / 2
                angle:          -_rollAngle
            }
        }
        //----------------------------------------------------
        //-- Pitch
        QGCPitchIndicator {
            id:                 pitchWidget
            visible:            root.showPitch
            size:               root.size * 0.5
            anchors.verticalCenter: parent.verticalCenter
            pitchAngle:         _pitchAngle
            rollAngle:          _rollAngle
            color:              Qt.rgba(0,0,0,0)
        }
        //----------------------------------------------------
        //-- Cross Hair
        Image {
            id:                 crossHair
            anchors.centerIn:   parent
            source:             "/qmlimages/crossHair.svg"
            mipmap:             true
            width:              size * 0.75
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
        }
        //left airspeed background
        Rectangle {
            id:             asBackground
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width:          root.width * .4
            height:         root.height * .3
            color:          "black"
            opacity:        0.5
            radius:         5
        }
        //right altitude background
        Rectangle {
            id:             altBackground
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            width:          root.width * .4
            height:         root.height * .3
            color:          "black"
            opacity:        0.5
            radius:         5
            visible:        true
        }
        QGCLabel {
            id: hudAltitudeUnits
            anchors.top:                altBackground.bottom
            anchors.right:               parent.right
            anchors.rightMargin:        Math.round(ScreenTools.defaultFontPixelWidth * 3)
            text:                       vehicle ? QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString : ""  //this should pull from current units, then when displayed it can use the conversion methood  QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(altitude).toFixed(0)
            color:                      "black"
            font.family:        ScreenTools.normalFontFamily
            font.pointSize:     ScreenTools.mediumFontPointSize
        }
        QGCLabel {
            id: hudAirspeedUnits
            anchors.top:                asBackground.bottom
            anchors.left:               parent.left
            anchors.leftMargin:        Math.round(ScreenTools.defaultFontPixelWidth * 3)
            text:                       vehicle ? QGroundControl.unitsConversion.appSettingsSpeedUnitsString : ""  //appSettingsSpeedUnitsString.. best I can tell there is not a method defined to convert speed from metersperSecondtoSpeedUnits
            color:                      "black"
            font.family:        ScreenTools.normalFontFamily
            font.pointSize:     ScreenTools.mediumFontPointSize
        }
    }

    Rectangle {
        id:             mask
        anchors.fill:   instrument
        radius:         width / 2
        color:          "black"
        visible:        false
    }

    OpacityMask {
        anchors.fill: instrument
        source: instrument
        maskSource: mask
    }

    Rectangle {
        id:             borderRect
        anchors.fill:   parent
        radius:         width / 2
        color:          Qt.rgba(0,0,0,0)
        border.color:   qgcPal.text
        border.width:   1
    }

    //Heading Label, bottom center
    QGCLabel {
        anchors.bottomMargin:       Math.round(ScreenTools.defaultFontPixelHeight * .75)
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        text:                       _headingString3
        color:                      "black"
        font.family:                ScreenTools.normalFontFamily
        font.pointSize:             ScreenTools.largeFontPointSize
        visible:                    showHeading

        property string _headingString: vehicle ? vehicle.heading.rawValue.toFixed(0) +"°" : "---"
        property string _headingString2: _headingString.length === 1 ? "0" + _headingString : _headingString
        property string _headingString3: _headingString2.length === 2 ? "0" + _headingString2 : _headingString2
    }

    //Ground and Airspeed, left, vertical center
    GridLayout {
        id:                 hudAirspeed
        anchors.verticalCenter:     parent.verticalCenter
        anchors.left:               parent.left
        anchors.leftMargin:         Math.round(ScreenTools.defaultFontPixelWidth * 1.5)
        rowSpacing:         0
        columns:            1
        rows:               2

        QGCLabel {
            text:               vehicle ?  vehicle.groundSpeed.value.toFixed(1) : "--.-"
            color:              "white"
            font.family:        ScreenTools.normalFontFamily
            font.pointSize:     ScreenTools.mediumFontPointSize
        }

    }
    QGCMouseArea {
        anchors.fill: hudAirspeed
        onClicked: {
            onClicked:  mainWindow.showPopupDialogFromComponent(airSpeedDialogComponent)
        }
    }


    //Altitude, right, vertical center
    QGCLabel {
        id: hudAltitude
        anchors.verticalCenter:     parent.verticalCenter
        anchors.right:               parent.right
        anchors.rightMargin:        Math.round(ScreenTools.defaultFontPixelWidth * 1.5)
        text:                       vehicle ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(vehicle.altitudeRelative.rawValue).toFixed(0) : "----"
        color:                      vehicle ? getAltColor() : "white"
        font.family:        ScreenTools.normalFontFamily
        font.pointSize:     ScreenTools.mediumFontPointSize
        function getAltColor() {
            if (!vehicle.flying)
                return "white"
            else if (vehicle.altitudeRelative.value < 15.24)
                return "red"
            else if (vehicle.altitudeRelative.value < 30.48)
                return "orange"
            else
                return "white"
        }
    }

    QGCColoredImage {
        width:                  height
        height:                 ScreenTools.defaultFontPixelWidth * 2.2
        sourceSize.height:      height
        source:                 getAirSpeedArrow()
        color:                  qgcPal.text
        anchors.verticalCenter: parent.verticalCenter
        anchors.right:               hudAltitude.left
        anchors.rightMargin:        Math.round(ScreenTools.defaultFontPixelWidth/3)
        visible:                isAscending()
        function getAirSpeedArrow()
        {
            if (vehicle)
            {
                if (vehicle.climbRate.value > 0)
                    return "/InstrumentValueIcons/arrow-thick-up.svg"
                else
                    return "/InstrumentValueIcons/arrow-thick-down.svg"
            }
            return ""
        }
        function isAscending()
        {
            if (vehicle)
            {
                if (vehicle.climbRate.value > 1.5 || vehicle.climbRate.value < -1.5)
                    return true
                else
                    return false
            }
            return false
        }
    }

    Component {
        id: airSpeedDialogComponent

        QGCPopupDialog {
            title:      qsTr("Change Target Airspeed")
            buttons:    StandardButton.Close
            ColumnLayout {
                id: airSpeedCol
                Layout.fillWidth:   true
                GridLayout {
                    columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                    columns: 3
                    QGCLabel {
                        text:           qsTr("Target Airspeed:")
                    }
                    QGCTextField {
                        id:             targetAirspeed
                        inputMethodHints:   Qt.ImhDigitsOnly
                        text:           vehicle ? getCurrentAirSpeed() : "";
                        enabled:        true

                        function getCurrentAirSpeed()
                        {
                            console.log("target airspeed is " + vehicle.targetAirSpeedSetPoint.value);
                            return QGroundControl.unitsConversion.metersPerSecondToAppSettingsSpeedUnits(vehicle.targetAirSpeedSetPoint.value).toFixed(0)
                        }
                    }
                    QGCLabel {
                        text:           QGroundControl.unitsConversion.appSettingsSpeedUnitsString
                        enabled:        true
                    }

                }

                Item {
                    width:  1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * .5)
                }
                QGCSlider {
                    Layout.fillWidth:           true
                    Layout.alignment:           Qt.AlignHCenter
                    maximumValue:               QGroundControl.unitsConversion.metersPerSecondToAppSettingsSpeedUnits(33.5).toFixed(0)
                    minimumValue:               QGroundControl.unitsConversion.metersPerSecondToAppSettingsSpeedUnits(22.35).toFixed(0)
                    value:                      vehicle ? QGroundControl.unitsConversion.metersPerSecondToAppSettingsSpeedUnits(vehicle.targetAirSpeedSetPoint.value).toFixed(0) : 0
                    updateValueWhileDragging:   true
                    visible:                    true
                    onValueChanged:             targetAirspeed.text = value.toFixed(0)

                }
                Item {
                    width:  1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * .5)
                }
                GridLayout {
                    Layout.fillWidth:   true
                    Layout.alignment:   Qt.AlignHCenter
                    columnSpacing:      ScreenTools.defaultFontPixelWidth * 2
                    columns: 2

                    QGCButton {
                        text:               qsTr("Cancel")
                        onClicked: {
                            hideDialog()
                        }
                    }
                    QGCButton {
                        function testEnabled() {
                            if (vehicle)
                                {
                                var temp = QGroundControl.unitsConversion.appSettingsSpeedUnitsToMetersPerSecond(targetAirspeed.text);       //convert to m/s first
                                if(temp  >= 20 && temp <= 33.6)
                                     return true
                                return false
                            }
                            return false
                        }
                        enabled:            testEnabled()
                        text:               qsTr("Apply")
                        onClicked: {
                            console.log("sending speed " + QGroundControl.unitsConversion.appSettingsSpeedUnitsToMetersPerSecond(targetAirspeed.text));
                            vehicle.setAirSpeed(QGroundControl.unitsConversion.appSettingsSpeedUnitsToMetersPerSecond(targetAirspeed.text));  //this should be sent in m/s
                            hideDialog();
                        }
                    }
                }
            }
        }
    }
}
