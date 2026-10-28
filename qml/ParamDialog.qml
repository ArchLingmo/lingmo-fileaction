/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// The values a slice asks for before it runs (e.g. Trim: start and end),
// prefilled with the last ones used. A small glass card at the pointer.
Window {
    id: dialog

    property int sliceIndex: -1
    property var slice: ({})
    property var fields: []

    function open(index, s) {
        sliceIndex = index
        slice = s
        fields = s.parameters || []
        const p = fa.anchor
        x = Math.max(0, p.x - width / 2)
        y = Math.max(0, p.y - height / 2)
        show()
        raise()
        requestActivate()
        form.forceActiveFocus()
    }

    function accept() {
        const values = {}
        for (let i = 0; i < repeater.count; ++i) {
            const f = repeater.itemAt(i)
            values[f.key] = f.value
        }
        fa.runWithParameters(sliceIndex, values)
        close()
    }

    width: 340
    height: column.implicitHeight + LingmoUI.Units.largeSpacing * 3
    color: "transparent"
    title: slice.label || ""
    flags: Qt.Dialog | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    LingmoUI.GlassSurface {
        anchors.fill: parent
        radius: LingmoUI.Theme.hugeRadius
    }

    LingmoUI.WindowShadow {
        view: dialog
        geometry: Qt.rect(0, 0, dialog.width, dialog.height)
        radius: LingmoUI.Theme.hugeRadius
    }

    FocusScope {
        id: form
        anchors.fill: parent
        anchors.margins: LingmoUI.Units.largeSpacing * 1.5
        Keys.onEscapePressed: dialog.close()
        Keys.onReturnPressed: dialog.accept()
        Keys.onEnterPressed: dialog.accept()

        ColumnLayout {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: LingmoUI.Units.smallSpacing

            Label {
                text: (dialog.slice.label || "") + (dialog.slice.subtitle ? " · " + dialog.slice.subtitle : "")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            Label {
                Layout.fillWidth: true
                text: fa.summary
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
                elide: Text.ElideMiddle
            }

            Repeater {
                id: repeater
                model: dialog.fields
                delegate: ColumnLayout {
                    required property var modelData
                    required property int index
                    readonly property string key: modelData.key
                    readonly property alias value: field.text
                    Layout.fillWidth: true
                    Layout.topMargin: LingmoUI.Units.smallSpacing
                    spacing: 2
                    Label {
                        text: modelData.label
                        color: LingmoUI.Theme.textColor
                        font.pointSize: LingmoUI.Units.fontBody
                    }
                    TextField {
                        id: field
                        Layout.fillWidth: true
                        text: modelData.value || ""
                        placeholderText: modelData.hint || ""
                        focus: index === 0
                        selectByMouse: true
                    }
                    Label {
                        visible: !!modelData.hint
                        text: modelData.hint || ""
                        color: LingmoUI.Theme.secondaryTextColor
                        font.pointSize: LingmoUI.Units.fontCaption
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: LingmoUI.Units.largeSpacing
                spacing: LingmoUI.Units.smallSpacing
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Cancel")
                    onClicked: dialog.close()
                }
                Button {
                    text: qsTr("Run")
                    flat: false
                    highlighted: true
                    onClicked: dialog.accept()
                }
            }
        }
    }
}
