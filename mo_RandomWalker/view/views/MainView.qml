import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/qt/qml/mo_randomwalker/view/panels" as Panels


ApplicationWindow {
	id: root
	width: 1280
    height: 800
    visible: true
    color: "#232323"

    Panels.ControlPanel {
        id: controlPanel
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
    }

    BaseImageView {
        id: baseImage
        anchors.top: controlPanel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

}