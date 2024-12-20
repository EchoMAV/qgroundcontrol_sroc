import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.11

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
    Component {
        id: pageComponent
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight

            //Main header
            QGCLabel {
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
                       ) ? qsTr("MONARK Flutter Management") : (QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                                                                || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                                                                ) ? qsTr("Pair New Drone") : QGroundControl.monarkManager.monarkState === 10 // ResetUnpairMonark
                                                                    ? qsTr("Microhard Radio Management") : qsTr("INVALID Application state. Restart application or contact support.")
            }

            //Sub header
            QGCLabel {
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
                      ? qsTr("Press to connect to ground radio.") : QGroundControl.monarkManager.monarkState
                        === 2 //ScanSuccessPairingRequired
                        ? qsTr("Please enter ground radio settings to begin configuration.") : QGroundControl.monarkManager.monarkState
                          === 3 //ScanSuccessBadCredentials
                          ? qsTr("A ground radio is detected Please enter its encryption key and click NEXT.") : QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                            ? qsTr("Could not find ground radio. Try again?") : QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              ? qsTr("Could not save settings. Try again?") : QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                                ? qsTr("Change Tx Power") : QGroundControl.monarkManager.monarkState
                                  === 13 //ChangeFrequencies
                                  ? qsTr("Change Frequency") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                    ? qsTr("Change Encryption Key") : qsTr(
                                          "INVALID Application state. Restart application or contact support.")
            }

            //Connect to ground radio button (states 0 and 5)
            QGCButton {
                visible: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                         || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                text: qsTr("Connect to ground radio")
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
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                          ? qsTr("Waiting for ground radio to connect...") : QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
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
                        text: qsTr("Network ID")
                    }
                    FactTextField {
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        fact: QGroundControl.settingsManager.monarkSettings.networkID
                        readOnly: true
                        textColor: acceptableInput ? "black" : "red"
                        id: networkIdTextField
                    }
                    QGCLabel {
                        text: qsTr("Encryption Key")
                    }
                    FactTextField {
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        readOnly: QGroundControl.monarkManager.monarkState
                                  === 4 //ScanSuccessAndPaired
                        fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        id: encryptionKeyTextField
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCLabel {
                        text: qsTr("Ground Tx Power (dBm)")
                    }
                    FactTextField {
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        readOnly: QGroundControl.monarkManager.monarkState
                                  === 4 //ScanSuccessAndPaired
                        fact: QGroundControl.settingsManager.monarkSettings.groundTxPower
                        validator: txPowerValidator
                        id: groundTxPowerTextField
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCLabel {
                        text: qsTr("Ground Frequency (MHz)")
                    }
                    FactTextField {
                        Layout.preferredWidth: 30 * ScreenTools.defaultFontPixelWidth
                        readOnly: QGroundControl.monarkManager.monarkState
                                  === 4 //ScanSuccessAndPaired
                        fact: QGroundControl.settingsManager.monarkSettings.groundFrequency
                        validator: frequencyValidator
                        id: groundFrequencyTextField
                        textColor: acceptableInput ? "black" : "red"
                    }
                }
                QGCButton {
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              )
                    text: qsTr("SAVE")
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
                    enabled: networkIdTextField.acceptableInput
                             && encryptionKeyTextField.acceptableInput
                             && groundTxPowerTextField.acceptableInput
                             && groundFrequencyTextField.acceptableInput
                }
                //acceptable input error messages
                QGCLabel {
                    visible: !encryptionKeyTextField.acceptableInput
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: "red"
                }
                QGCLabel {
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              ) && !groundTxPowerTextField.acceptableInput
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Tx Power must be an integer between 7 and 33")
                    color: "red"
                }
                QGCLabel {
                    visible: (QGroundControl.monarkManager.monarkState
                              === 2 //ScanSuccessPairingRequired
                              || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                              ) && !groundFrequencyTextField.acceptableInput
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Frequency is out of band for an 8Mhz bandwidth (see documentation)")
                    color: "red"
                }
            }

            //Bad credentials stuff (state 3)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                GridLayout {
                    columns: 2

                    QGCLabel {
                        text: qsTr("Set Encryption Key")
                    }
                    FactTextField {
                        fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        id: encryptionKeyTextFieldBadCredentials
                        textColor: acceptableInput ? "black" : "red"
                    }
                }
                QGCButton {
                    text: qsTr("NEXT")
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
                    visible: !encryptionKeyTextFieldBadCredentials.acceptableInput
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: "red"
                }
                //bad credentials message
                QGCLabel {
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption key did not match. Enter a different one or perform a factory reset of the ground radio, close the app, then try again.")
                    color: "red"
                }
            }

            //connected to radio in a configured state (state 4)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                //show connected drones here
                ColumnLayout {
                    QGCLabel {
                        font.pointSize: ScreenTools.mediumFontPointSize
                        text: qsTr("Connected Drones")
                    }
                    QGCLabel {
                        font.pointSize: ScreenTools.smallFontPointSize
                        text: qsTr("NONE")
                        visible: !QGroundControl.monarkManager.connectedDroneList
                                 || QGroundControl.monarkManager.connectedDroneList.isEmpty()
                    }
                    Repeater {
                        model: QGroundControl.monarkManager.connectedDroneList
                        QGCLabel {
                            font.pointSize: ScreenTools.smallFontPointSize
                            text: modelData.droneName
                        }
                    }
                }

                //Buttons to change settings and add new drones
                GridLayout {
                    columns: 2
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("PAIR NEW DRONE")
                        onClicked: {
                            QGroundControl.monarkManager.gotoBeforePairNewDrone(
                                        )
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE ENCRYPTION KEY")
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeEncryptionKey()
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE FREQUENCIES")
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeFrequencies()
                        }
                    }
                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("CHANGE TX POWER")
                        onClicked: {
                            QGroundControl.monarkManager.gotoChangeTxPower()
                        }
                    }
                }
            }

            //Before and after drone pairing (states 8 and 11)
            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                //|| QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                         || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                GridLayout {
                    columns: 2
                    QGCLabel {
                        text: qsTr("MONARK ID")
                    }
                    FactTextField {
                        readOnly: false //QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                        fact: QGroundControl.settingsManager.monarkSettings.monarkID
                        validator: monarkIdValidator
                        id: monarkIdTextField
                        textColor: acceptableInput ? "black" : "red"
                    }
                }
                QGCLabel {
                    visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                             || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
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
                        onClicked: {
                            QGroundControl.monarkManager.detect()
                        }
                        enabled: monarkIdTextField.acceptableInput
                    }
                    QGCButton {
                        text: qsTr("NO")
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
                    QGCLabel {
                        font.pointSize: ScreenTools.largeFontPointSize
                        text: qsTr("Pair New Drone")
                    }
                    QGCLabel {
                        text: qsTr("MONARK ID: ")
                              + QGroundControl.settingsManager.monarkSettings.monarkID.rawValue
                    }
                    QGCLabel {
                        text: qsTr("If the MONARK is in pairing state, you\nshould hear a single beep heartbeat. If\nnot, reattempt a factory reset. Otherwise,\nproceed as follows:")
                    }
                    QGCLabel {
                        text: qsTr("1. Aim the drone’s camera\ncentered at the QR code about\n4 inches away.")
                    }
                    QGCLabel {
                        text: qsTr("2. Slowly move the drone\nbackwards until you hear three\nquick beeps. It will not scan\nbeyond 3 feet away.")
                    }
                    QGCLabel {
                        text: qsTr("3. You will hear a double beep\nheartbeat as the pairing\nprocess begins.")
                    }
                    QGCLabel {
                        text: qsTr("4. Once beeping stops you should\nbe paired.")
                    }
                    QGCButton {
                        text: qsTr("Cancel")
                        onClicked: {
                            QGroundControl.monarkManager.gotoDetectionFailed()
                        }
                    }
                }
                Image {
                    source: "image://MONARKQRCodes/" + networkIdTextField.text + ","
                            + encryptionKeyTextField.text + "," + groundTxPowerTextField.text + ","
                            + groundFrequencyTextField.text + "," + monarkIdTextField.text
                    sourceSize.width: 500
                    sourceSize.height: 500
                    cache: false
                    fillMode: Image.PreserveAspectFit
                }
            }

            //Reset/Unpair Monark (state 10)
            ColumnLayout {
                visible: QGroundControl.monarkManager.monarkState === 10 //ResetUnpairMonark
                QGCLabel {
                    font.pointSize: ScreenTools.largeFontPointSize
                    text: qsTr("Reset/Unpair MONARK")
                }
                QGCLabel {
                    font.pointSize: ScreenTools.smallFontPointSize
                    text: qsTr("To reset the microhard radio in the MONARK, ensure the drone has been running for 30 or more seconds on battery.\nThen, press and hold the factory reset button using a SIM extractor tool on the radio module for 10+ seconds.\nYou should perceive a small click. Then release. Wait 30 or more seconds before powering down the drone.")
                }
                QGCButton {
                    text: qsTr("OK")
                    onClicked: {
                        QGroundControl.monarkManager.gotoScanSuccessAndPaired()
                    }
                }
            }

            //Changing settings (states 12, 13, and 14)
            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                         || QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                         || QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                GridLayout {
                    columns: 2
                    QGCLabel {
                        text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                              ? qsTr("Current Tx Power") : QGroundControl.monarkManager.monarkState
                                === 13 //ChangeFrequencies
                                ? qsTr("Current Frequency") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                  ? qsTr("Current Encryption Key") : qsTr(
                                        "INVALID Application state. Restart application or contact support.")
                    }

                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                        text: QGroundControl.settingsManager.monarkSettings.groundTxPower.rawValue
                        readOnly: true
                        validator: txPowerValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                        text: QGroundControl.settingsManager.monarkSettings.groundFrequency.rawValue
                        readOnly: true
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCTextField {
                        id: currentEncryptionKey
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        readOnly: false
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCLabel {
                        text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                              ? qsTr("Desired Tx Power") : QGroundControl.monarkManager.monarkState
                                === 13 //ChangeFrequencies
                                ? qsTr("Desired Frequency") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                                  ? qsTr("Desired Encryption Key") : qsTr(
                                        "INVALID Application state. Restart application or contact support.")
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                        id: desiredTxPower
                        inputMethodHints: Qt.ImhDigitsOnly
                        text: QGroundControl.settingsManager.monarkSettings.groundTxPower.rawValue
                        validator: txPowerValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                        id: desiredFrequency
                        text: QGroundControl.settingsManager.monarkSettings.groundFrequency.rawValue
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: frequencyValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCTextField {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        id: desiredEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                    QGCLabel {
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        text: qsTr("Confirm Encryption Key")
                    }
                    QGCTextField {
                        id: confirmEncryptionKey
                        visible: QGroundControl.monarkManager.monarkState
                                 === 14 //ChangeEncryptionKey
                        echoMode: TextInput.PasswordEchoOnEdit
                        readOnly: false
                        validator: encryptionKeyValidator
                        textColor: acceptableInput ? "black" : "red"
                    }
                }

                QGCLabel {
                    visible: !desiredTxPower.acceptableInput
                             && QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Tx Power must be an integer between 7 and 33")
                    color: "red"
                }
                QGCLabel {
                    visible: !desiredFrequency.acceptableInput
                             && QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Ground Frequency is out of band for an 8Mhz bandwidth (see documentation)")
                    color: "red"
                }
                QGCLabel {
                    visible: !desiredEncryptionKey.acceptableInput
                             && QGroundControl.monarkManager.monarkState
                             === 14 //ChangeEncryptionKey
                    font.pointSize: ScreenTools.mediumFontPointSize
                    text: qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals.")
                    color: "red"
                }
                QGCLabel {
                    text: qsTr("Once the update begins, DO NOT CLOSE THIS APP.\nIf any of the radios fail to update, you may need to factory reset them and start over.")
                }
                QGCButton {
                    id: understandButton
                    text: QGroundControl.monarkManager.monarkState === 12 //ChangeTxPower
                          ? qsTr("I Understand, Change Tx Power") : QGroundControl.monarkManager.monarkState === 13 //ChangeFrequencies
                            ? qsTr("I Understand, Change Frequencies") : QGroundControl.monarkManager.monarkState === 14 //ChangeEncryptionKey
                              ? qsTr("I Understand, Change Encryption Keys") : qsTr(
                                    "INVALID Application state. Restart application or contact support.")
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
                                        currentEncryptionKey.text,
                                        desiredEncryptionKey.text)
                        }
                    }
                    enabled: (QGroundControl.monarkManager.groundRadioUpdateState
                              === 0 //BeforeUpdate
                              || QGroundControl.monarkManager.groundRadioUpdateState
                              === 2 //UpdateSuccessful
                              || QGroundControl.monarkManager.groundRadioUpdateState
                              === 3) //UpdateFailed
                             && ((desiredTxPower.acceptableInput
                                  && QGroundControl.monarkManager.monarkState
                                  === 12) //ChangeTxPower
                                 || (desiredFrequency.acceptableInput
                                     && QGroundControl.monarkManager.monarkState
                                     === 13) //ChangeFrequencies
                                 || (desiredEncryptionKey.acceptableInput
                                     && QGroundControl.monarkManager.monarkState === 14
                                     && desiredEncryptionKey.text
                                     === confirmEncryptionKey.text)) //ChangeEncryptionKey
                }
                QGCButton {
                    Layout.preferredWidth: understandButton.width
                    text: qsTr("Previous Screen")
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
                Repeater {
                    model: QGroundControl.monarkManager.connectedDroneList
                    RowLayout {
                        QGCLabel {
                            text: modelData.droneName
                        }
                        Image {
                            visible: modelData.updateState === 2 //UpdateSuccessful
                                     || modelData.updateState === 3 //UpdateFailed
                            source: modelData.updateState === 2 //UpdateSuccessful
                                    ? "/qmlimages/checkbox-check.svg" : "/res/XDelete.svg"
                        }
                        BusyIndicator {
                            running: true
                            visible: modelData.updateState === 1 //UpdateInProgress
                        }
                    }
                }
                RowLayout {
                    QGCLabel {
                        text: qsTr("Ground Radio")
                    }
                    Image {

                        visible: QGroundControl.monarkManager.groundRadioUpdateState
                                 === 2 //UpdateSuccessful
                                 || QGroundControl.monarkManager.groundRadioUpdateState
                                 === 3 //UpdateFailed
                        source: QGroundControl.monarkManager.groundRadioUpdateState
                                === 2 //UpdateSuccessful
                                ? "/qmlimages/checkbox-check.svg" : "/res/XDelete.svg"
                    }
                    BusyIndicator {
                        running: true
                        visible: QGroundControl.monarkManager.groundRadioUpdateState
                                 === 1 //UpdateInProgress
                    }
                }
            }

            //Validators
            IntValidator {
                id: monarkIdValidator
                bottom: 1
                top: 255
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
        }
    }
}
