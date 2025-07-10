import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 
import QtQuick.Layouts 1.15

import App.Enums 1.0
import App.ViewModel 1.0

Rectangle {
    width: 300; height: 100; color: "#222"
    property var vm: ControlPanelViewModel

    RowLayout  {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 10   

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
        }
        ComboBox {
            id: combo
            width: 120
            model: [
                { text: "Object", value: SeedLabel.Object },
                { text: "Background", value: SeedLabel.Background }
            ]
            textRole: "text"
            onCurrentIndexChanged: {
                if (currentIndex >= 0 && model[currentIndex])
                    vm.selected_type = model[currentIndex].value;
            }
            Component.onCompleted: {
                var idx = model.findIndex(function(item) { return item.value === vm.selected_type; });
                if (idx !== -1) currentIndex = idx;
            }
        }
        Button {
            text: "Clear seeds"
            onClicked: vm.clear_seeds()
        }
        Button {
            text: "Clear"
            onClicked: vm.clear()
        }
        Button {
            text: "Start"
            onClicked: vm.start()
        }
        Item { Layout.fillWidth: true }
    }

    Connections {
        target: vm
        function onSelected_type_changed() {
            var idx = combo.model.findIndex(function(item) { return item.value === vm.selected_type; })
            if (combo.currentIndex !== idx) combo.currentIndex = idx
        }
    }
}