import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.11
import QtQuick.Window 2.2

import QGroundControl 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

//MONARK Config
SetupPage {
    id: monarkPage
    pageComponent: pageComponent
    pageName: qsTr("MONARK")
    pageDescription: ""

    MessageDialog {
        id: newDroneAddedConfirmation
        title: qsTr("New MONARK ID Added")
        text: "MONARK-" + QGroundControl.monarkManager.newDroneId + " has been added"
        standardButtons: StandardButton.Ok
    }

    Connections {
        target: QGroundControl.monarkManager
        onNewDroneIdChanged: {
            newDroneAddedConfirmation.open()
        }
    }

    Component {
        id: pageComponent
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight
            Layout.maximumWidth: 97 * ScreenTools.defaultFontPixelWidth
            //Main header
            QGCLabel {
                wrapMode: Text.Wrap
                //Layout.fillWidth: true
                visible: !(QGroundControl.monarkManager.monarkState === 9) //Not ShowQRCode
                font.pointSize: ScreenTools.largeFontPointSize
                text: (QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                       || QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                       || QGroundControl.monarkManager.monarkState
                       === 2 //ScanSuccessPairingRequired
                       || QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                       || QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                       || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                       || QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                       || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                       || QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                       || QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                       || QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                       ) ? qsTr(
                               "MONARK Management") : (QGroundControl.monarkManager.monarkState
                                                       === 8 //BeforePairNewDrone
                                                       || QGroundControl.monarkManager.monarkState
                                                       === 11 //DetectionFailed
                                                       ) ? qsTr("Pair New Drone") : QGroundControl.monarkManager.monarkState === 10 // ResetUnpairMonark
                                                           ? qsTr("Microhard Radio Management") : qsTr("INVALID Application state. Restart application or contact support.")
            }

            //Sub header
            QGCLabel {
                wrapMode: Text.Wrap
                //Layout.fillWidth: true
                visible: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                         || QGroundControl.monarkManager.monarkState
                         === 2 //ScanSuccessPairingRequired
                         || QGroundControl.monarkManager.monarkState
                         === 3 //ScanSuccessBadCredentials
                         || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                         || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                         || QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                         || QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                         || QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey

                font.pointSize: ScreenTools.mediumFontPointSize
                text: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                      ? qsTr("Press to connect to EchoLink.") : QGroundControl.monarkManager.monarkState
                        === 2 //ScanSuccessPairingRequired
                        ? qsTr("Please enter EchoLink settings to begin configuration.") : QGroundControl.monarkManager.monarkState
                          === 3 //ScanSuccessBadCredentials
                          ? qsTr("An EchoLink is detected Please enter its encryption key and click NEXT.") : QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                            ? qsTr("Could not find EchoLink. Try again?") : QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              ? qsTr("Could not save settings. Try again?") : QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                                ? qsTr("Change Tx Power (7-33 dBm)") : QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                                  ? qsTr("Change Frequency") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                    ? qsTr("Change Encryption Key") : qsTr(
                                          "INVALID Application state. Restart application or contact support.")
            }

            //Connect to EchoLink button (states 0 and 5)
            QGCButton {
                visible: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                         || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                text: qsTr("Connect to EchoLink")
                font.pointSize: ScreenTools.mediumFontPointSize
                onClicked: {
                    // assumed start state is
                    // 0 BeforeScan
                    // 5 ScanFailedNotDetected
                    // on clicked, will transition to
                    // 1 ScanInProgress
                    // at the end, will transition to
                    // 2 ScanSuccessPairingRequired if default configuration was found
                    // 3 ScanSuccessBadCredentials  if configured radio was found with bad credentials
                    // 4 ScanSuccessAndPaired       if configured radio was found with good credentials
                    // 5 ScanFailedNotDetected      if no radio was found
                    QGroundControl.monarkManager.startScanning()
                }
            }

            //Busy indicator stuff (states 1 and 6)
            RowLayout {
                visible: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                         || QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                BusyIndicator {
                    running: true
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                          ? qsTr("Waiting for EchoLink to connect...") : QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                            ? qsTr("Saving settings...") : qsTr(
                                  "INVALID Application state. Restart application or contact support")
                }
            }

            //Saving settings stuff )states 2, 4, and 7)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                         || QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                         || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                GridLayout {
                    columns: 2
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: qsTr("Network ID:")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        text: QGroundControl.settingsManager.monarkSettings.networkID.rawValue
                        id: networkIdTextField
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: qsTr("Encryption Key:")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    FactTextField {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 2 //ScanSuccessPairingRequired
                                 || QGroundControl.monarkManager.monarkState
                                 === 7 //SaveSettingsFailed
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        id: encryptionKeyTextField
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState
                                 === 4 //ScanSuccessAndPaired
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        text: "****************"
                        //text: QGroundControl.settingsManager.monarkSettings.encryptionKey.rawValue
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState
                                 === 2 //ScanSuccessPairingRequired
                                 || QGroundControl.monarkManager.monarkState
                                 === 7 //SaveSettingsFailed
                        text: qsTr("Confirm Encryption Key")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCTextField {
                        id: confirmEncryptionKey1
                        visible: QGroundControl.monarkManager.monarkState
                                 === 2 //ScanSuccessPairingRequired
                                 || QGroundControl.monarkManager.monarkState
                                 === 7 //SaveSettingsFailed
                        echoMode: TextInput.PasswordEchoOnEdit
                        readOnly: false
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: qsTr("Ground Tx Power (dBm):")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    FactTextField {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 2 //ScanSuccessPairingRequired
                                 || QGroundControl.monarkManager.monarkState
                                 === 7 //SaveSettingsFailed
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        fact: QGroundControl.settingsManager.monarkSettings.groundTxPower
                        validator: txPowerValidator
                        id: groundTxPowerTextField
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState
                                 === 4 //ScanSuccessAndPaired
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        text: QGroundControl.settingsManager.monarkSettings.groundTxPower.rawValue
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: qsTr("Ground Frequency (MHz):")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    FactTextField {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 2 //ScanSuccessPairingRequired
                                 || QGroundControl.monarkManager.monarkState
                                 === 7 //SaveSettingsFailed
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        fact: QGroundControl.settingsManager.monarkSettings.groundFrequency
                        validator: frequencyValidator
                        id: groundFrequencyTextField
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState
                                 === 4 //ScanSuccessAndPaired
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        text: QGroundControl.settingsManager.monarkSettings.groundFrequency.rawValue
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                }
                QGCButton {
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              )
                    text: qsTr("SAVE")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    onClicked: {
                        // assumed start state is
                        // 2 ScanSuccessPairingRequired
                        // 7 SaveSettingsFailed
                        // on clicked, will transition to
                        // 6 SaveSettingsInProgress
                        // at the end, will transition to
                        // 4 ScanSuccessAndPaired      if the settings were saved
                        // 7 SaveSettingsFailed        if the settings could not be saved
                        QGroundControl.monarkManager.saveFlutterManagementSettings()
                    }
                    enabled: encryptionKeyTextField.acceptableInput
                             && groundTxPowerTextField.acceptableInput
                             && groundFrequencyTextField.acceptableInput
                             && (encryptionKeyTextField.text === confirmEncryptionKey1.text)
                }
                //acceptable input error messages
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              )
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: encryptionKeyTextField.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              )
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Tx Power must be an integer between 7 and 33")
                    color: groundTxPowerTextField.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              )
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Frequency must be in band for an 8Mhz bandwidth (see documentation)")
                    color: groundFrequencyTextField.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
            }

            //Bad credentials stuff (state 3)
            ColumnLayout {
                Layout.maximumWidth: 97 * ScreenTools.defaultFontPixelWidth
                visible: QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                GridLayout {
                    columns: 2

                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: qsTr("Encryption Key")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    FactTextField {
                        fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        id: encryptionKeyTextFieldBadCredentials
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                }
                QGCButton {
                    text: qsTr("NEXT")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    onClicked: {
                        // assumed start state is
                        // 3 ScanSuccessBadCredentials
                        // on clicked, will transition to
                        // 6 SaveSettingsInProgress
                        // at the end, will transition to
                        // 3 ScanSuccessBadCredentials if the password was bad
                        // 4 ScanSuccessAndPaired      if the settings were saved
                        QGroundControl.monarkManager.saveFlutterManagementSettings()
                    }
                    enabled: encryptionKeyTextFieldBadCredentials.acceptableInput
                }
                //acceptable input error messages
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: encryptionKeyTextFieldBadCredentials.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                //bad credentials message
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption key did not match.\nEnter a different one or perform a factory reset of the EchoLink, close the app, then try again.")
                    color: qgcPal.warningText
                }
            }

            //connected to radio in a configured state (state 4)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                //show connected drones here
                ColumnLayout {
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("Connected Drones")
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: QGroundControl.monarkManager.allDrones
                    }
                }

                //Buttons to change settings and add new drones
                GridLayout {
                    columns: 2
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("PAIR NEW DRONE")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        enabled: !QGroundControl.multiVehicleManager.activeVehicle
                        onClicked: {
                            QGroundControl.monarkManager.gotoBeforePairNewDrone(
                                        )
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE ENCRYPTION KEY")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeEncryptionKey()
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE FREQUENCIES")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeFrequencies()
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE TX POWER")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeTxPower()
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("RESET ACTIVE VEHICLE")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        enabled: QGroundControl.multiVehicleManager.activeVehicle
                        onClicked: {
                            confirmVehicleReset.open()
                        }

                        MessageDialog {
                            id: confirmVehicleReset
                            title: qsTr("Confirm Reset")
                            text: qsTr("Are you sure you want to reset active drone ID ")
                                  + QGroundControl.multiVehicleManager.activeVehicle.id + qsTr(
                                      "? Be sure to power-cycle the drone after you hear a beep indicator.")
                            standardButtons: StandardButton.Yes | StandardButton.No
                            onYes: {
                                QGroundControl.monarkManager.resetActiveVehicle(
                                            )
                            }
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("REBOOT ACTIVE VEHICLE")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        enabled: QGroundControl.multiVehicleManager.activeVehicle
                        onClicked: {
                            confirmVehicleReboot.open()
                        }

                        MessageDialog {
                            id: confirmVehicleReboot
                            title: qsTr("Confirm Reboot")
                            text: qsTr("Are you sure you want to reboot active drone ID ")
                                  + QGroundControl.multiVehicleManager.activeVehicle.id + qsTr(
                                      "?")
                            standardButtons: StandardButton.Yes | StandardButton.No
                            onYes: {
                                QGroundControl.monarkManager.rebootActiveVehicle()
                            }
                        }
                    }
                }
            }

            //Before and after drone pairing (states 8 and 11)
            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight
                visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                         || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                GridLayout {
                    columns: 2
                    QGCLabel {
                        wrapMode: Text.Wrap
                        text: qsTr("MONARK ID (1-254)")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    FactTextField {
                        readOnly: false //QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                        fact: QGroundControl.settingsManager.monarkSettings.monarkID
                        validator: monarkIdValidator
                        id: monarkIdTextField
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    text: qsTr("Wait for single beep heartbeat before proceeding.")
                    font.pointSize: ScreenTools.mediumFontPointSize
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                             || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: QGroundControl.monarkManager.monarkState
                          === 8 ? qsTr("Is the drone's microhard radio factory reset?") : QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                                  ? qsTr("Drone not detected. Try again?") : qsTr(
                                        "INVALID Application state. Restart application or contact support.")
                }

                RowLayout {
                    visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                             || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                    QGCButton {
                        text: qsTr("YES")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.detect()
                        }
                        enabled: monarkIdTextField.acceptableInput
                    }
                    QGCButton {
                        text: qsTr("NO")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            if (QGroundControl.monarkManager.monarkState === 8) {
                                QGroundControl.monarkManager.gotoResetUnpairMonark()
                            } else {
                                QGroundControl.monarkManager.gotoScanSuccessAndPaired()
                            }
                        }
                    }
                    QGCButton {
                        visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("I DON'T KNOW")
                        onClicked: {
                            QGroundControl.monarkManager.gotoResetUnpairMonark()
                        }
                    }
                }
            }

            //Show QR code (state 9)
            RowLayout {
                visible: QGroundControl.monarkManager.monarkState === 9 //ShowQRCode

                ColumnLayout {
                    id: pairingInstructions
                    Layout.maximumWidth: 45 * ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        font.pointSize: ScreenTools.defaultFontPointSize
                        text: qsTr("Pair New Drone (MONARK-")
                              + QGroundControl.settingsManager.monarkSettings.monarkID.rawValue
                              + ")"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCLabel {
                        text: qsTr("You should hear a single beep heartbeat. If not, re-attempt a factory reset.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCLabel {
                        text: qsTr("1. Aim drone camera 4 inches away from phone.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCLabel {
                        text: qsTr("2. Slowly move the drone backwards until success beep.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCLabel {
                        text: qsTr("3. You will hear a double beep heartbeat as the pairing process begins.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCLabel {
                        text: qsTr("4. Once the beeping stops, wait for the GCS to finalize connection.")
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    QGCButton {
                        text: qsTr("Cancel")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.gotoDetectionFailed()
                        }
                    }
                    Timer {
                        property var timeLeft: 180
                        id: paringCountdownTimer
                        interval: 1000
                        running: timerText.visible
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: {

                            timerText.text = timeLeft + " seconds remaining"
                            if (timeLeft > 0) {
                                --timeLeft
                            }
                        }
                    }
                    QGCLabel {
                        id: timerText
                        font.pointSize: ScreenTools.defaultFontPointSize
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        onVisibleChanged: {
                            if (visible) {
                                paringCountdownTimer.timeLeft = 180
                                paringCountdownTimer.restart()
                            }
                        }
                    }
                }

                Image {
                    Layout.alignment: Qt.AlignTop
                    source: "image://MONARKQRCodes/" + networkIdTextField.text + ","
                            + encryptionKeyTextField.text + "," + groundTxPowerTextField.text + ","
                            + groundFrequencyTextField.text + "," + monarkIdTextField.text
                    sourceSize.width: 875
                    sourceSize.height: 875 //TODO is there a way to un-hard-code these
                    cache: false
                    fillMode: Image.PreserveAspectFit
                }
            }

            //Reset/Unpair Monark (state 10)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 10 //ResetUnpairMonark
                Layout.maximumWidth: 97 * ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    wrapMode: Text.Wrap
                    //Layout.fillWidth: true
                    font.pointSize: ScreenTools.largeFontPointSize
                    text: qsTr("Reset/Unpair MONARK")
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("To reset the microhard radio in the MONARK, ensure the drone has been running for 30 or more seconds on battery. Then, press and hold the factory reset button using a reset tool on the radio module for 30+ seconds. You should perceive a small click as the button is pressed. Then release. Wait 30 or more seconds before powering down the drone.")
                }
                QGCButton {
                    text: qsTr("OK")
                    font.pointSize: ScreenTools.mediumFontPointSize
                    onClicked: {
                        QGroundControl.monarkManager.gotoScanSuccessAndPaired()
                    }
                }
            }

            //Changing settings (states 12, 13, and 14)
            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight
                Layout.maximumWidth: 97 * ScreenTools.defaultFontPixelWidth

                visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                         || QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                         || QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                GridLayout {
                    columns: 2
                    QGCLabel {
                        visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                                 || QGroundControl.monarkManager.monarkState
                                 === 13 //ChangeFrequencies
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                              ? qsTr("Current Tx Power") : QGroundControl.monarkManager.monarkState
                                === 13 //ChangeFrequencies
                                ? qsTr("Current Frequency") : qsTr(
                                      "INVALID Application state. Restart application or contact support.")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                        text: QGroundControl.settingsManager.monarkSettings.groundTxPower.rawValue
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                        text: QGroundControl.settingsManager.monarkSettings.groundFrequency.rawValue
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }


                    /*
                    QGCTextField {
                        id: currentEncryptionKey
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        readOnly: false
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }*/
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                              ? qsTr("Desired Tx Power") : QGroundControl.monarkManager.monarkState
                                === 13 //ChangeFrequencies
                                ? qsTr("Desired Frequency") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                  ? qsTr("Desired Encryption Key") : qsTr(
                                        "INVALID Application state. Restart application or contact support.")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                        id: desiredTxPower
                        inputMethodHints: Qt.ImhDigitsOnly
                        text: QGroundControl.settingsManager.monarkSettings.groundTxPower.rawValue
                        validator: txPowerValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                        id: desiredFrequency
                        text: QGroundControl.settingsManager.monarkSettings.groundFrequency.rawValue
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: frequencyValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        id: desiredEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        text: qsTr("Confirm Encryption Key")
                        font.pointSize: ScreenTools.mediumFontPointSize
                    }
                    QGCTextField {
                        id: confirmEncryptionKey
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        readOnly: false
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? qgcPal.textFieldText : qgcPal.warningText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                    }
                }

                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("EchoLink Tx Power must be an integer between 7 and 33")
                    color: desiredTxPower.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    visible: QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Frequency must be in band for an 8Mhz bandwidth (see documentation)")
                    color: desiredFrequency.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    visible: QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: desiredEncryptionKey.acceptableInput ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    visible: QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr(
                              "Encryption keys must match in both text fields.")
                    color: desiredEncryptionKey.text
                           === confirmEncryptionKey.text ? qgcPal.text : qgcPal.warningText
                }
                QGCLabel {
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    text: qsTr("Once the update begins, DO NOT CLOSE THIS APP or power down/unplug any devices. If any of the radios fail to update, you may need to factory reset them and start over.")
                    font.pointSize: ScreenTools.mediumFontPointSize
                }
                RowLayout {
                    QGCButton {
                        id: understandButton
                        text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                              ? qsTr("I Understand, Change Tx Power") : QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                                ? qsTr("I Understand, Change Frequencies") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                  ? qsTr("I Understand, Change Encryption Keys") : qsTr(
                                        "INVALID Application state. Restart application or contact support.")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            if (QGroundControl.monarkManager.monarkState === 12) //ChangeTxPower
                            {
                                QGroundControl.monarkManager.changeTxPower(
                                            desiredTxPower.text)
                            } else if (QGroundControl.monarkManager.monarkState
                                       === 13) //ChangeFrequencies
                            {
                                QGroundControl.monarkManager.changeFrequencies(
                                            desiredFrequency.text)
                            } else if (QGroundControl.monarkManager.monarkState
                                       === 14) //ChangeEncryptionKey
                            {
                                QGroundControl.monarkManager.changeEncryptionKey(
                                            desiredEncryptionKey.text)
                            }
                        }
                        enabled: (QGroundControl.monarkManager.groundRadioUpdateState
                                  === 0 //BeforeUpdate
                                  || QGroundControl.monarkManager.groundRadioUpdateState
                                  === 2 //UpdateSuccessful
                                  || QGroundControl.monarkManager.groundRadioUpdateState
                                  === 3) //UpdateFailed
                                 && (inProgressDronesText.text.length === 0)
                                 && ((desiredTxPower.acceptableInput
                                      && QGroundControl.monarkManager.monarkState
                                      === 12) //ChangeTxPower
                                     || (desiredFrequency.acceptableInput
                                         && QGroundControl.monarkManager.monarkState
                                         === 13) //ChangeFrequencies
                                     || (desiredEncryptionKey.acceptableInput
                                         && QGroundControl.monarkManager.monarkState
                                         === 14 //ChangeEncryptionKey
                                         && desiredEncryptionKey.text
                                         === confirmEncryptionKey.text))
                    }
                    QGCButton {
                        Layout.preferredWidth: understandButton.width
                        text: qsTr("< Previous Screen")
                        font.pointSize: ScreenTools.mediumFontPointSize
                        onClicked: {
                            QGroundControl.monarkManager.gotoScanSuccessAndPaired()
                        }
                        enabled: QGroundControl.monarkManager.groundRadioUpdateState
                                 === 0 //BeforeUpdate
                                 || QGroundControl.monarkManager.groundRadioUpdateState
                                 === 2 //UpdateSuccessful
                                 || QGroundControl.monarkManager.groundRadioUpdateState
                                 === 3 //UpdateFailed
                    }
                }
                GridLayout {
                    columns: 2
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("Pending:")
                        //color: desiredEncryptionKey.acceptableInput ? qgcPal.text : qgcPal.warningText
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        id: pendingDronesText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: QGroundControl.monarkManager.beforeUpdateDrones
                        //color: desiredEncryptionKey.acceptableInput ? qgcPal.text : qgcPal.warningText
                    }

                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("In Progress:")
                        //color: desiredEncryptionKey.acceptableInput ? qgcPal.text : qgcPal.warningText
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        id: inProgressDronesText
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: QGroundControl.monarkManager.updateInProgressDrones
                        //color: desiredEncryptionKey.acceptableInput ? qgcPal.text : qgcPal.warningText
                    }

                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("Successful:")
                        color: qgcPal.colorGreen
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: QGroundControl.monarkManager.updateSuccessfulDrones
                        color: qgcPal.colorGreen
                    }

                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        visible: QGroundControl.monarkManager.updateFailedDrones.length != 0
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("Failed:")
                        color: qgcPal.warningText
                    }
                    QGCLabel {
                        wrapMode: Text.Wrap
                        //Layout.fillWidth: true
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: QGroundControl.monarkManager.updateFailedDrones
                        color: qgcPal.warningText
                    }
                }
            }

            //Validators
            IntValidator {
                id: monarkIdValidator
                bottom: 1
                top: 254
            }
            IntValidator {
                id: txPowerValidator
                bottom: 7
                top: 33
            }
            RegExpValidator {
                id: encryptionKeyValidator
                //8 to 16 characters, all ASCII except comma, quotes, and equals
                regExp: /^[!#-+\--<>-~]{8,16}$/
            }
            RegExpValidator {
                id: frequencyValidator
                //I'm really sorry about this
                //1629
                //1630-1699
                //1700-1719
                //1720-1721
                //1784-1789
                //1790-1799
                //1800-1839
                //1840-1846
                //2024-2029
                //2030-2099
                //2100-2106
                //2204-2209
                //2210-2289
                //2290-2296
                //2305-2309
                //2310-2379
                //2380-2386
                //2404-2409
                //2410-2489
                //2490-2496
                regExp: /^(1629|16[3-9][0-9]|17[01][0-9]|172[01]|178[4-9]|179[0-9]|18[0-3][0-9]|184[0-6]|202[4-9]|20[3-9][0-9]|210[0-6]|220[4-9]|22[1-8][0-9]|229[0-6]|230[5-9]|23[1-7][0-9]|238[0-6]|240[4-9]|24[1-8][0-9]|249[0-6])$/
            }
            QGCPalette {
                id: qgcPal
                colorGroupEnabled: enabled
            }
        }
    }
}
