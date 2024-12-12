import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

//MONARK Config
SetupPage {
    id: monarkPage
    pageComponent: pageComponent
    pageName: qsTr("MONARK")
    pageDescription: ""

    Component{
         id: pageComponent

             ColumnLayout {
                 width : availableWidth - (ScreenTools.defaultFontPixelWidth * 5)
                 //width:   parent.width
                 //height:  availableHeight
                 spacing: ScreenTools.defaultFontPixelHeight
                 //main header
                 QGCLabel{
                     //Layout.fillWidth: true
                     wrapMode:               Text.WordWrap
                     Layout.alignment: Qt.AlignHCenter
                     font.pointSize:         ScreenTools.largeFontPointSize
                     text:    (
                                  QGroundControl.monarkManager.monarkState === 0  //BeforeScan
                               || QGroundControl.monarkManager.monarkState === 1  //ScanInProgress
                               || QGroundControl.monarkManager.monarkState === 2  //ScanSuccessPairingRequired
                               || QGroundControl.monarkManager.monarkState === 3  //ScanSuccessBadCredentials
                               || QGroundControl.monarkManager.monarkState === 4  //ScanSuccessAndPaired
                               || QGroundControl.monarkManager.monarkState === 5  //ScanFailedNotDetected
                               || QGroundControl.monarkManager.monarkState === 6  //SaveSettingsInProgress
                               || QGroundControl.monarkManager.monarkState === 7  //SaveSettingsFailed
                              )
                            ? qsTr("MONARK Flutter Management")
                            : (
                                  QGroundControl.monarkManager.monarkState === 8  //BeforePairNewDrone
                               || QGroundControl.monarkManager.monarkState === 9  //ShowQRCode
                               || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                              )
                            ? qsTr("PAIR New Drone")
                            : QGroundControl.monarkManager.monarkState === 10 // ResetUnpairMonark
                            ? qsTr("Microhard Radio Management")
                            : qsTr("INVALID Application state. Restart application or contact support.")
                 }
                 //sub header
                 QGCLabel{
                      visible : QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                             || QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                             || QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                             || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                     // Layout.fillWidth: true
                      wrapMode:               Text.WordWrap
                      Layout.alignment: Qt.AlignHCenter
                      font.pointSize: ScreenTools.mediumFontPointSize
                      text:       QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                                ? qsTr("Press to connect to ground radio.")
                                : QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                                ? qsTr("Please enter ground radio settings to begin configuration.")
                                : QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                                ? qsTr("A ground radio is detected Please enter its encryption key and click NEXT.")
                                : QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                                ? qsTr("Could not find ground radio. Try again?")
                                : QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                                ? qsTr("Could not save settings. Try again?")
                                : qsTr("INVALID Application state. Restart application or contact support.")
                 }

                 //connect to ground radio button (states 0 and 5)
                 QGCButton {
                     Layout.fillWidth: true
                     Layout.alignment: Qt.AlignHCenter
                     visible: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                           || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                     text: qsTr("Connect to ground radio")
                     onClicked : {
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
                 //busy indicator stuff (states 1 and 6)
                 RowLayout{
                     visible: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                           || QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                     BusyIndicator{
                         running: true
                     }
                     QGCLabel{
                          Layout.fillWidth: true
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignHCenter
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                                    ? qsTr("Waiting for ground radio to connect...")
                                    : QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                                    ? qsTr("Saving settings...")
                                    : qsTr("INVALID Application state. Restart application or contact support")
                     }
                 }


                 //Saving settings stuff )states 2, 4, and 7)
                 ColumnLayout{
                     Layout.fillWidth: true
                     Layout.alignment:       Qt.AlignHCenter
                     visible : QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                            || QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                            || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Network ID")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.networkID
                             readOnly: true
                             Layout.fillWidth: true
                             textColor: acceptableInput ? "black" : "red"
                             id: networkIdTextField
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text:  qsTr("Encryption Key")
                         }
                         FactTextField{
                            readOnly: QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                            fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                            echoMode: TextInput.PasswordEchoOnEdit
                            //8 to 16 characters, all ASCII except comma, quotes, and equals
                            validator: RegExpValidator {regExp: /^[!#-+\--<>-~]{8,16}$/ }
                            Layout.fillWidth: true
                            id: encryptionKeyTextField
                            textColor: acceptableInput ? "black" : "red"
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Tx Power (dBm)")
                         }
                         FactTextField{
                             readOnly: QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                             fact: QGroundControl.settingsManager.monarkSettings.groundTxPower
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                             Layout.fillWidth: true
                             id: groundTxPowerTextField
                             textColor: acceptableInput ? "black" : "red"
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Frequency (MHz)")
                         }
                         FactTextField{
                             readOnly: QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                             fact: QGroundControl.settingsManager.monarkSettings.groundFrequency
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                             Layout.fillWidth: true
                             id: groundFrequencyTextField
                             textColor: acceptableInput ? "black" : "red"
                         }
                     }
                     QGCButton {
                         visible : (
                                       QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                                    || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                                   )
                         Layout.alignment:       Qt.AlignLeft
                         text:   qsTr("SAVE")
                         onClicked : {
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
                     QGCLabel{
                          visible : !encryptionKeyTextField.acceptableInput
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignLeft
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals")
                          color: "red"

                     }
                     QGCLabel{
                          visible : (
                                        QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                                     || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                                    )&& !groundTxPowerTextField.acceptableInput
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignLeft
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       qsTr("Ground Tx Power must be an integer between 1 and 10000")
                          color: "red"

                     }
                     QGCLabel{
                         visible : (
                                       QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                                    || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsFailed
                                   )&& !groundFrequencyTextField.acceptableInput
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignLeft
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       qsTr("Ground Frequency must be an integer between 1 and 10000")
                          color: "red"
                     }
                 }

                 //bad credentials stuff (state 3)
                 ColumnLayout{
                     Layout.fillWidth: true
                     Layout.alignment:       Qt.AlignHCenter
                     visible : QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2


                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text:  qsTr("Set Encryption Key")
                         }
                         FactTextField{
                            fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                            echoMode: TextInput.PasswordEchoOnEdit
                            //8 to 16 characters, all ASCII except comma, quotes, and equals
                            validator: RegExpValidator {regExp: /^[!#-+\--<>-~]{8,16}$/ }
                            Layout.fillWidth: true
                            id: encryptionKeyTextFieldBadCredentials
                            textColor: acceptableInput ? "black" : "red"
                         }
                     }
                     QGCButton {
                         Layout.alignment:       Qt.AlignLeft
                         text:   qsTr("NEXT")
                         onClicked : {
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
                     QGCLabel{
                          visible : !encryptionKeyTextFieldBadCredentials.acceptableInput
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignLeft
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       qsTr("Encryption Key must be 8 to 16 characters, all ASCII except comma, quotes, and equals")
                          color: "red"

                     }
                     //bad credentials message
                     QGCLabel{
                          wrapMode:               Text.WordWrap
                          Layout.alignment: Qt.AlignLeft
                          font.pointSize: ScreenTools.mediumFontPointSize
                          text:       qsTr("Encryption key did not match. Enter a different one or perform a factory reset of the ground radio, close the app, then try again.")
                          color: acceptableInput ? "black" : "red"

                     }
                 }

                 //connected to radio in a configured state (state 4)
                 ColumnLayout{
                     visible : QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                     //show connected drones here
                     ColumnLayout{
                         Layout.alignment:       Qt.AlignLeft
                         QGCLabel{
                             font.pointSize: ScreenTools.mediumFontPointSize
                             text:  qsTr("Connected Drones")
                         }
                         QGCLabel{
                             font.pointSize: ScreenTools.smallFontPointSize
                             text:  qsTr("NONE")
                             visible: QGroundControl.monarkManager.connectedDroneList.isEmpty()
                         }
                         Repeater{
                             model: QGroundControl.monarkManager.connectedDroneList
                             QGCLabel{
                                 font.pointSize: ScreenTools.smallFontPointSize
                                 text:  modelData.droneName
                             }
                         }
                     }

                     //Buttons to change settings and add new drones
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2
                         QGCButton {
                             text: qsTr("PAIR NEW DRONE")
                             onClicked : {
                                QGroundControl.monarkManager.gotoBeforePairNewDrone()
                             }
                         }
                         QGCButton {
                             text: qsTr("CHANGE ENCRYPTION KEY")
                             onClicked : {
                                //TODO
                             }
                         }
                         QGCButton {
                             text: qsTr("CHANGE FREQUENCIES")
                             onClicked : {
                                //TODO
                             }
                         }
                         QGCButton {
                             text: qsTr("CHANGE TX POWER")
                             onClicked : {
                                //TODO
                             }
                         }
                    }
                 }

                 //Adding a new drone (states 8 and 9)
                 ColumnLayout{
                     visible : QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                            || QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                            || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("MONARK ID")

                         }
                         FactTextField{
                             readOnly: QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                             fact: QGroundControl.settingsManager.monarkSettings.monarkID
                             validator: IntValidator {bottom: 1; top: 255}
                             Layout.fillWidth: true
                             id: monarkIdTextField
                             textColor: acceptableInput ? "black" : "red"
                         }
                     }
                     QGCLabel{
                         Layout.alignment: Qt.AlignLeft
                         visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                         || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                         text: QGroundControl.monarkManager.monarkState === 8
                         ? qsTr("Is the drone's microhard radio factory reset?")
                         : QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                         ? qsTr("Drone not detected. Try again?")
                         : qsTr("INVALID Application state. Restart application or contact support.")
                     }
                     Image {
                         visible: QGroundControl.monarkManager.monarkState === 9 //ShowQRCode
                         source:         "image://MONARKQRCodes/"+
                                         networkIdTextField.text+","+encryptionKeyTextField.text+","+groundTxPowerTextField.text+","+groundFrequencyTextField.text+","+monarkIdTextField.text
                                         /*
                                         "{"+
                                             "\"e\":\""+encryptionKeyTextField.text+"\","+
                                             "\"n\":\""+networkIdTextField.text+"\","+
                                             "\"t\":\""+groundTxPowerTextField.text+"\","+
                                             "\"f\":\""+groundFrequencyTextField.text+"\","+
                                             "\"m\":\""+monarkIdTextField.text+"\""+
                                         "}
*/
                         sourceSize.width: 500
                         sourceSize.height: 500
                         Layout.fillWidth: true
                         height:         width
                         cache:          false
                         fillMode:       Image.PreserveAspectFit
                     }

                     RowLayout{
                         visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                          || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                         QGCButton {
                             text: qsTr("YES")
                             onClicked : {
                                QGroundControl.monarkManager.detect()
                             }
                            enabled: monarkIdTextField.acceptableInput
                         }
                         QGCButton {
                             text: qsTr("NO")
                             onClicked : {
                                 if(QGroundControl.monarkManager.monarkState === 8)
                                 {
                                    QGroundControl.monarkManager.monarkState = 10 //ResetUnpairMonark
                                 }
                                 else
                                 {
                                     QGroundControl.monarkManager.monarkState = 4 //ScanSuccessAndPaired
                                 }
                                }
                         }
                         QGCButton {
                             visible: QGroundControl.monarkManager.monarkState === 8 //BeforePairNewDrone
                             text: qsTr("I DON'T KNOW")
                             onClicked : {
                                 QGroundControl.monarkManager.monarkState = 10 //ResetUnpairMonark
                             }
                         }
                     }


                 }

                 /*
                 QGCLabel {
                     Layout.fillWidth: true
                     Layout.alignment:       Qt.AlignLeft
                     Layout.columnSpan:      2
                     wrapMode:               Text.WordWrap
                     font.pointSize:         ScreenTools.largeFontPointSize
                     text:                     QGroundControl.monarkManager.monarkState === 0 ? qsTr("Press to connect to ground radio")
                                             : QGroundControl.monarkManager.monarkState === 1 ? qsTr("Waiting for ground radio to connect...")
                                             : QGroundControl.monarkManager.monarkState === 2 ? qsTr("SRM pairing required")
                                             : QGroundControl.monarkManager.monarkState === 3 ? qsTr("Bad credentials")
                                             : QGroundControl.monarkManager.monarkState === 4 ? qsTr("Already paired to SRM")
                                             : QGroundControl.monarkManager.monarkState === 5 ? qsTr("Scan failed")
                                             : QGroundControl.monarkManager.monarkState === 6 ? qsTr("Saving settings...")
                                             : QGroundControl.monarkManager.monarkState === 7 ? qsTr("Settings saved")
                                             : QGroundControl.monarkManager.monarkState === 8 ? qsTr("Save settings failed")
                                             : QGroundControl.monarkManager.monarkState === 9 ? qsTr("Detection in progress...")
                                             : QGroundControl.monarkManager.monarkState === 10 ? qsTr("MONARK detected...")
                                             : QGroundControl.monarkManager.monarkState === 11 ? qsTr("MONARK not detected...")
                                                                                              : qsTr("INVALID STATE REACHED PLEASE RESTART THE APPLICATION")
                 }
                 QGCButton {
                     Layout.fillWidth: true
                     visible: QGroundControl.monarkManager.monarkState === 0 //BeforeScan
                           || QGroundControl.monarkManager.monarkState === 5 //ScanFailedNotDetected
                     Layout.alignment:       Qt.AlignLeft
                     text: qsTr("Connect to ground radio")
                     onClicked : {
                         QGroundControl.monarkManager.startScanning()
                         //TODO show the scanning-in-progress page
                         //When scanning concludes, show an appropriate response
                     }
                 }
                 Image {
                     visible: QGroundControl.monarkManager.monarkState === 9 //DetectionInProgress
                     source:         "image://MONARKQRCodes/"+
                                     "{"+
                                         "\"e\":\""+encryptionKeyTextField.text+"\","+
                                         "\"n\":\""+networkIdTextField.text+"\","+
                                         "\"t\":\""+groundTxPowerTextField.text+"\","+
                                         "\"f\":\""+groundFrequencyTextField.text+"\","+
                                         "\"m\":\""+monarkIdTextField.text+"\""+
                                     "}"
                     sourceSize.width: 500
                     sourceSize.height: 500
                     Layout.fillWidth: true
                     height:         width
                     cache:          false
                     fillMode:       Image.PreserveAspectFit
                 }

                 BusyIndicator{
                     width: parent.width
                     height: parent.height
                     Layout.alignment:       Qt.AlignHCenter
                     visible: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                              ||    QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress

                     running: QGroundControl.monarkManager.monarkState === 1 //ScanInProgress
                              ||    QGroundControl.monarkManager.monarkState === 6 //SaveSettingsInProgress
                 }

                 ColumnLayout{
                     Layout.fillWidth: true
                     Layout.alignment:       Qt.AlignLeft
                     visible : QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                            || QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                            || QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                            || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsSuccess
                            || QGroundControl.monarkManager.monarkState === 8 //SaveSettingsFailed
                            || QGroundControl.monarkManager.monarkState === 10 //DetectionSuccess
                            || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Network ID")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.networkID
                             readOnly: true
                             Layout.fillWidth: true
                             textColor: acceptableInput ? "black" : "red"
                             id: networkIdTextField
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Encryption Key")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                             echoMode: TextInput.PasswordEchoOnEdit
                             //8 to 16 characters, all ASCII except comma, quotes, and equals
                             validator: RegExpValidator {regExp: /^[!#-+\--<>-~]{8,16}$/ }
                            Layout.fillWidth: true
                            id: encryptionKeyTextField
                            textColor: acceptableInput ? "black" : "red"
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Tx Power (dBm)")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.groundTxPower
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                             Layout.fillWidth: true
                             id: groundTxPowerTextField
                             textColor: acceptableInput ? "black" : "red"
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Frequency (MHz)")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.groundFrequency
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                             Layout.fillWidth: true
                             id: groundFrequencyTextField
                             textColor: acceptableInput ? "black" : "red"
                         }

                     }
                     QGCButton {
                         Layout.alignment:       Qt.AlignLeft
                         text: qsTr("SAVE")
                         onClicked : {
                             QGroundControl.monarkManager.saveFlutterManagementSettings()
                         }
                         enabled: encryptionKeyTextField.acceptableInput && groundTxPowerTextField.acceptableInput && groundFrequencyTextField.acceptableInput
                     }
                     QGCLabel {
                         Layout.fillWidth: true
                         visible : QGroundControl.monarkManager.monarkState === 7 //SaveSettingsSuccess
                         Layout.alignment:       Qt.AlignLeft
                         wrapMode:               Text.WordWrap
                         font.pointSize:         ScreenTools.largeFontPointSize
                         text:                   qsTr("Connect USB-C cable to the \"Pair\" port of the MONARK and connect the battery")
                     }
                     GridLayout{
                         Layout.alignment:       Qt.AlignHCenter
                         columns: 2
                         visible: QGroundControl.monarkManager.monarkState === 7 //SaveSettingsSuccess
                              || QGroundControl.monarkManager.monarkState === 10 //DetectionSuccess
                              || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("MONARK ID")

                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.monarkID
                             validator: IntValidator {bottom: 1; top: 255} //TODO find acceptable range
                             Layout.fillWidth: true
                             id: monarkIdTextField
                             textColor: acceptableInput ? "black" : "red"
                         }
                     }
                     QGCButton {
                         Layout.alignment:       Qt.AlignLeft
                         text: qsTr("DETECT")
                         onClicked : {
                             QGroundControl.monarkManager.detect()
                         }
                         enabled: monarkIdTextField.acceptableInput
                         visible: QGroundControl.monarkManager.monarkState === 7 //SaveSettingsSuccess
                              || QGroundControl.monarkManager.monarkState === 10 //DetectionSuccess
                              || QGroundControl.monarkManager.monarkState === 11 //DetectionFailed
                     }
                 }

*/

             }

    }
}
