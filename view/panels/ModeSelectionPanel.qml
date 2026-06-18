import QtQuick 2.15
import QtQuick.Controls 2.15

Pane {
    id: panel
    padding: 20
    background: Rectangle {
        color: "#1e1e1e"
        radius: 8
        border.color: "#333"
        border.width: 1
    }

    property var vm: ModeSelectionlPanelViewModel
    visible: vm.visible

    Column {
        id: contentItem
        spacing: 20
        anchors.centerIn: parent
        Text {
            text: "Select mode"
            font.pixelSize: 20
            color: "#fff"
            horizontalAlignment: Text.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            Button {
                text: "Reference"
                onClicked: {
                    vm.select_with_reference()
                }
            }
            Button {
                text: "No reference"
                onClicked: {
                    vm.select_without_reference()
                }
            }
        }
    }
}
