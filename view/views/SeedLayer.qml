import QtQuick 2.15

Item {
    id: root

    required property var seedModel
    required property real imageX
    required property real imageY
    required property real imageWidth
    required property real imageHeight
    required property int sourceWidth
    required property int sourceHeight

    Repeater {
        model: root.seedModel

        delegate: Rectangle {
            required property int seedX
            required property int seedY
            required property int seedWidth
            required property int seedHeight
            required property int seedLabel

            x: root.imageX + seedX * root.imageWidth / root.sourceWidth
            y: root.imageY + seedY * root.imageHeight / root.sourceHeight
            width: seedWidth * root.imageWidth / root.sourceWidth
            height: seedHeight * root.imageHeight / root.sourceHeight

            color: seedLabel === 0 ? "#440000ff" : "#44ff0000"
            border.color: seedLabel === 0 ? "blue" : "red"
            border.width: 1
        }
    }
}
