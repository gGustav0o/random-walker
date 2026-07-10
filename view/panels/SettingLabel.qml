import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    id: root

    property string text: ""
    property string helpText: ""

    Layout.fillWidth: true
    spacing: 6

    Label {
        Layout.fillWidth: true
        text: root.text
        color: "#ddd"
        elide: Text.ElideRight
    }

    Item {
        id: helpAnchor
        Layout.preferredWidth: 16
        Layout.preferredHeight: 16
        visible: root.helpText.length > 0

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: helpMouseArea.containsMouse ? "#3a3a3a" : "#303030"
            border.color: helpMouseArea.containsMouse ? "#6a6a6a" : "#555"
        }

        Text {
            anchors.centerIn: parent
            text: "?"
            color: helpMouseArea.containsMouse ? "#f0f0f0" : "#aaa"
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }

        MouseArea {
            id: helpMouseArea
            anchors.fill: parent
            hoverEnabled: true
        }

        ToolTip {
            visible: helpMouseArea.containsMouse
            delay: 350
            timeout: 12000
            x: helpAnchor.width + 8
            y: -implicitHeight / 2 + helpAnchor.height / 2

            contentItem: Text {
                text: root.helpText
                color: "#f0f0f0"
                wrapMode: Text.WordWrap
                width: 280
                font.pixelSize: 12
                lineHeight: 1.15
                lineHeightMode: Text.ProportionalHeight
            }

            background: Rectangle {
                color: "#303030"
                border.color: "#5a5a5a"
                radius: 6
            }
        }
    }
}
