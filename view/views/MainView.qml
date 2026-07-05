import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/qt/qml/random-walker/view/panels" as Panels
import "qrc:/qt/qml/random-walker/view/windows" as Windows

ApplicationWindow {
    id: root
    width: 1000
    height: 700
    visible: true
    title: "Random Walker Segmentation"
    color: "#232323"

    property bool settingsPanelOpen: false

    Windows.AboutWindow {
        id: aboutWindow
    }

    Panels.ControlPanel {
        id: controlPanel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 96
        settingsPanelOpen: root.settingsPanelOpen
        onSettingsToggled: root.settingsPanelOpen = !root.settingsPanelOpen
        onAboutRequested: aboutWindow.open()
    }

    RowLayout {
        anchors.top: controlPanel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        BaseImageView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Panels.SettingsPanel {
            vm: SegmentationViewModel
            expanded: root.settingsPanelOpen
        }
    }
}
