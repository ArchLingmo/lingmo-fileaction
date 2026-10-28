/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

PageScroll {
    id: page
    title: qsTr("General")

    // "Shift" / "Ctrl" / "Alt" for a wheel, as set on the Gestures page
    function keyOf(wheel) {
        const k = prefs.keyLabels[wheel] || ""
        return k === "⇧" ? "Shift" : k
    }

    LingmoUI.Banner {
        Layout.fillWidth: true
        type: LingmoUI.Banner.Error
        text: prefs.settingsError
        closable: false
    }

    // How to use: short on purpose (also the text of the system help)
    LingmoUI.Card {
        id: help
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: help.padding
            anchors.rightMargin: help.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Label {
                text: qsTr("How to use")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            Repeater {
                model: [
                    { icon: "transform-move", text: qsTr("Drag a file and, while dragging, tap a key: %1 converts, %2 opens the tools, %3 the actions. Drop the file on a slice; the result is saved next to the original.")
                          .arg(page.keyOf("convert") || "–").arg(page.keyOf("tools") || "–").arg(page.keyOf("actions") || "–") },
                    { icon: "input-mouse", text: qsTr("In Lingmo Files, right-click a file and choose Convert… or File actions…") },
                    { icon: "input-keyboard", text: qsTr("%1 opens the wheel for the files selected in Lingmo Files or copied to the clipboard. Arrows choose, Enter runs, Tab switches wheels.").arg(prefs.shortcut) }
                ]
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: LingmoUI.Units.largeSpacing
                    LingmoUI.IconItem {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: LingmoUI.Units.iconSizes.smallMedium
                        Layout.preferredHeight: LingmoUI.Units.iconSizes.smallMedium
                        source: modelData.icon
                        color: LingmoUI.Theme.highlightColor
                    }
                    Label {
                        Layout.fillWidth: true
                        text: modelData.text
                        wrapMode: Text.Wrap
                        color: LingmoUI.Theme.textColor
                        font.pointSize: LingmoUI.Units.fontBody
                    }
                }
            }
        }
    }

    LingmoUI.Card {
        id: startCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: startCard.padding
            anchors.rightMargin: startCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            SettingRow {
                title: qsTr("Start with the session")
                subtitle: qsTr("Needed for the drag gesture. Turned off, the wheel still opens from the menu, the shortcut and Lingmo Files. Takes effect at the next login.")
                Switch {
                    checked: prefs.autostart
                    onToggled: prefs.autostart = checked
                }
            }
        }
    }

    LingmoUI.Card {
        id: outCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: outCard.padding
            anchors.rightMargin: outCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Label {
                text: qsTr("Where results go")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            RadioButton {
                text: qsTr("Next to the original, with a new name")
                checked: prefs.outputDirectory.length === 0
                onToggled: if (checked) prefs.outputDirectory = ""
            }
            RowLayout {
                Layout.fillWidth: true
                RadioButton {
                    id: folderRadio
                    text: qsTr("In a folder:")
                    checked: prefs.outputDirectory.length > 0
                    onToggled: if (checked && prefs.outputDirectory.length === 0) folderDialog.open()
                }
                Label {
                    Layout.fillWidth: true
                    text: prefs.outputDirectory.length > 0 ? prefs.outputDirectory : qsTr("none chosen")
                    elide: Text.ElideMiddle
                    color: prefs.outputDirectory.length > 0 ? LingmoUI.Theme.textColor : LingmoUI.Theme.secondaryTextColor
                    font.pointSize: LingmoUI.Units.fontBody
                }
                LingmoUI.ActionButton {
                    text: qsTr("Choose…")
                    onClicked: folderDialog.open()
                }
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("The original is never changed or overwritten.")
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
                wrapMode: Text.Wrap
            }
        }
    }

    LingmoUI.Card {
        id: afterCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: afterCard.padding
            anchors.rightMargin: afterCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Label {
                text: qsTr("After finishing")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            SettingRow {
                title: qsTr("Show in the Island")
                subtitle: qsTr("Progress, Cancel, and Open or Show in folder when it is done. Errors are always shown.")
                Switch {
                    checked: prefs.showInIsland
                    onToggled: prefs.showInIsland = checked
                }
            }
            SettingRow {
                title: qsTr("Open the folder")
                subtitle: qsTr("Shows the result selected in Lingmo Files.")
                Switch {
                    checked: prefs.openFolder
                    onToggled: prefs.openFolder = checked
                }
            }
        }
    }

    LingmoUI.Card {
        id: lookCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: lookCard.padding
            anchors.rightMargin: lookCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Label {
                text: qsTr("Appearance")
                font.pointSize: LingmoUI.Units.fontSubheading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            SettingRow {
                title: qsTr("Glass wheel")
                subtitle: qsTr("The wheel is translucent and blurs what is behind it. Turned off, it is solid.")
                Switch {
                    checked: prefs.wheelGlass
                    onToggled: prefs.wheelGlass = checked
                }
            }
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Folder for the results")
        onAccepted: prefs.outputDirectory = selectedFolder.toString()
        onRejected: folderRadio.checked = prefs.outputDirectory.length > 0
    }
}
