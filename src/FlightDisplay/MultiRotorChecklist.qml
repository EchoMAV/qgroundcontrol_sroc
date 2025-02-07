/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQml.Models                 2.1
import QtQuick.Layouts              1.12


import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Vehicle       1.0

//going to do stuff for monark
Item {
    property var model: listModel
    PreFlightCheckModel {
        id:     listModel
        PreFlightCheckGroup {
            name: qsTr("Multirotor Initial Checks")
            PreFlightJoystickCheck {
                allowTelemetryFailureOverride:    !QGroundControl.settingsManager.appSettings.enforceJoystickRequired.value
            }



            PreFlightCheckButton {
                name:           qsTr("Folding Arms")
                manualText:     qsTr("Open Arms until fully locked.")

                need_pic:       true
                pic_name:       "/qmlimages/folding_arms.png"
            }

            PreFlightBatteryCheck {
                failurePercent:                 40
                allowFailurePercentOverride:    false

                need_pic: true
                pic_name: "/qmlimages/battery_latch.png"
            }

            PreFlightCheckButton {
                name:           qsTr("Antenna Orientation")
                manualText:     qsTr("Orientate antennas up 90deg.")
            }

            PreFlightSensorsHealthCheck {
            }

            PreFlightGPSCheck {
                failureSatCount:        9
                allowOverrideSatCount:  true
            }

            PreFlightCheckButton {
                name:           qsTr("Propellers")
                manualText:     qsTr("unfolded Propellers")

                need_pic: true
                pic_name: "/qmlimages/folding_props.png"
            }

            PreFlightMultiRotorHealthCheck {
                need_pic: true
                pic_name: "/qmlimages/motor_test.png"
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Please arm the vehicle here")

            PreFlightCheckButton {
                name:            qsTr("Motors")
                manualText:      qsTr("Propellers free? Then throttle up gently. Working properly?")
            }

            PreFlightCheckButton {
                name:           qsTr("Mission")
                manualText:     qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")
            }

            PreFlightSoundCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Last preparations before launch")

            // Check list item group 2 - Final checks before launch
            PreFlightCheckButton {
                name:           qsTr("Payload")
                manualText:     qsTr("Is Video Feed Operational?")
            }

            PreFlightCheckButton {
                name:           qsTr("Wind & weather")
                manualText:     qsTr("Under 30mph winds? Raining?")
            }

            PreFlightCheckButton {
                name:           qsTr("Flight area")
                manualText:     qsTr("Launch area and path free of obstacles/people?")
            }
        }
    }
}

