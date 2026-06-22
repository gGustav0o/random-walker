import QtQuick 2.15

Image {
    id: root

    required property var vm

    source: vm.has_result
        ? vm.result_source + "?v=" + vm.result_version
        : ""
    visible: vm.has_result
    cache: false
    smooth: false
    fillMode: Image.Stretch
}
