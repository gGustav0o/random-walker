import QtQuick 2.15

Rectangle {
    id: baseImage
    color: "transparent"

    property var vm: BaseImageViewModel

    Image {
        id: image
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        source: (vm.image_source !== "" && vm.image_loaded)
          ? vm.image_source + "?v=" + vm.image_version
          : ""
        visible: vm.image_loaded
        smooth: true
    }

    Text {
        anchors.centerIn: parent
        text: "No image"
        visible: !vm.image_loaded
        color: "#555"
        font.pixelSize: 18
        z: 1
    }
}
