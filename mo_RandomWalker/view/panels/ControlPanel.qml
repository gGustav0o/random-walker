import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 
import QtQuick.Layouts 1.15

import App.Enums 1.0

Rectangle {
    width: 300; height: 100; color: "#222"
    property var vm: ControlPanelViewModel

    RowLayout  {
        anchors.margins: 8
        spacing: 10   
        anchors.fill: parent

        FileDialog {
            id: openImageDialog
            title: "Open Image"
            nameFilters: ["Image files (*.png *.jpg *.bmp *.tiff)", "All files (*)"]
            onAccepted: {
            if (selectedFile)
                vm.open_image(selectedFile)
            }
        }
        Button {
            text: "Open"
            onClicked: openImageDialog.open()
            enabled: !vm.image_loaded
        }
        Button {
            text: "Clear"
            onClicked: vm.clear()
            enabled: vm.image_loaded
        }
        Item { Layout.fillWidth: true }
    }
}