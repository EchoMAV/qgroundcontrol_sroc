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
                     text:                     QGroundControl.monarkManager.monarkState === 0 ? qsTr("Press to connect to ground radio (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 1 ? qsTr("Waiting for ground radio to connect... (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 2 ? qsTr("Pairing Required (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 3 ? qsTr("Bad credentials (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 4 ? qsTr("Already paired (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 5 ? qsTr("Scan failed (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 6 ? qsTr("Saving settings... (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 7 ? qsTr("Settings saved (placeholder text)") //TODO
                                             : QGroundControl.monarkManager.monarkState === 8 ? qsTr("Save settings failed (placeholder text)") //TODO
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

                 ColumnLayout{
                     Layout.fillWidth: true
                     Layout.alignment:       Qt.AlignLeft
                     visible : QGroundControl.monarkManager.monarkState === 2 //ScanSuccessPairingRequired
                            || QGroundControl.monarkManager.monarkState === 3 //ScanSuccessBadCredentials
                            || QGroundControl.monarkManager.monarkState === 4 //ScanSuccessAndPaired
                            || QGroundControl.monarkManager.monarkState === 7 //SaveSettingsSuccess
                            || QGroundControl.monarkManager.monarkState === 8 //SaveSettingsFailed
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
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Tx Power (dBm)")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.groundTxPower
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                              Layout.fillWidth: true
                         }
                         QGCLabel{
                             Layout.alignment: Qt.AlignRight
                             text: qsTr("Ground Frequency (MHz)")
                         }
                         FactTextField{
                             fact: QGroundControl.settingsManager.monarkSettings.groundFrequency
                             validator: IntValidator {bottom: 1; top: 10000} //TODO find acceptable range
                              Layout.fillWidth: true
                         }

                     }
                     QGCButton {
                         Layout.alignment:       Qt.AlignLeft
                         text: qsTr("SAVE")
                         onClicked : {
                             QGroundControl.monarkManager.saveFlutterManagementSettings()
                         }
                     }

                 }
                 QGCLabel {
                     Layout.fillWidth: true
                     visible : QGroundControl.monarkManager.monarkState === 7
                     Layout.alignment:       Qt.AlignLeft
                     wrapMode:               Text.WordWrap
                     font.pointSize:         ScreenTools.largeFontPointSize
                     text:                   qsTr("Connect USB-C cable to the \"Pair\" port of the MONARK and connect the battery")
                 }

             }

    }
}
