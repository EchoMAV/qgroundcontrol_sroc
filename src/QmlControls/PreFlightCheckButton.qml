/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         2.5
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts              1.12
import QtQuick.Window 2.15


import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

/// The PreFlightCheckButton supports creating a button which the user then has to verify/click to confirm a check.
/// It also supports failing the check based on values from within the system: telemetry or QGC app values. These
/// controls are normally placed within a PreFlightCheckGroup.
///
/// Two types of checks may be included on the button:
///     Manual - This is simply a check which the user must verify and confirm. It is not based on any system state.
///     Telemetry - This type of check can fail due to some state within the system. A telemetry check failure can be
///                 a hard stop in that there is no way to pass the checklist until the system state resolves itself.
///                 Or it can also optionally be override by the user.
/// If a button uses both manual and telemetry checks, the telemetry check takes precendence and must be passed first.
QGCButton {
    property string name:                           ""
    property string manualText:                     ""      ///< text to show for a manual check, "" signals no manual check

    property bool need_pic:                       faLse
    property string pic_name:                       ""

    property string telemetryTextFailure                    ///< text to show if telemetry check failed (override not allowed)
    property bool   telemetryFailure:               false   ///< true: telemetry check failing, false: telemetry check passing
    property bool   allowTelemetryFailureOverride:  false   ///< true: user can click past telemetry failure
    property bool   passed:                         _manualState === _statePassed && _telemetryState === _statePassed
    property bool   failed:                         _manualState === _stateFailed || _telemetryState === _stateFailed
    property bool reference_pic_visible: false

    property int _manualState:          manualText === "" ? _statePassed : _statePending
    property int _telemetryState:       _statePassed
    property int _horizontalPadding:    ScreenTools.defaultFontPixelWidth
    property int _verticalPadding:      Math.round(ScreenTools.defaultFontPixelHeight / 2)
    property real _stateFlagWidth:      ScreenTools.defaultFontPixelWidth * 5

    readonly property int _statePending:    0   ///< Telemetry check is failing or manual check not yet verified, user can click to make it pass
    readonly property int _stateFailed:     1   ///< Telemetry check is failing, user cannot click to make it pass
    readonly property int _statePassed:     2   ///< Check has passed

    readonly property color _passedColor:   "#86cc6a"
    readonly property color _pendingColor:  "#f7a81f"
    readonly property color _failedColor:   "#c31818"

    property string _text: "<b>" + name +"</b>: " +
                           ((_telemetryState !== _statePassed) ?
                               telemetryTextFailure :
                               (_manualState !== _statePassed ? manualText : qsTr("Passed")))
    property color  _color: _telemetryState === _statePassed && _manualState === _statePassed ?
                                _passedColor :
                                (_telemetryState == _stateFailed ?
                                     _failedColor :
                                     (_telemetryState === _statePending || _manualState === _statePending ?
                                          _pendingColor :
                                          _failedColor))

    width:          40 * ScreenTools.defaultFontPixelWidth
    topPadding:     _verticalPadding
    bottomPadding:  _verticalPadding
    leftPadding:    (_horizontalPadding * 2) + _stateFlagWidth
    rightPadding:   _horizontalPadding

    Dialog {
        id: referenceImageDialog
        modal: true
        // standardButtons: Dialog.Ok

        contentItem: Column {
            // spacing: 10
            // anchors.margins: 10

            Image {
                source: pic_name
                // Helps keep the aspect ratio
                fillMode: Image.PreserveAspectFit
                // Example size; adjust as needed
                // width: 300
                // height: 300
                width: Screen.width * 0.1
                height: Screen.height * 0.1
            }
        }
    }



    background: Rectangle {
        color:          qgcPal.button
        border.color:   qgcPal.button;

        Rectangle {
            color:          _color
            anchors.left:   parent.left
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _stateFlagWidth

            Text {
                visible: need_pic
                anchors.centerIn: parent
                text: "?"
                font.pixelSize: 32
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("I got clicked")
                    // if (reference_pic_visible == true)
                    //     reference_pic_visible = false
                    // else
                    //     reference_pic_visible = true
                    if (need_pic) {
                        referenceImageDialog.open()
                    }
                }
            }
        }
        // Image {
        //     id: reference_pic
        //     source: "/qmlimages/folding_arms.png"
        //     visible: reference_pic_visible

        //     anchors.right: parent.right
        //     // Optionally center it vertically within the parent
        //     anchors.verticalCenter: parent.verticalCenter
        //     // You can also add some right margin if you want a gap
        //     anchors.rightMargin: 10
        // }
    }




    contentItem: QGCLabel {
        wrapMode:               Text.WordWrap
        horizontalAlignment:    Text.AlignHCenter
        color:                  qgcPal.buttonText
        text:                   _text
    }

    function _updateTelemetryState() {
        if (telemetryFailure) {
            // We have a new telemetry failure, reset user pass
            _telemetryState = allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

    onTelemetryFailureChanged:              _updateTelemetryState()
    onAllowTelemetryFailureOverrideChanged: _updateTelemetryState()

    onClicked: {
        if (telemetryFailure && !allowTelemetryFailureOverride) {
            // No way to proceed past this failure
            return
        }
        if (telemetryFailure && allowTelemetryFailureOverride && _telemetryState !== _statePassed) {
            // User is allowed to proceed past this failure
            _telemetryState = _statePassed
            return
        }
        if (manualText !== "" && _manualState !== _statePassed) {
            // User is confirming a manual check
            _manualState = _statePassed
        }
    }

    onPassedChanged: callButtonPassedChanged()
    onParentChanged: callButtonPassedChanged()

    function callButtonPassedChanged() {
        if (typeof parent.buttonPassedChanged === "function") {
            parent.buttonPassedChanged()
        }
    }

    function reset() {
        _manualState = manualText === "" ? _statePassed : _statePending
        if (telemetryFailure) {
            _telemetryState = allowTelemetryFailureOverride ? _statePending : _stateFailed
        } else {
            _telemetryState = _statePassed
        }
    }

}
