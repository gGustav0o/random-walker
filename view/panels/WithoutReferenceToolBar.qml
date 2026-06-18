import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    color: "#222"
    property var vm: WithoutReferenceToolBarViewModel
    visible: vm.visible

    Rectangle {
        id: gridContainer
        anchors.fill: parent
        anchors.margins: 16
        radius: 12
        color: "#333"
        width: grid.implicitWidth + 28
        height: grid.implicitHeight + 28

        property var actions: [
            { label: "Preprocess",       method: "on_preprocess" },
            { label: "ROI",              method: "on_select_roi" },
            { label: "Segment",          method: "on_segment" },
            { label: "Morphology",       method: "on_morphology" },
            { label: "Clean",            method: "on_cleanup" },
            { label: "Contour analusis", method: "on_contour_analysis" },
            { label: "Features",         method: "on_features" },
            { label: "Compare",          method: "on_compare" },
            { label: "Classify",         method: "on_classify" }
        ]

        property int columns: 1
        property int buttonCount: actions.length 
        property int rows: Math.ceil(buttonCount / columns)
        
        Grid {
            id: grid
            anchors.centerIn: parent
            columns: gridContainer.columns
            rowSpacing: 8
            columnSpacing: 8

            Repeater {
                model: gridContainer.buttonCount
                delegate: Button {
                    property int row: index % gridContainer.rows
                    property int col: Math.floor(index / gridContainer.rows)
                    property int trueIndex: row + col * gridContainer.rows
                    text: gridContainer.actions[trueIndex].label
                    enabled: root.vm.current_step === trueIndex
                    onClicked: root.vm[gridContainer.actions[trueIndex].method]()
                    width: 110
                }
            anchors.fill: parent
            }
        }
    }
}
