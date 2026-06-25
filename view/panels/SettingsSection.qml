import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    spacing: 8

    property string title: ""
    default property alias content: body.data

    Layout.fillWidth: true

    Label {
        Layout.fillWidth: true
        text: root.title
        color: "#d8d8d8"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }

    ColumnLayout {
        id: body
        Layout.fillWidth: true
        spacing: 8
    }
}
