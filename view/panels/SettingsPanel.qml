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

        ScrollView {
            id: settingsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: settingsScroll.availableWidth
                spacing: 14

                SettingsSection {
                    title: "Graph"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            Layout.fillWidth: true
                            text: "Connectivity"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        ComboBox {
                            Layout.preferredWidth: 150
                            enabled: !root.vm.busy
                            model: ["4-connectivity", "8-connectivity"]
                            currentIndex: root.vm.connectivity
                            onActivated: function(index) {
                                root.vm.connectivity = index
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            Layout.fillWidth: true
                            text: "Distance p"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        TextField {
                            id: distancePowerField
                            Layout.preferredWidth: 108
                            enabled: !root.vm.busy
                            horizontalAlignment: Text.AlignRight
                            selectByMouse: true
                            validator: DoubleValidator {
                                bottom: 0
                                top: 2
                                notation: DoubleValidator.StandardNotation
                                locale: Qt.locale().name
                            }

                            function syncFromViewModel() {
                                if (!activeFocus)
                                    forceSyncFromViewModel()
                            }

                            function forceSyncFromViewModel() {
                                text = Number(root.vm.distance_power).toLocaleString(
                                    Qt.locale(),
                                    "f",
                                    2)
                            }

                            Component.onCompleted: forceSyncFromViewModel()

                            onEditingFinished: {
                                if (acceptableInput) {
                                    root.vm.distance_power = Number.fromLocaleString(
                                        Qt.locale(),
                                        text)
                                }
                                forceSyncFromViewModel()
                            }

                            Connections {
                                target: root.vm
                                function onDistance_power_changed() {
                                    distancePowerField.syncFromViewModel()
                                }
                            }
                        }
                    }

                    Slider {
                        id: distancePowerSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 2
                        stepSize: 0.01
                        enabled: !root.vm.busy
                        value: root.vm.distance_power
                        onMoved: root.vm.distance_power = value
                    }
                }

                SettingsSection {
                    id: betaSection
                    title: "Beta"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: !root.vm.busy

                        Label {
                            Layout.fillWidth: true
                            text: "Beta"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        TextField {
                            id: betaField
                            Layout.preferredWidth: 108
                            enabled: parent.enabled
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
            }
        }
    }
}
