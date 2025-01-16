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
import QtQuick.Controls         2.12
import QGroundControl.ScreenTools   1.0
import QtQuick.Layouts  1.11
import QtQuick.Dialogs      1.2
import QGroundControl.Palette       1.0
import QGroundControl.SettingsManager 1.0

PreFlightCheckButton {
    name:               qsTr("Quad Motor Run Up")
    manualText:         qsTr("A run up of the electric quadrotor motors is recommend to ensure proper operation prior to flight.")

    bottomPadding: Math.round(ScreenTools.defaultFontPixelHeight / 2) + motorTestButton.height + Math.round(ScreenTools.defaultFontPixelHeight / 2)
    property bool   allowFailurePercentOverride:    false
    property string   _buttonLabel:   qsTr("Quad Motor Run Up")
    property int    _motorTestThrottle: 20  //percent throttle to use for motor runup
    property int    _motorTestDurationSec: 2  //seconds for the motor test to last
    property int    _testDelayDurationSec: 4  //seconds to delay between each motor test message

    property bool  cancelTest: false  //flag to keep track of the user requesting a test cancel

    Button {
        id:             motorTestButton
        text:           _buttonLabel
        visible:        true
        enabled:        true
        onClicked:      startMotorTest()
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: ScreenTools.defaultFontPixelWidth * 3
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Math.round(ScreenTools.defaultFontPixelHeight / 2)

        function startMotorTest()
        {
            preFlightChecklistWindow.showPopupDialogFromComponent(motorTestComponent)
        }
    }

    Component {
        id: motorTestComponent
        QGCPopupDialog {
            title:      qsTr("Quad Motor Run Up")
            buttons:    StandardButton.Close
            ColumnLayout {
                id: motorTestCol
                Layout.fillWidth:   true
                Layout.fillHeight:   true

                RowLayout{
                    Layout.fillWidth:           true
                    Layout.fillHeight:          true
                    spacing: ScreenTools.defaultFontPixelWidth * 4
                    Column
                    {
                        width: ScreenTools.defaultFontPixelWidth * 60
                        QGCLabel {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width:          ScreenTools.defaultFontPixelWidth * 60
                            height:         ScreenTools.defaultFontPixelHeight * 4
                            wrapMode:       Text.WordWrap
                            text:           qsTr("WARNING: Running up the quad motors is dangerous! Ensure the props are clear, then push and hold the button below. The test will spin each motor sequentially for 3 seconds each. Watch each motor and ensure it spins normally.")
                        }
                        Item {
                            width:  1
                            height: Math.round(ScreenTools.defaultFontPixelHeight * 1)
                        }
                        Timer {
                            id: timer1
                            function setTimeout(cb, delayTime) {
                                timer1.interval = delayTime;
                                timer1.repeat = false;
                                timer1.triggered.connect(cb);
                                timer1.triggered.connect(function release () {
                                    timer1.triggered.disconnect(cb);
                                    timer1.triggered.disconnect(release);
                                    //if not cancelled
                                    if (!cancelTest)
                                    {
                                        sliderMotorTest.startMotorTest(2);
                                        timer2.setTimeout(function(){}, _testDelayDurationSec * 1000);
                                    }

                                });
                                timer1.start();
                            }
                        }
                        Timer {
                            id: timer2
                            function setTimeout(cb, delayTime) {
                                timer2.interval = delayTime;
                                timer2.repeat = false;
                                timer2.triggered.connect(cb);
                                timer2.triggered.connect(function release () {
                                    timer2.triggered.disconnect(cb);
                                    timer2.triggered.disconnect(release);
                                    //if not cancelled
                                    if (!cancelTest)
                                    {
                                        sliderMotorTest.startMotorTest(3);
                                        timer3.setTimeout(function(){}, _testDelayDurationSec * 1000);
                                    }
                                });
                                timer2.start();
                            }
                        }
                        Timer {
                            id: timer3
                            function setTimeout(cb, delayTime) {
                                timer3.interval = delayTime;
                                timer3.repeat = false;
                                timer3.triggered.connect(cb);
                                timer3.triggered.connect(function release () {
                                    timer3.triggered.disconnect(cb);
                                    timer3.triggered.disconnect(release);

                                    //if not cancelled
                                    if (!cancelTest)
                                    {
                                        sliderMotorTest.startMotorTest(4);
                                        timer4.setTimeout(function(){}, _testDelayDurationSec * 1000);
                                    }

                                });
                                timer3.start();
                            }
                        }
                        Timer {
                            id: timer4
                            function setTimeout(cb, delayTime) {
                                timer4.interval = delayTime;
                                timer4.repeat = false;
                                timer4.triggered.connect(cb);
                                timer4.triggered.connect(function release () {
                                    timer4.triggered.disconnect(cb);
                                    timer4.triggered.disconnect(release);
                                    sliderMotorTest.finishMotorTest();
                                   });
                                timer4.start();
                            }
                        }

                        SliderSwitch {
                            id:                     sliderMotorTest
                            confirmText:            qsTr("Slide to Start Motor Test")
                            Layout.minimumWidth:    Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 30)
                            anchors.horizontalCenter: parent.horizontalCenter

                            onAccept: {
                                visible = false
                                cancelTest = false
                                startMotorTest(1)
                                timer1.setTimeout(function(){}, _testDelayDurationSec * 1000);
                            }
                            function startMotorTest(motor)
                            {
                                motorTestStatus.text= qsTr("Test Running! (Motor "+ motor + ")")

                                console.log("Starting motor test for motor " + motor);
                                globals.activeVehicle.motorTest(motor, _motorTestThrottle, _motorTestDurationSec, true)
                            }
                            function finishMotorTest()
                            {
                                cancelTest = false
                                motorTestStatus.text =""
                                visible = true
                                _manualState = _statePassed  //mark check as passed
                            }
                        }
/*
                        DelayButton {
                            id: multiDelayButton
                            anchors.horizontalCenter: parent.horizontalCenter
                            text:               qsTr("Push and Hold To Start Motor Test")
                            palette {
                                   button: "red"
                               }
                            delay: 1500
                            onActivated: {
                                visible = false
                                cancelTest = false
                                startMotorTest(1)
                                timer1.setTimeout(function(){}, _testDelayDurationSec * 1000);
                            }

                            function startMotorTest(motor)
                            {
                                motorTestStatus.text= qsTr("Test Running! (Motor "+ motor + ")")

                                console.log("Starting motor test for motor " + motor);
                                globals.activeVehicle.motorTest(motor, _motorTestThrottle, _motorTestDurationSec, true)
                            }
                            function finishMotorTest()
                            {
                                cancelTest = false
                                motorTestStatus.text =""
                                visible = true
                                _manualState = _statePassed  //mark check as passed
                            }
                        }
                        */
                        QGCButton {
                            text:               qsTr("Cancel the test")
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: !sliderMotorTest.visible
                            onClicked: {
                                cancelTest = true;
                                motorTestStatus.text =""
                                sliderMotorTest.visible = true
                               }
                        }
                        Item {
                            width:  1
                            height: Math.round(ScreenTools.defaultFontPixelHeight * .5)
                        }
                        QGCLabel {
                            id: motorTestStatus
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pointSize:     ScreenTools.largeFontPointSize
                            text:           qsTr("")
                        }
                    }
                }
                Item {
                    width:  1
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 1)
                }
                RowLayout
                {
                    Layout.fillWidth:           true
                    Layout.alignment:   Qt.AlignHCenter
                    QGCButton {
                        text:               qsTr("Close")
                        onClicked: {
                            hideDialog()
                        }
                    }
                }
            }
            function reject() {
                hideDialog()
            }
        }
    }
}
