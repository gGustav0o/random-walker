import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#262626"
    clip: true

    property var vm: SegmentationViewModel
    property bool expanded: false
    readonly property real expandedWidth: 320
    property real panelWidth: expanded ? expandedWidth : 0

    Layout.preferredWidth: panelWidth
    Layout.minimumWidth: panelWidth
    Layout.maximumWidth: panelWidth
    Layout.fillHeight: true
    opacity: expanded ? 1 : 0
    enabled: expanded

    Behavior on panelWidth {
        NumberAnimation {
            duration: 220
            easing.type: Easing.OutCubic
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: 140
            easing.type: Easing.OutCubic
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 14

        Label {
            Layout.fillWidth: true
            text: "Algorithm settings"
            color: "#f0f0f0"
            font.pixelSize: 18
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3a3a"
        }

        SettingsSection {
            title: "Random Walker"

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: "Beta"
                    color: "#ddd"
                    elide: Text.ElideRight
                }

                TextField {
                    id: betaField
                    Layout.preferredWidth: 108
                    enabled: !root.vm.busy
                    horizontalAlignment: Text.AlignRight
                    selectByMouse: true
                    validator: DoubleValidator {
                        bottom: 0.000001
                        top: 0.1
                        notation: DoubleValidator.ScientificNotation
                        locale: Qt.locale().name
                    }

                    function syncFromViewModel() {
                        if (!activeFocus)
                            forceSyncFromViewModel()
                    }

                    function forceSyncFromViewModel() {
                        text = Number(root.vm.beta).toLocaleString(
                            Qt.locale(),
                            "g",
                            6)
                    }

                    Component.onCompleted: forceSyncFromViewModel()

                    onEditingFinished: {
                        if (acceptableInput) {
                            root.vm.beta = Number.fromLocaleString(
                                Qt.locale(),
                                text)
                        }
                        forceSyncFromViewModel()
                    }

                    Connections {
                        target: root.vm
                        function onBetaChanged() {
                            betaField.syncFromViewModel()
                        }
                    }
                }
            }

            Slider {
                id: betaSlider
                Layout.fillWidth: true
                from: 0
                to: 1
                enabled: !root.vm.busy
                value: root.vm.beta_slider_position
                onMoved: root.vm.beta_slider_position = value
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
