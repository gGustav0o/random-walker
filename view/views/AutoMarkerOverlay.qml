import QtQuick 2.15

Image {
    required property var vm

    source: vm.has_automatic_markers
        ? vm.automatic_marker_source + "?v=" + vm.automatic_marker_version
        : ""
    visible: vm.has_automatic_markers
    cache: false
    smooth: false
    fillMode: Image.Stretch
}
