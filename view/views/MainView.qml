import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/qt/qml/random-walker/view/panels" as Panels



ApplicationWindow {
	id: root
	width: 1280
    height: 800
    visible: true
    title: "random-walker"
    color: "#232323"

    Panels.ControlPanel {
        id: controlPanel
        anchors.top: root.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
    }
    Panels.ModeSelectionPanel {
        id: modeSelectionPanel
        anchors.top: controlPanel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Panels.WithReferenceToolBar {
        id: withReferenceToolBar
        width: visible ? 200 : 0
        anchors.left: parent.left
        anchors.top: controlPanel.bottom
        anchors.bottom: parent.bottom
    }
    Panels.WithoutReferenceToolBar {
        id: withoutReferenceToolBar
        width: visible ? 200 : 0
        anchors.left: parent.left
        anchors.top: controlPanel.bottom
        anchors.bottom: parent.bottom
    }

    BaseImageView {
        id: baseImage
        anchors.top: controlPanel.bottom
        anchors.left: withReferenceToolBar.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
