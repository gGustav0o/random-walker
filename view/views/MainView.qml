import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/qt/qml/random-walker/view/panels" as Panels

ApplicationWindow {
    id: root
    width: 1000
    height: 700
    visible: true
    title: "Random Walker Segmentation"
    color: "#232323"

    Panels.ControlPanel {
        id: controlPanel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
    }

    BaseImageView {
        anchors.top: controlPanel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
