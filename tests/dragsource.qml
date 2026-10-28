// A drag source for the gesture test: a tile that starts a real XDND drag of
// the file in $FA_DRAG_FILE (a file:// URL), like a file manager does.
import QtQuick
import QtQuick.Window

Window {
    x: 0; y: 0
    width: 300; height: 220
    visible: true
    color: "#dfe6f2"
    title: "drag source"

    Rectangle {
        id: tile
        x: 40; y: 40
        width: 140; height: 100
        radius: 10
        color: "#2463eb"
        Text { anchors.centerIn: parent; color: "white"; text: "photo.png" }

        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction
        Drag.proposedAction: Qt.CopyAction
        Drag.mimeData: ({ "text/uri-list": Qt.application.arguments[Qt.application.arguments.length - 1] + "\r\n" })
        Drag.active: area.drag.active
        Drag.onDragFinished: (action) => console.log("drag finished, action", action)

        MouseArea {
            id: area
            anchors.fill: parent
            drag.target: parent
            drag.threshold: 4
        }
    }
}
