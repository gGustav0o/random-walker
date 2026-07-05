import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    title: "About Random Walker"
    width: 760
    height: 540
    minimumWidth: 520
    minimumHeight: 360
    visible: false
    color: "#202020"

    palette.window: "#202020"
    palette.windowText: "#f0f0f0"
    palette.base: "#262626"
    palette.alternateBase: "#303030"
    palette.text: "#f0f0f0"
    palette.button: "#333333"
    palette.buttonText: "#f0f0f0"
    palette.highlight: "#4d7cff"
    palette.highlightedText: "#ffffff"

    property url algorithmPdfUrl: ""
    readonly property bool hasAlgorithmPdf: algorithmPdfUrl.toString().length > 0

    property int currentTabIndex: 0

    component DarkButton: Rectangle {
        id: buttonRoot

        property string text: ""
        signal clicked()

        implicitWidth: 112
        implicitHeight: 30
        color: enabled
            ? (buttonMouseArea.pressed ? "#3a3a3a" : "#333333")
            : "#2a2a2a"
        border.color: enabled && buttonMouseArea.containsMouse ? "#5a82ff" : "#4a4a4a"
        border.width: 1
        radius: 4

        Text {
            anchors.fill: parent
            anchors.margins: 8
            text: buttonRoot.text
            color: buttonRoot.enabled ? "#f0f0f0" : "#8a8a8a"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        MouseArea {
            id: buttonMouseArea
            anchors.fill: parent
            hoverEnabled: true
            enabled: buttonRoot.enabled
            onClicked: buttonRoot.clicked()
        }
    }

    component DarkTabButton: Rectangle {
        id: tabButtonRoot

        property string text: ""
        property bool checked: false
        signal clicked()

        Layout.fillWidth: true
        Layout.preferredHeight: 32
        color: checked ? "#343434" : "#282828"
        border.color: checked ? "#4d7cff" : "#3a3a3a"
        border.width: 1

        Text {
            anchors.fill: parent
            anchors.margins: 8
            text: tabButtonRoot.text
            color: tabButtonRoot.checked ? "#ffffff" : "#cfcfcf"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: tabButtonRoot.clicked()
        }
    }

    component InfoBlock: ColumnLayout {
        id: infoRoot

        property string title: ""
        property string body: ""

        Layout.fillWidth: true
        spacing: 4

        Label {
            Layout.fillWidth: true
            text: infoRoot.title
            color: "#f0f0f0"
            font.pixelSize: 14
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: infoRoot.body
            color: "#cfcfcf"
            wrapMode: Text.WordWrap
        }
    }

    component ReferenceItem: ColumnLayout {
        id: referenceRoot

        property string title: ""
        property string url: ""
        property string note: ""

        Layout.fillWidth: true
        spacing: 4

        Text {
            Layout.fillWidth: true
            text: '<a href="' + referenceRoot.url + '">' + referenceRoot.title + '</a>'
            textFormat: Text.RichText
            color: "#f0f0f0"
            linkColor: "#8fb0ff"
            wrapMode: Text.WordWrap
            onLinkActivated: function(link) {
                Qt.openUrlExternally(link)
            }
        }

        Label {
            Layout.fillWidth: true
            text: referenceRoot.note
            color: "#cfcfcf"
            wrapMode: Text.WordWrap
        }
    }

    function open() {
        show()
        raise()
        requestActivate()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: "Random Walker"
            color: "#f0f0f0"
            font.pixelSize: 20
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3a3a"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 0

            DarkTabButton {
                text: "Algorithm"
                checked: root.currentTabIndex === 0
                onClicked: root.currentTabIndex = 0
            }

            DarkTabButton {
                text: "References"
                checked: root.currentTabIndex === 1
                onClicked: root.currentTabIndex = 1
            }

            DarkTabButton {
                text: "Developers"
                checked: root.currentTabIndex === 2
                onClicked: root.currentTabIndex = 2
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#262626"
            border.color: "#3a3a3a"
            border.width: 1

            StackLayout {
                anchors.fill: parent
                anchors.margins: 14
                currentIndex: root.currentTabIndex

                ScrollView {
                    id: algorithmPage
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: algorithmPage.availableWidth
                        spacing: 14

                        Label {
                            Layout.fillWidth: true
                            text: "Algorithm description"
                            color: "#f0f0f0"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: root.hasAlgorithmPdf
                                ? "A detailed mathematical description is available as a bundled PDF."
                                : "A detailed mathematical PDF description will be added later."
                            color: "#cfcfcf"
                            wrapMode: Text.WordWrap
                        }

                        InfoBlock {
                            title: "Purpose"
                            body: "The program segments a grayscale image into object and background regions. The user provides a small set of trusted markers, and the algorithm propagates these labels through the image."
                        }

                        InfoBlock {
                            title: "Image as a graph"
                            body: "Each pixel is treated as a graph node. Neighboring pixels are connected by edges using either 4-connectivity or 8-connectivity. Strong edges connect similar pixels; weak edges make boundaries harder to cross."
                        }

                        InfoBlock {
                            title: "Markers as boundary conditions"
                            body: "Background and object markers are fixed labels. They define the boundary conditions that guide all unknown pixels."
                        }

                        InfoBlock {
                            title: "Edge weights"
                            body: "The Global beta model uses one contrast scale for the whole image. The Local variance model estimates a local contrast scale, which is usually more robust when different parts of the image have different noise or contrast."
                        }

                        InfoBlock {
                            title: "Solving"
                            body: "For every unmarked pixel, the algorithm computes how strongly it is connected to the object markers versus the background markers. Internally this is solved as a sparse linear system built from the graph Laplacian."
                        }

                        InfoBlock {
                            title: "Result"
                            body: "The solver produces a probability map. Pixels with probability at least 0.5 are shown as object; the rest are shown as background."
                        }

                        InfoBlock {
                            title: "Automatic markers"
                            body: "Automatic marker placement uses a two-component Gaussian mixture over intensities. Only high-confidence pixels are proposed as markers, then small or unstable connected regions are filtered out."
                        }

                        InfoBlock {
                            title: "Limitations"
                            body: "The method depends on meaningful markers and on edge weights that reflect real image boundaries. Weak contrast, heavy noise, or incorrect markers can produce ambiguous or incorrect regions."
                        }

                        DarkButton {
                            text: root.hasAlgorithmPdf ? "Open PDF" : "PDF is not attached"
                            enabled: root.hasAlgorithmPdf
                            onClicked: Qt.openUrlExternally(root.algorithmPdfUrl)
                        }
                    }
                }

                ScrollView {
                    id: referencesPage
                    clip: true
                    contentWidth: availableWidth

                    ColumnLayout {
                        width: referencesPage.availableWidth
                        spacing: 14

                        Label {
                            Layout.fillWidth: true
                            text: "References"
                            color: "#f0f0f0"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        ReferenceItem {
                            title: "Leo Grady, Random Walks for Image Segmentation, IEEE TPAMI, 2006"
                            url: "https://doi.org/10.1109/TPAMI.2006.233"
                            note: "Main theoretical basis: graph construction, seed boundary conditions, graph Laplacian, harmonic probabilities, and the classical Gaussian edge weight used by Global beta."
                        }

                        ReferenceItem {
                            title: "Peter G. Doyle, J. Laurie Snell, Random Walks and Electric Networks, 1984"
                            url: "https://math.dartmouth.edu/~doyle/docs/walks/walks.pdf"
                            note: "Mathematical background: hitting probabilities, harmonic functions, and the electrical-network interpretation behind solving the Dirichlet problem on a weighted graph."
                        }

                        ReferenceItem {
                            title: "Dominik Drees, Florian Eilers, Ang Bian, Xiaoyi Jiang, Noise Model-Aware Random Walker, 2022"
                            url: "https://arxiv.org/abs/2206.00947"
                            note: "Weight-model context: motivates moving beyond a fixed beta and treating edge weights as similarity functions derived from local/noise statistics."
                        }

                        ReferenceItem {
                            title: "A. P. Dempster, N. M. Laird, D. B. Rubin, Maximum Likelihood from Incomplete Data via the EM Algorithm, 1977"
                            url: "https://doi.org/10.1111/j.2517-6161.1977.tb01600.x"
                            note: "Auto-marker basis: EM estimation for latent-variable models, used here to fit the one-dimensional Gaussian mixture over image intensities."
                        }

                        ReferenceItem {
                            title: "Carsten Rother, Vladimir Kolmogorov, Andrew Blake, GrabCut, ACM TOG, 2004"
                            url: "https://www.microsoft.com/en-us/research/publication/grabcut-interactive-foreground-extraction-using-iterated-graph-cuts/"
                            note: "Related segmentation idea: uses GMMs for interactive foreground/background modelling; in this project GMM is used only to propose reliable Random Walker markers."
                        }

                        ReferenceItem {
                            title: "Azriel Rosenfeld, John L. Pfaltz, Sequential Operations in Digital Picture Processing, JACM, 1966"
                            url: "https://doi.org/10.1145/321356.321357"
                            note: "Auto-marker cleanup basis: connected-component processing of candidate marker pixels before small/noisy regions are rejected."
                        }
                    }
                }

                ColumnLayout {
                    id: developersPage
                    spacing: 12

                    Label {
                        Layout.fillWidth: true
                        text: "Developers"
                        color: "#f0f0f0"
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 10

                        TextEdit {
                            Layout.columnSpan: 2
                            Layout.fillWidth: true
                            text: "Gustavo"
                            color: "#f0f0f0"
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            readOnly: true
                            selectByMouse: true
                            selectedTextColor: "#ffffff"
                            selectionColor: "#4d7cff"
                            wrapMode: TextEdit.NoWrap
                        }

                        Label {
                            text: "Email"
                            color: "#a8a8a8"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: '<a href="mailto:g0329668@gmail.com">g0329668@gmail.com</a>'
                            textFormat: Text.RichText
                            color: "#f0f0f0"
                            linkColor: "#8fb0ff"
                            elide: Text.ElideRight
                            onLinkActivated: function(link) {
                                Qt.openUrlExternally(link)
                            }
                        }

                        Label {
                            text: "GitHub"
                            color: "#a8a8a8"
                        }

                        Text {
                            Layout.fillWidth: true
                            text: '<a href="https://github.com/gGustav0o">https://github.com/gGustav0o</a>'
                            textFormat: Text.RichText
                            color: "#f0f0f0"
                            linkColor: "#8fb0ff"
                            elide: Text.ElideRight
                            onLinkActivated: function(link) {
                                Qt.openUrlExternally(link)
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
