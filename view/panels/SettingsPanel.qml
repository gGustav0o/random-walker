import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: settingsPanel
    property var vm: SettingsPanelViewModel {}

    width: 240
    color: "#2c2c2c"
    visible: true

    anchors.top: parent.top
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    z: 10

    property real closedX: parent ? parent.width : 1280
    property real openedX: parent ? parent.width - width : 1040

    x: vm.visible ? openedX : closedX

    opacity: vm.visible ? 1 : 0
    enabled: vm.visible

    Behavior on x {
        NumberAnimation { duration: 320; easing.type: Easing.InOutCubic }
    }
    Behavior on opacity {
        NumberAnimation { duration: 200 }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 24

        Text {
            text: "Settings"
            font.bold: true
            font.pixelSize: 20
            color: "#fff"
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: 10
            Label { text: "Sigma:"; color: "#fff" }
            SpinBox {
                id: sigmaBox
                from: 0.1; to: 10.0; value: vm.sigma
                stepSize: 0.1; decimals: 2
                width: 96

                onValueChanged: vm.sigma = value
                Connections {
                    target: vm
                    function onSigmaChanged() { sigmaBox.value = vm.sigma }
                }
            }
        }

        Button {
            text: "Close"
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: vm.visible = false
        }
    }
}
