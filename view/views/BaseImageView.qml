import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "#222"

    property var vm: SegmentationViewModel

    readonly property real contentX:
        image.x + (image.width - image.paintedWidth) / 2
    readonly property real contentY:
        image.y + (image.height - image.paintedHeight) / 2

    Image {
        id: image
        anchors.fill: parent
        anchors.margins: 12
        fillMode: Image.PreserveAspectFit
        source: vm.image_loaded
            ? vm.image_source + "?v=" + vm.image_version
            : ""
        visible: vm.image_loaded
        cache: false
        smooth: true
    }

    ResultOverlay {
        vm: root.vm
        x: root.contentX
        y: root.contentY
        width: image.paintedWidth
        height: image.paintedHeight
        z: 1
    }

    SeedLayer {
        anchors.fill: parent
        visible: vm.image_loaded
        seedModel: vm.seed_model
        imageX: root.contentX
        imageY: root.contentY
        imageWidth: image.paintedWidth
        imageHeight: image.paintedHeight
        sourceWidth: vm.image_width
        sourceHeight: vm.image_height
        z: 2
    }

    RectSeedMouseArea {
        anchors.fill: parent
        enabled: vm.image_loaded && !vm.busy
        imageX: root.contentX
        imageY: root.contentY
        imageWidth: image.paintedWidth
        imageHeight: image.paintedHeight
        sourceWidth: vm.image_width
        sourceHeight: vm.image_height
        selectedLabel: vm.selected_label
        z: 3

        onRectangleCreated: function(x, y, width, height) {
            vm.add_seed_rectangle(x, y, width, height)
        }
    }

    Text {
        anchors.centerIn: parent
        text: "No image"
        visible: !vm.image_loaded
        color: "#888"
        font.pixelSize: 18
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: vm.busy
        visible: running
        z: 20
    }
}
