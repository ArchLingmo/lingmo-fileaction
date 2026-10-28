/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Window
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// The radial wheel: Lingmo glass, always on top, at the pointer. Files are
// dropped on a slice (XDND, from any app), or a slice is clicked / chosen with
// the keyboard when the files are known already. The middle cancels.
//
//   drag:      drop on a slice runs it, drop in the middle cancels
//   keys:      arrows choose, Enter runs, Tab switches wheel, Esc closes,
//              1-9 run a slice, R repeats the last action
//   in a drag: the same keys arrive through the gesture daemon (the drag
//              source holds the keyboard); tapping Shift/Ctrl/Alt switches
//              wheels, the same key again repeats
Window {
    id: win

    readonly property real outerRadius: ring.outerRadius
    readonly property real innerRadius: ring.innerRadius
    readonly property real margin: ring.margin
    readonly property real tabsHeight: 44
    readonly property real ringSize: ring.ringSize
    readonly property real cx: ring.cx
    readonly property real cy: ring.cy

    readonly property var slices: fa.slices
    readonly property int count: slices.length
    readonly property real step: count > 0 ? 360 / count : 360
    // -1 nothing, -2 the middle (cancel), else a slice
    property int selected: -1
    property bool dragInside: false
    property bool solid: false

    width: ringSize + margin * 2
    height: ringSize + margin * 2 + tabsHeight
    color: "transparent"
    title: qsTr("File actions")
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | (fa.dragMode ? Qt.WindowDoesNotAcceptFocus : 0)
    visible: false

    readonly property var wheelNames: ({
        convert: qsTr("Convert"),
        tools: qsTr("Tools"),
        actions: qsTr("Actions")
    })

    function place() {
        const p = fa.anchor
        let geo = null
        for (let i = 0; i < Qt.application.screens.length; ++i) {
            const s = Qt.application.screens[i]
            if (p.x >= s.virtualX && p.x < s.virtualX + s.width && p.y >= s.virtualY && p.y < s.virtualY + s.height) {
                geo = s
                break
            }
        }
        if (!geo)
            geo = { virtualX: 0, virtualY: 0, width: Screen.width, height: Screen.height }
        // the pointer lands in the middle of the ring (the cancel zone)
        x = Math.max(geo.virtualX, Math.min(p.x - cx, geo.virtualX + geo.width - width))
        y = Math.max(geo.virtualY, Math.min(p.y - cy, geo.virtualY + geo.height - height))
    }

    // Slice under (px, py) in window coordinates
    function hit(px, py) {
        const dx = px - cx, dy = py - cy
        const r = Math.sqrt(dx * dx + dy * dy)
        if (r < innerRadius)
            return -2
        if (r > outerRadius + margin * 0.5 || count === 0)
            return -1
        let a = Math.atan2(dy, dx) * 180 / Math.PI + 90 + step / 2   // 0 at the top slice's start
        a = ((a % 360) + 360) % 360
        return Math.min(count - 1, Math.floor(a / step))
    }

    function run(index, urls) {
        if (index >= 0 && index < count)
            fa.activate(index, urls || [])
        else
            fa.close()
    }

    function handleKey(key) {
        if (key === "escape")
            fa.close()
        else if (key === "tab")
            fa.cycleWheel(1)
        else if (key === "r")
            fa.repeat()
        else if (/^[1-9]$/.test(key) && fa.files.length > 0)
            run(parseInt(key) - 1)
    }

    Connections {
        target: fa
        function onVisibleChanged() {
            if (fa.visible) {
                win.selected = -1
                win.solid = !prefs.wheelGlass
                win.place()
                win.show()
                win.raise()
                if (!fa.dragMode)
                    win.requestActivate()
                content.forceActiveFocus()
                nudge.restart()
            } else {
                win.hide()
            }
        }
        function onKeyDuringDrag(key) { win.handleKey(key) }
        function onSlicesChanged() {
            if (fa.visible)
                console.debug(wheelLog, "slices", fa.repeatMode ? "repeat" : fa.wheel, JSON.stringify(fa.slices.map(s => s.id)))
            if (win.selected >= win.count)
                win.selected = -1
            if (!fa.dragMode && win.selected < 0 && win.count > 0)
                win.selected = 0
        }
    }

    LoggingCategory {
        id: wheelLog
        name: "lingmo.fileaction.wheel"
        defaultLogLevel: LoggingCategory.Info
    }

    // once mapped, so the drag source finds the wheel under the pointer
    Timer {
        id: nudge
        property int tries: 0
        interval: 80
        onTriggered: {
            fa.shown()
            // until the files arrive (the window may take a moment to be mapped)
            if (fa.dragMode && fa.files.length === 0 && ++tries < 6) {
                interval = 200
                restart()
            } else {
                tries = 0
                interval = 80
            }
        }
    }
    // keep the ring's middle on the pointer when it grows or shrinks
    onWidthChanged: if (visible) place()

    Item {
        id: content
        anchors.fill: parent
        focus: true
        opacity: fa.visible ? 1 : 0
        scale: fa.visible ? 1 : 0.88
        transformOrigin: Item.Center
        Behavior on opacity { NumberAnimation { duration: LingmoUI.Units.shortDuration; easing.type: LingmoUI.Units.easing } }
        Behavior on scale { NumberAnimation { duration: LingmoUI.Units.longDuration; easing.type: Easing.OutBack } }

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                fa.close()
            } else if (event.key === Qt.Key_Tab) {
                fa.cycleWheel(1)
            } else if (event.key === Qt.Key_Backtab) {
                fa.cycleWheel(-1)
            } else if (event.key === Qt.Key_Right || event.key === Qt.Key_Down) {
                if (win.count > 0)
                    win.selected = win.selected < 0 ? 0 : (win.selected + 1) % win.count
            } else if (event.key === Qt.Key_Left || event.key === Qt.Key_Up) {
                if (win.count > 0)
                    win.selected = win.selected < 0 ? win.count - 1 : (win.selected - 1 + win.count) % win.count
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                win.run(win.selected)
            } else if (event.key === Qt.Key_R) {
                fa.repeat()
            } else if (event.key >= Qt.Key_1 && event.key <= Qt.Key_9) {
                win.run(event.key - Qt.Key_1)
            } else {
                return
            }
            event.accepted = true
        }

        WheelRing {
            id: ring
            width: implicitWidth
            height: implicitHeight
            slices: win.slices
            selected: win.selected
            solid: win.solid
            showNumbers: !fa.dragMode
            loading: fa.loading
            cancelling: win.selected === -2 && (fa.files.length > 0 || fa.dragMode)
            centerTitle: fa.repeatMode ? "↻"
                       : (!fa.dragMode && fa.files.length === 0) ? "⇣"
                       : (win.wheelNames[fa.wheel] || "")
            centerBig: fa.repeatMode
            centerSubtitle: fa.summary
            emptyText: fa.files.length === 0 ? ""
                     : fa.error.length > 0 ? fa.error : qsTr("Nothing on this wheel for these files. Tab shows the others.")
        }

        // The three wheels and their keys (and Repeat)
        LingmoUI.GlassSurface {
            id: tabs
            anchors.horizontalCenter: parent.horizontalCenter
            y: win.margin + win.ringSize + 6
            width: tabRow.implicitWidth + 8
            height: 32
            radius: height / 2
            blurMode: LingmoUI.GlassSurface.Always
            elevated: true
            solid: win.solid

            Row {
                id: tabRow
                anchors.centerIn: parent
                spacing: 2
                Repeater {
                    // the keys chosen in Settings › Gestures and keys
                    model: [
                        { id: "convert", key: prefs.keyLabels["convert"] || "" },
                        { id: "tools", key: prefs.keyLabels["tools"] || "" },
                        { id: "actions", key: prefs.keyLabels["actions"] || "" },
                        { id: "repeat", key: "R" }
                    ]
                    delegate: Rectangle {
                        id: tab
                        required property var modelData
                        readonly property bool current: modelData.id === "repeat" ? fa.repeatMode
                                                                                  : (!fa.repeatMode && fa.wheel === modelData.id)
                        visible: modelData.id !== "repeat" || fa.canRepeat
                        height: 26
                        width: tabText.implicitWidth + 18
                        radius: height / 2
                        color: current ? LingmoUI.Theme.highlightColor
                                       : tabMouse.containsMouse ? LingmoUI.Theme.hoverColor : "transparent"
                        Behavior on color { ColorAnimation { duration: LingmoUI.Units.shortDuration } }
                        Label {
                            id: tabText
                            anchors.centerIn: parent
                            readonly property string name: tab.modelData.id === "repeat" ? qsTr("Repeat") : win.wheelNames[tab.modelData.id]
                            text: tab.modelData.key.length > 0 ? "<b>%1</b>  %2".arg(tab.modelData.key).arg(name) : name
                            textFormat: Text.StyledText
                            color: tab.current ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                            font.pointSize: LingmoUI.Units.fontCaption
                        }
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: tab.modelData.id === "repeat" ? fa.repeat() : fa.switchWheel(tab.modelData.id)
                        }
                    }
                }
                // Settings (not in a drag: the wheel takes no clicks then)
                LingmoUI.IconButton {
                    visible: !fa.dragMode
                    anchors.verticalCenter: parent.verticalCenter
                    size: 26
                    iconSize: 14
                    round: true
                    flat: true
                    iconName: "settings-configure"
                    tipText: qsTr("Settings")
                    onClicked: fa.openSettings()
                }
            }
        }

        // Pointer: hover chooses, click runs (when the files are known)
        MouseArea {
            anchors.fill: parent
            anchors.bottomMargin: win.tabsHeight
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onPositionChanged: (mouse) => { win.selected = win.hit(mouse.x, mouse.y) }
            onClicked: (mouse) => {
                if (mouse.button === Qt.RightButton) {
                    fa.close()
                    return
                }
                const i = win.hit(mouse.x, mouse.y)
                if (i >= 0 && fa.files.length > 0)
                    win.run(i)
                else if (i === -2 || i === -1)
                    fa.close()
            }
        }

        // Files dropped from any app (XDND): the slice under the drop runs, the middle cancels
        DropArea {
            anchors.fill: parent
            onEntered: (drag) => {
                win.dragInside = true
                if (drag.hasUrls)
                    fa.setDroppedFiles(drag.urls)
                drag.accept(Qt.CopyAction)
            }
            onPositionChanged: (drag) => {
                win.selected = win.hit(drag.x, drag.y)
                drag.accept(Qt.CopyAction)
            }
            onExited: {
                win.dragInside = false
                win.selected = -1
            }
            onDropped: (drop) => {
                win.dragInside = false
                const i = win.hit(drop.x, drop.y)
                // always a copy: the source must never delete its file
                drop.accept(Qt.CopyAction)
                if (i >= 0)
                    win.run(i, drop.hasUrls ? drop.urls : [])
                else
                    fa.close()
            }
        }
    }
}
