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

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
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
}
