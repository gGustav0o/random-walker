import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#262626"
    clip: true

    property var vm: SegmentationViewModel
    property bool expanded: false
    readonly property real expandedWidth: 360
    property real panelWidth: expanded ? expandedWidth : 0

    Layout.preferredWidth: panelWidth
    Layout.minimumWidth: panelWidth
    Layout.maximumWidth: panelWidth
    Layout.fillHeight: true
    opacity: expanded ? 1 : 0
    enabled: expanded

    QtObject {
        id: help

        readonly property string connectivity:
            "Defines which neighboring pixels are connected in the Random Walker graph. "
            + "4-connectivity uses only horizontal and vertical neighbors. "
            + "8-connectivity also adds diagonals, usually producing smoother propagation through diagonal structures."

        readonly property string distancePower:
            "Geometric distance exponent in the edge weight denominator d^p. "
            + "p = 0 ignores edge length. With 8-connectivity, larger values weaken diagonal edges "
            + "relative to horizontal and vertical edges."

        readonly property string beta:
            "Global contrast sensitivity in the Random Walker edge weight exp(-beta * deltaI^2). "
            + "Higher beta weakens edges across intensity jumps more strongly, so boundaries follow contrast more tightly. "
            + "Lower beta allows labels to propagate more smoothly across weak contrast."

        readonly property string autoMarkerPolarity:
            "Selects which Gaussian mixture component is treated as the object: the darker component or the brighter component. "
            + "Use Dark object when the target is darker than the background, and Bright object when it is brighter."

        readonly property string autoMarkerConfidence:
            "Minimum posterior probability required for automatic marker candidates. "
            + "Higher values produce fewer, more conservative markers. Lower values produce more markers, "
            + "but can include unreliable candidates when object and background intensities overlap."

        readonly property string autoMarkerSafeMargin:
            "Minimum Euclidean distance from the edge of each high-confidence candidate region. "
            + "Increasing it keeps only the interior core of proposed markers. "
            + "Set to 0 to disable this distance-based cleanup."

        readonly property string autoMarkerMinimumArea:
            "Minimum connected-component area for automatic markers. Increasing it removes small noisy components. "
            + "Decreasing it allows small objects, but may also keep isolated artifacts."
    }

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
            Layout.preferredHeight: 1
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

                        SettingLabel {
                            text: "Connectivity"
                            helpText: help.connectivity
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

                        SettingLabel {
                            text: "Distance p"
                            helpText: help.distancePower
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

                        SettingLabel {
                            text: "Beta"
                            helpText: help.beta
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

                SettingsSection {
                    title: "Auto markers"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SettingLabel {
                            text: "Object polarity"
                            helpText: help.autoMarkerPolarity
                        }

                        ComboBox {
                            Layout.preferredWidth: 150
                            enabled: !root.vm.busy
                            model: ["Dark object", "Bright object"]
                            currentIndex: root.vm.auto_marker_foreground_polarity
                            onActivated: function(index) {
                                root.vm.auto_marker_foreground_polarity = index
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        enabled: !root.vm.busy

                        SettingLabel {
                            text: "Confidence"
                            helpText: help.autoMarkerConfidence
                        }

                        TextField {
                            id: autoMarkerConfidenceField
                            Layout.preferredWidth: 108
                            enabled: parent.enabled
                            horizontalAlignment: Text.AlignRight
                            selectByMouse: true
                            validator: DoubleValidator {
                                bottom: root.vm.minimum_auto_marker_confidence_threshold
                                top: root.vm.maximum_auto_marker_confidence_threshold
                                notation: DoubleValidator.StandardNotation
                                locale: Qt.locale().name
                            }

                            function syncFromViewModel() {
                                if (!activeFocus)
                                    forceSyncFromViewModel()
                            }

                            function forceSyncFromViewModel() {
                                text = Number(
                                    root.vm.auto_marker_confidence_threshold
                                ).toLocaleString(Qt.locale(), "f", 2)
                            }

                            Component.onCompleted: forceSyncFromViewModel()

                            onEditingFinished: {
                                if (acceptableInput) {
                                    root.vm.auto_marker_confidence_threshold =
                                        Number.fromLocaleString(
                                            Qt.locale(),
                                            text)
                                }
                                forceSyncFromViewModel()
                            }

                            Connections {
                                target: root.vm
                                function onAuto_marker_confidence_threshold_changed() {
                                    autoMarkerConfidenceField.syncFromViewModel()
                                }
                            }
                        }
                    }

                    Slider {
                        id: autoMarkerConfidenceSlider
                        Layout.fillWidth: true
                        from: root.vm.minimum_auto_marker_confidence_threshold
                        to: root.vm.maximum_auto_marker_confidence_threshold
                        stepSize: 0.01
                        enabled: !root.vm.busy
                        value: root.vm.auto_marker_confidence_threshold
                        onMoved: root.vm.auto_marker_confidence_threshold = value
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SettingLabel {
                            text: "Safe margin, px"
                            helpText: help.autoMarkerSafeMargin
                        }

                        SpinBox {
                            Layout.preferredWidth: 126
                            enabled: !root.vm.busy
                            editable: true
                            from: root.vm.minimum_auto_marker_boundary_distance
                            to: root.vm.maximum_auto_marker_boundary_distance
                            value: root.vm.auto_marker_minimum_boundary_distance
                            onValueModified: {
                                root.vm.auto_marker_minimum_boundary_distance =
                                    value
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        SettingLabel {
                            text: "Min area, px"
                            helpText: help.autoMarkerMinimumArea
                        }

                        SpinBox {
                            Layout.preferredWidth: 126
                            enabled: !root.vm.busy
                            editable: true
                            from: root.vm.minimum_auto_marker_component_area
                            to: root.vm.maximum_auto_marker_component_area
                            value: root.vm.auto_marker_minimum_component_area
                            onValueModified: {
                                root.vm.auto_marker_minimum_component_area =
                                    value
                            }
                        }
                    }
                }
            }
        }
    }
}
