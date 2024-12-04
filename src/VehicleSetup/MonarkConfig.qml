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
                     source:         "image://MONARKQRCodes/"+monarkIdTextField.text
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
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Encryption Key")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.encryptionKey
                             echoMode: TextInput.PasswordEchoOnEdit
                             //8 to 16 characters, all ASCII except comma and equals
                             validator: RegExpValidator {regExp: /^[!-+\--<>-~]{8,16}$/ }
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



             }

    }
}
