/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// The radial wheel itself: the glass ring, its slices and the middle. Used by
// the wheel window (Wheel.qml) and by the live preview in Settings › Wheels.
Item {
    id: ring

    property var slices: []
    // -1 nothing, -2 the middle, else a slice
    property int selected: -1
    property bool showNumbers: false
    property string centerTitle
    property string centerSubtitle
    property bool centerBig: false
    property bool loading: false
    property bool cancelling: false
    // shown under the middle when there are no slices
    property string emptyText
    // the KWin blur behind the glass (the wheel window); off inside a normal window
    property bool blur: true
    // a solid ring instead of the glass (Settings › General › Glass wheel)
    property bool solid: false

    readonly property int count: slices.length
    // a bigger ring when many slices share it (an image has 12 tools)
    readonly property real outerRadius: count > 8 ? 200 : 168
    readonly property real innerRadius: 58
    readonly property real margin: 28            // room for the soft shadow
    readonly property real ringSize: outerRadius * 2
    readonly property real cx: margin + outerRadius
    readonly property real cy: margin + outerRadius
    readonly property real step: count > 0 ? 360 / count : 360

    implicitWidth: ringSize + margin * 2
    implicitHeight: ringSize + margin * 2

    // The ring: one round glass surface
    LingmoUI.GlassSurface {
        id: glass
        x: ring.margin
        y: ring.margin
        width: ring.ringSize
        height: ring.ringSize
        radius: ring.outerRadius
        blurMode: ring.blur ? LingmoUI.GlassSurface.Always : LingmoUI.GlassSurface.Never
        elevated: true
        solid: ring.solid
    }

    // Slices
    Repeater {
        model: ring.slices
        delegate: Item {
            id: slice
            required property int index
            required property var modelData
            readonly property real a0: (-90 - ring.step / 2 + index * ring.step + (ring.count > 1 ? 0.8 : 0)) * Math.PI / 180
            readonly property real a1: (-90 - ring.step / 2 + (index + 1) * ring.step - (ring.count > 1 ? 0.8 : 0)) * Math.PI / 180
            readonly property real mid: (a0 + a1) / 2
            readonly property real rIn: ring.innerRadius + 6
            readonly property real rOut: ring.outerRadius - 7
            readonly property real rMid: rIn + (rOut - rIn) * 0.55
            readonly property bool active: ring.selected === index
            readonly property bool available: modelData.available !== false
            anchors.fill: parent

            readonly property color fill: active ? LingmoUI.Theme.highlightColor
                                                 : Qt.rgba(LingmoUI.Theme.textColor.r, LingmoUI.Theme.textColor.g,
                                                           LingmoUI.Theme.textColor.b, LingmoUI.Theme.darkMode ? 0.06 : 0.045)

            Shape {
                anchors.fill: parent
                visible: ring.count > 1
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeWidth: -1
                    fillColor: slice.fill
                    startX: ring.cx + slice.rOut * Math.cos(slice.a0)
                    startY: ring.cy + slice.rOut * Math.sin(slice.a0)
                    PathArc {
                        x: ring.cx + slice.rOut * Math.cos(slice.a1)
                        y: ring.cy + slice.rOut * Math.sin(slice.a1)
                        radiusX: slice.rOut; radiusY: slice.rOut
                        useLargeArc: ring.step > 180
                    }
                    PathLine {
                        x: ring.cx + slice.rIn * Math.cos(slice.a1)
                        y: ring.cy + slice.rIn * Math.sin(slice.a1)
                    }
                    PathArc {
                        x: ring.cx + slice.rIn * Math.cos(slice.a0)
                        y: ring.cy + slice.rIn * Math.sin(slice.a0)
                        radiusX: slice.rIn; radiusY: slice.rIn
                        direction: PathArc.Counterclockwise
                        useLargeArc: ring.step > 180
                    }
                }
                Behavior on opacity { NumberAnimation { duration: LingmoUI.Units.shortDuration } }
            }
            // a single slice (repeat) is the whole ring
            Rectangle {
                visible: ring.count === 1
                x: ring.cx - slice.rOut
                y: ring.cy - slice.rOut
                width: slice.rOut * 2
                height: width
                radius: width / 2
                color: "transparent"
                border.width: slice.rOut - slice.rIn
                border.color: slice.fill
            }

            Column {
                readonly property real lx: ring.count === 1 ? ring.cx : ring.cx + slice.rMid * Math.cos(slice.mid)
                readonly property real ly: ring.count === 1 ? ring.cy - slice.rMid : ring.cy + slice.rMid * Math.sin(slice.mid)
                x: lx - width / 2
                y: ly - height / 2
                width: Math.min(100, 2 * slice.rMid * Math.sin(Math.min(Math.PI / 2, (slice.a1 - slice.a0) / 2)) + 10)
                spacing: 0
                opacity: slice.available ? 1 : 0.45

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !!slice.modelData.icon
                    source: slice.modelData.icon ? "image://icontheme/" + slice.modelData.icon : ""
                    sourceSize: Qt.size(22, 22)
                    width: visible ? 22 : 0
                    height: visible ? 22 : 0
                }
                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: slice.modelData.label
                    color: slice.active ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                    font.pointSize: slice.modelData.kind === "op" && slice.modelData.wheel === "convert"
                                    ? LingmoUI.Units.fontSubheading : LingmoUI.Units.fontBody
                    font.weight: Font.DemiBold
                    // long words shrink to the slice instead of breaking in the middle
                    fontSizeMode: Text.HorizontalFit
                    minimumPointSize: LingmoUI.Units.fontCaption - 1
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                Label {
                    width: parent.width
                    visible: !!slice.modelData.subtitle
                    horizontalAlignment: Text.AlignHCenter
                    text: slice.modelData.subtitle || ""
                    color: slice.active ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.secondaryTextColor
                    font.pointSize: LingmoUI.Units.fontCaption
                    fontSizeMode: Text.HorizontalFit
                    minimumPointSize: LingmoUI.Units.fontCaption - 1
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
            }

            // number for the keyboard (1-9)
            Label {
                visible: ring.showNumbers && slice.index < 9 && ring.count > 1
                readonly property real r: ring.outerRadius - 14
                x: ring.cx + r * Math.cos(slice.mid) - width / 2
                y: ring.cy + r * Math.sin(slice.mid) - height / 2
                text: slice.index + 1
                color: slice.active ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption - 1
                opacity: 0.8
            }
        }
    }

    // The middle: what is being converted, or "cancel" under the pointer
    Rectangle {
        id: hub
        x: ring.cx - width / 2
        y: ring.cy - height / 2
        width: (ring.innerRadius - 2) * 2
        height: width
        radius: width / 2
        // nothing to cancel in an empty drop zone
        readonly property bool cancelling: ring.cancelling
        color: cancelling ? Qt.rgba(LingmoUI.Theme.redColor.r, LingmoUI.Theme.redColor.g, LingmoUI.Theme.redColor.b, 0.18)
                          : Qt.rgba(LingmoUI.Theme.textColor.r, LingmoUI.Theme.textColor.g, LingmoUI.Theme.textColor.b,
                                    LingmoUI.Theme.darkMode ? 0.08 : 0.05)
        border.width: 1
        border.color: LingmoUI.Theme.glassBorderColor
        Behavior on color { ColorAnimation { duration: LingmoUI.Units.shortDuration } }

        Column {
            anchors.centerIn: parent
            width: parent.width - 14
            spacing: 1
            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: ring.loading && !hub.cancelling
                running: visible
                width: 26; height: 26
            }
            Label {
                width: parent.width
                visible: !ring.loading || hub.cancelling
                horizontalAlignment: Text.AlignHCenter
                text: hub.cancelling ? "✕" : ring.centerTitle
                color: hub.cancelling ? LingmoUI.Theme.redColor : LingmoUI.Theme.textColor
                font.pointSize: hub.cancelling || ring.centerBig ? LingmoUI.Units.fontHeading : LingmoUI.Units.fontBody
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: hub.cancelling ? qsTr("Cancel") : ring.centerSubtitle
                visible: text.length > 0
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideMiddle
            }
        }
    }

    // Nothing to offer (an unknown file type) or the engine failed
    Label {
        visible: ring.emptyText.length > 0 && !ring.loading && ring.count === 0
        x: ring.cx - width / 2
        y: ring.cy + ring.innerRadius + 18
        width: ring.outerRadius * 1.3
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        text: ring.emptyText
        color: LingmoUI.Theme.secondaryTextColor
        font.pointSize: LingmoUI.Units.fontCaption
    }
}
