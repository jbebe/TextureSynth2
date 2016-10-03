import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {

    property int windowWidth: 700
    property int windowHeight: 390
    property string backgroundColor: "#333333"

    id: mainWindow
    visible: true
    width: windowWidth
    height: windowHeight
    title: qsTr("Hello World")
    color: backgroundColor

    ColumnLayout {
        id: columnLayout1
        width: 700
        height: 400

        Flickable {
            id: scrollablePanel
            width: 700
            height: 88
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            Layout.fillHeight: false
        }

        RowLayout {
            id: rowLayout1
            width: 100
            height: 0
            spacing: 1
            Layout.fillHeight: true
            Layout.fillWidth: true

            Item {
                id: settingsPanel
                width: 300
                height: 200
                Layout.fillHeight: true

                GridLayout {
                    id: settingsLayout
                    x: 8
                    y: 0
                    width: 292
                    height: 295
                    columnSpacing: 10
                    rowSpacing: 10
                    rows: 9
                    columns: 5

                    Label {
                        id: tileableLbl
                        color: "#ffffff"
                        text: qsTr("Tileable:")
                        font.pointSize: 10
                        font.family: "Tahoma"
                        Layout.columnSpan: 4
                    }

                    CheckBox {
                        id: tileableChb
                        text: qsTr("")
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        spacing: 0
                        bottomPadding: 0
                        rightPadding: 0
                        leftPadding: 0
                        topPadding: 0
                        Layout.fillHeight: false
                        Layout.fillWidth: false
                        Layout.preferredWidth: -1
                        Layout.preferredHeight: -1
                        Layout.columnSpan: 1
                        enabled: true
                    }

                    Label {
                        id: generationLbl
                        color: "#ffffff"
                        text: qsTr("Generation Mode:")
                        font.pointSize: 10
                        Layout.columnSpan: 2
                        Layout.rowSpan: 1
                    }

                    ComboBox {
                        id: generationCob
                        currentIndex: 3
                        textRole: "Hello"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        Layout.columnSpan: 3
                    }

                    Label {
                        id: outputSizeLbl
                        color: "#ffffff"
                        text: qsTr("Output size:")
                        font.pointSize: 10
                        Layout.columnSpan: 5
                    }

                    Label {
                        id: outputWidthLbl
                        color: "#ffffff"
                        text: qsTr("Width:")
                        font.pointSize: 10
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.columnSpan: 2
                    }

                    TextField {
                        id: outputWidthInput
                        color: "#ffffff"
                        text: qsTr("")
                        Layout.fillWidth: true
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 30
                    }

                    Label {
                        id: outputHeightLbl
                        color: "#ffffff"
                        text: qsTr("Height:")
                        font.pointSize: 10
                    }

                    TextField {
                        id: outputHeightInput
                        color: "#ffffff"
                        text: qsTr("")
                        Layout.fillWidth: true
                        Layout.preferredWidth: 60
                        Layout.preferredHeight: 30
                    }

                    Label {
                        id: similarityLbl
                        color: "#ffffff"
                        text: qsTr("Similarity threshold:")
                        font.pointSize: 10
                        Layout.columnSpan: 2
                    }

                    TextField {
                        id: similarityInput
                        color: "#ffffff"
                        text: qsTr("")
                        Layout.fillWidth: true
                        Layout.preferredWidth: -1
                        Layout.preferredHeight: 30
                        Layout.columnSpan: 3
                    }

                    Label {
                        id: searchSizeLbl
                        color: "#ffffff"
                        text: qsTr("Search size:")
                        font.pointSize: 10
                        Layout.columnSpan: 2
                    }

                    TextField {
                        id: searchSizeInput
                        color: "#ffffff"
                        text: qsTr("")
                        Layout.preferredHeight: 30
                        Layout.fillWidth: true
                        Layout.columnSpan: 3
                    }

                    Label {
                        id: initLbl
                        color: "#ffffff"
                        text: qsTr("Init method:")
                        font.pointSize: 10
                        Layout.columnSpan: 2
                    }

                    ComboBox {
                        id: initCob
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        Layout.columnSpan: 3
                    }

                    Button {
                        id: generateBtn
                        text: qsTr("Generate")
                        Layout.fillHeight: false
                        Layout.fillWidth: false
                        Layout.preferredHeight: 30
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.columnSpan: 5
                    }
                }
            }

            Item {
                id: resultViewer
                x: 300
                width: 300
                height: 200
                Layout.preferredHeight: 200
                Layout.fillWidth: true
                Layout.fillHeight: true

                GridLayout {
                    id: gridLayout1
                    width: 399
                    height: 307
                    rows: 3
                    columns: 3


                    Item {
                        id: item1
                        width: 200
                        height: 200
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.preferredHeight: 10
                        Layout.preferredWidth: -1
                    }

                    Item {
                        id: item2
                        width: 200
                        height: 200
                        Layout.preferredWidth: 10
                        Layout.fillHeight: true
                    }
                    Image {
                        id: image1
                        width: 100
                        height: 100
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        Layout.preferredHeight: 200
                        Layout.preferredWidth: 200
                        Layout.fillHeight: false
                        Layout.columnSpan: 1
                        Layout.fillWidth: false
                        source: "qrc:/icon.jpg"
                    }

                    Item {
                        id: item3
                        width: 200
                        height: 200
                        Layout.preferredWidth: 10
                        Layout.fillHeight: true
                    }

                    Item {
                        id: item4
                        width: 200
                        height: 200
                        Layout.preferredHeight: 10
                        Layout.columnSpan: 3
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
