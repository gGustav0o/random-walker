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
                    id: edgeWeightSection
                    title: "Edge Weight"
                    readonly property bool globalBetaWeightSelected:
                        root.vm.edge_weight_model === 0
                    readonly property bool localVarianceWeightSelected:
                        root.vm.edge_weight_model === 1

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            Layout.fillWidth: true
                            text: "Weight"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        ComboBox {
                            id: edgeWeightModelBox
                            Layout.preferredWidth: 150
                            enabled: !root.vm.busy
                            model: ["Global beta", "Local variance"]
                            currentIndex: root.vm.edge_weight_model
                            onActivated: function(index) {
                                root.vm.edge_weight_model = index
                            }
                        }
                    }


                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: edgeWeightSection.globalBetaWeightSelected
                            && !root.vm.busy
                        opacity: edgeWeightSection.globalBetaWeightSelected
                            ? 1.0 : 0.45

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
                        enabled: edgeWeightSection.globalBetaWeightSelected
                            && !root.vm.busy
                        opacity: edgeWeightSection.globalBetaWeightSelected
                            ? 1.0 : 0.45
                        value: root.vm.beta_slider_position
                        onMoved: root.vm.beta_slider_position = value
                    }
                }

                SettingsSection {
                    id: localVarianceSection
                    title: "Local Variance"
                    readonly property bool selected:
                        root.vm.edge_weight_model === 1
                    readonly property bool manualFloorSelected:
                        root.vm.local_contrast_minimum_variance_mode === 0

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: localVarianceSection.selected && !root.vm.busy
                        opacity: localVarianceSection.selected ? 1.0 : 0.45

                        Label {
                            Layout.fillWidth: true
                            text: "Window radius"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        SpinBox {
                            id: localContrastRadiusBox
                            Layout.preferredWidth: 108
                            from: 1
                            to: 16
                            value: root.vm.local_contrast_radius
                            editable: true
                            onValueModified: root.vm.local_contrast_radius = value
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: localVarianceSection.selected && !root.vm.busy
                        opacity: localVarianceSection.selected ? 1.0 : 0.45

                        Label {
                            Layout.fillWidth: true
                            text: "Variance floor"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        ComboBox {
                            Layout.preferredWidth: 150
                            model: ["Manual", "Auto"]
                            currentIndex: root.vm.local_contrast_minimum_variance_mode
                            onActivated: function(index) {
                                root.vm.local_contrast_minimum_variance_mode = index
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: localVarianceSection.selected
                            && localVarianceSection.manualFloorSelected
                            && !root.vm.busy
                        opacity: localVarianceSection.selected
                            && localVarianceSection.manualFloorSelected
                            ? 1.0 : 0.45

                        Label {
                            Layout.fillWidth: true
                            text: "Min variance"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        TextField {
                            id: localContrastMinimumVarianceField
                            Layout.preferredWidth: 108
                            horizontalAlignment: Text.AlignRight
                            selectByMouse: true
                            validator: DoubleValidator {
                                bottom: 0.000001
                                top: 65025
                                notation: DoubleValidator.StandardNotation
                                locale: Qt.locale().name
                            }

                            function syncFromViewModel() {
                                if (!activeFocus)
                                    forceSyncFromViewModel()
                            }

                            function forceSyncFromViewModel() {
                                text = Number(
                                    root.vm.local_contrast_minimum_variance
                                ).toLocaleString(Qt.locale(), "g", 6)
                            }

                            Component.onCompleted: forceSyncFromViewModel()

                            onEditingFinished: {
                                if (acceptableInput) {
                                    root.vm.local_contrast_minimum_variance =
                                        Number.fromLocaleString(Qt.locale(), text)
                                }
                                forceSyncFromViewModel()
                            }

                            Connections {
                                target: root.vm
                                function onLocal_contrast_changed() {
                                    localContrastMinimumVarianceField
                                        .syncFromViewModel()
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: localVarianceSection.selected
                            && !localVarianceSection.manualFloorSelected
                            && !root.vm.busy
                        opacity: localVarianceSection.selected
                            && !localVarianceSection.manualFloorSelected
                            ? 1.0 : 0.45

                        Label {
                            Layout.fillWidth: true
                            text: "Auto quantile"
                            color: "#ddd"
                            elide: Text.ElideRight
                        }

                        TextField {
                            id: localContrastAutoQuantileField
                            Layout.preferredWidth: 108
                            horizontalAlignment: Text.AlignRight
                            selectByMouse: true
                            validator: DoubleValidator {
                                bottom: 0.01
                                top: 0.5
                                notation: DoubleValidator.StandardNotation
                                locale: Qt.locale().name
                            }

                            function syncFromViewModel() {
                                if (!activeFocus)
                                    forceSyncFromViewModel()
                            }

                            function forceSyncFromViewModel() {
                                text = Number(
                                    root.vm.local_contrast_auto_quantile
                                ).toLocaleString(Qt.locale(), "f", 2)
                            }

                            Component.onCompleted: forceSyncFromViewModel()

                            onEditingFinished: {
                                if (acceptableInput) {
                                    root.vm.local_contrast_auto_quantile =
                                        Number.fromLocaleString(Qt.locale(), text)
                                }
                                forceSyncFromViewModel()
                            }

                            Connections {
                                target: root.vm
                                function onLocal_contrast_changed() {
                                    localContrastAutoQuantileField
                                        .syncFromViewModel()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
