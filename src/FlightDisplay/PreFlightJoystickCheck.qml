/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl           1.0
import QGroundControl.Controls  1.0
import QGroundControl.Vehicle   1.0
import QGroundControl.FactSystem      1.0
import QGroundControl.FactControls    1.0
import QtQuick.Controls         2.12
import QGroundControl.ScreenTools   1.0

PreFlightCheckButton {
    name:               qsTr("Joystick Check")
    telemetryFailure:   joysticksDisabled() //this causes the button to go yellow/red
    allowTelemetryFailureOverride: false

    bottomPadding: getPadding()
    property var    _activeVehicle:         globals.activeVehicle
    property var    _planMasterController:  globals.planMasterControllerFlyView
    property var    _missionController:     _planMasterController.missionController

    Row{
        id: spacer
        anchors.horizontalCenter: parent.horizontalCenter
        Item {
            width:  1
            height: Math.round(ScreenTools.defaultFontPixelHeight * 1)
        }
    }

    function getPadding()
    {
        if (joysticksDisabled())
            return (Math.round(ScreenTools.defaultFontPixelHeight / 2) + Math.round(ScreenTools.defaultFontPixelHeight / 2))
        else
            return Math.round(ScreenTools.defaultFontPixelHeight / 2)
    }

    function joysticksDisabled()
    {
        if (!_activeVehicle)
            return true

        if (joystickManager.activeJoystick && _activeVehicle.joystickEnabled)
            return false
        else
            return true
    }

    Component.onCompleted: updateTelemetryTextFailure()

    function updateTelemetryTextFailure() {
        if(joysticksDisabled()) {
            telemetryTextFailure = qsTr("No Joystick found. Please use a Joystick to operate the drone.")
            return
         }
    }
}

