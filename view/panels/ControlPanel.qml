import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#222"

    property var vm: SegmentationViewModel

    FileDialog {
        id: openImageDialog
        title: "Open Image"
        nameFilters: [
            "Image files (*.png *.jpg *.jpeg *.bmp *.tiff)",
            "All files (*)"
        ]
        onAccepted: {
            if (selectedFile)
                root.vm.open_image(selectedFile)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: "Open"
                enabled: !root.vm.busy
                onClicked: openImageDialog.open()
            }

            Button {
                text: "Clear image"
                enabled: root.vm.image_loaded && !root.vm.busy
                onClicked: root.vm.clear()
            }

            Button {
                text: "Clear seeds"
                enabled: (root.vm.background_seed_count > 0
                          || root.vm.object_seed_count > 0)
                         && !root.vm.busy
                onClicked: root.vm.clear_seeds()
            }

            ComboBox {
                id: labelSelector
                enabled: root.vm.image_loaded && !root.vm.busy
                model: ["Background", "Object"]
                currentIndex: root.vm.selected_label
                onActivated: function(index) {
                    root.vm.selected_label = index
                }
            }

            Label {
                text: "Beta:"
                color: "#ddd"
            }

            Slider {
                id: betaSlider
                Layout.preferredWidth: 140
                from: 0
                to: 1
                enabled: !root.vm.busy
                value: root.vm.beta_slider_position
                onMoved: root.vm.beta_slider_position = value
            }

            TextField {
                id: betaField
                Layout.preferredWidth: 92
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

            Button {
                text: root.vm.busy ? "Running..." : "Run segmentation"
                enabled: root.vm.can_run
                onClicked: root.vm.run_segmentation()
            }

            Label {
                text: "Background: " + root.vm.background_seed_count
                    + "  Object: " + root.vm.object_seed_count
                color: "#ddd"
            }

            Label {
                Layout.fillWidth: true
                text: root.vm.error_message
                color: "#ff7777"
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.vm.busy
            spacing: 10

            Label {
                Layout.preferredWidth: 220
                text: root.vm.status_text
                color: "#ddd"
                elide: Text.ElideRight
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: root.vm.progress_fraction
                indeterminate: root.vm.progress_indeterminate
            }

            Label {
                Layout.preferredWidth: 48
                horizontalAlignment: Text.AlignRight
                visible: !root.vm.progress_indeterminate
                text: Math.round(root.vm.progress_fraction * 100) + "%"
                color: "#ddd"
            }
        }
    }
}
