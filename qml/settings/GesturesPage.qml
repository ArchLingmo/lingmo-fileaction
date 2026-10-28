/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

PageScroll {
    id: page
    title: qsTr("Gestures and keys")

    readonly property var wheelChoices: [
        { text: qsTr("Convert"), value: "convert" },
        { text: qsTr("Tools"), value: "tools" },
        { text: qsTr("Actions"), value: "actions" },
        { text: qsTr("Nothing"), value: "" }
    ]

    LingmoUI.Card {
        id: dragCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: dragCard.padding
            anchors.rightMargin: dragCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            SettingRow {
                title: qsTr("Drag gesture")
                subtitle: qsTr("While dragging a file, a quick tap on a key opens a wheel under the pointer. Holding the key (copy or move in a file manager) is not a tap.")
                Switch {
                    checked: prefs.gestureEnabled
                    onToggled: prefs.gestureEnabled = checked
                }
            }
            SettingRow {
                enabled: prefs.gestureEnabled
                opacity: enabled ? 1 : 0.5
                title: qsTr("Tap time")
                subtitle: qsTr("Press and release within this time. Longer is easier, shorter avoids opening the wheel by accident.")
                Slider {
                    id: tapSlider
                    Layout.preferredWidth: 180
                    from: 150; to: 800; stepSize: 50
                    snapMode: Slider.SnapAlways
                    value: prefs.tapMs
                    onMoved: prefs.tapMs = value
                }
                Label {
                    Layout.preferredWidth: 60
                    text: qsTr("%1 ms").arg(Math.round(tapSlider.value))
                    color: LingmoUI.Theme.textColor
                    font.pointSize: LingmoUI.Units.fontBody
                }
            }
        }
    }

    LingmoUI.Card {
        id: keysCard
        Layout.fillWidth: true
        enabled: prefs.gestureEnabled
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: keysCard.padding
            anchors.rightMargin: keysCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing
            opacity: keysCard.enabled ? 1 : 0.5

            Label {
                text: qsTr("Which key opens which wheel")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            Repeater {
                model: [
                    { key: "shift", name: "Shift" },
                    { key: "ctrl", name: "Ctrl" },
                    { key: "alt", name: "Alt" }
                ]
                delegate: SettingRow {
                    required property var modelData
                    title: qsTr("Tap %1").arg(modelData.name)
                    ComboBox {
                        Layout.preferredWidth: 180
                        model: page.wheelChoices
                        textRole: "text"
                        valueRole: "value"
                        currentIndex: {
                            prefs.revision
                            return indexOfValue(prefs.wheelForKey(modelData.key))
                        }
                        onActivated: prefs.setWheelForKey(modelData.key, currentValue)
                    }
                }
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("On the wheel, tapping another key switches wheels; the same key again repeats the last action.")
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
                wrapMode: Text.Wrap
            }
        }
    }

    LingmoUI.Card {
        id: shortcutCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: shortcutCard.padding
            anchors.rightMargin: shortcutCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            SettingRow {
                title: qsTr("Keyboard shortcut")
                subtitle: qsTr("Opens the wheel for the files selected in Lingmo Files, or the files or picture on the clipboard.")

                // click, then press the new keys (Esc cancels)
                Button {
                    id: capture
                    property bool listening: false
                    Layout.preferredWidth: 180
                    text: listening ? qsTr("Press the keys…") : prefs.shortcut
                    highlighted: listening
                    onClicked: {
                        listening = true
                        shortcutError.text = ""
                        forceActiveFocus()
                    }
                    onActiveFocusChanged: if (!activeFocus) listening = false
                    Keys.onPressed: (event) => {
                        if (!listening)
                            return
                        event.accepted = true
                        if (event.key === Qt.Key_Escape) {
                            listening = false
                            return
                        }
                        const seq = prefs.sequenceFor(event.key, event.modifiers)
                        if (seq.length === 0)
                            return          // a modifier alone: wait for the key
                        listening = false
                        shortcutError.text = prefs.setShortcut(seq)
                    }
                }
                LingmoUI.IconButton {
                    iconName: "edit-undo"
                    tipText: qsTr("Back to Meta+Shift+A")
                    visible: prefs.shortcut !== "Meta+Shift+A"
                    onClicked: shortcutError.text = prefs.setShortcut("Meta+Shift+A")
                }
            }
            Label {
                id: shortcutError
                Layout.fillWidth: true
                visible: text.length > 0
                color: LingmoUI.Theme.errorColor
                font.pointSize: LingmoUI.Units.fontCaption
                wrapMode: Text.Wrap
            }
        }
    }
}
