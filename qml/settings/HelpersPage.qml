/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// The optional programs some slices need; without them those slices are dimmed
PageScroll {
    id: page
    title: qsTr("Helper programs")
    subtitle: qsTr("Pictures, audio and video need nothing else. These add more formats.")

    LingmoUI.Banner {
        Layout.fillWidth: true
        type: prefs.installOk ? LingmoUI.Banner.Success : LingmoUI.Banner.Warning
        text: prefs.installing.length === 0 ? prefs.installMessage : ""
        closable: false
    }

    Repeater {
        model: prefs.helpers
        delegate: LingmoUI.Card {
            id: card
            required property var modelData
            readonly property bool busy: prefs.installing === modelData.id
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: card.padding
                anchors.rightMargin: card.padding
                anchors.topMargin: LingmoUI.Units.largeSpacing
                anchors.bottomMargin: LingmoUI.Units.largeSpacing
                spacing: LingmoUI.Units.largeSpacing

                LingmoUI.IconItem {
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: LingmoUI.Units.iconSizes.medium
                    Layout.preferredHeight: LingmoUI.Units.iconSizes.medium
                    source: card.modelData.icon
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    RowLayout {
                        spacing: LingmoUI.Units.smallSpacing
                        Label {
                            text: card.modelData.name
                            font.pointSize: LingmoUI.Units.fontSubheading
                            font.weight: Font.DemiBold
                            color: LingmoUI.Theme.textColor
                        }
                        LingmoUI.Badge {
                            text: card.modelData.installed ? qsTr("Installed") : qsTr("Not installed")
                            tint: card.modelData.installed ? LingmoUI.Theme.successColor : LingmoUI.Theme.warningColor
                            iconName: card.modelData.installed ? "emblem-ok-symbolic" : ""
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: card.modelData.needs
                        wrapMode: Text.Wrap
                        color: LingmoUI.Theme.textColor
                        font.pointSize: LingmoUI.Units.fontBody
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: card.modelData.detail
                        wrapMode: Text.Wrap
                        color: LingmoUI.Theme.secondaryTextColor
                        font.pointSize: LingmoUI.Units.fontCaption
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: card.modelData.dimmed.length > 0
                        text: qsTr("Dimmed now: %1").arg(card.modelData.dimmed.join(", "))
                        wrapMode: Text.Wrap
                        color: LingmoUI.Theme.warningColor
                        font.pointSize: LingmoUI.Units.fontCaption
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: card.busy
                        text: prefs.installMessage
                        color: LingmoUI.Theme.secondaryTextColor
                        font.pointSize: LingmoUI.Units.fontCaption
                    }
                }
                LingmoUI.ActionButton {
                    Layout.alignment: Qt.AlignVCenter
                    visible: !card.modelData.installed
                    primary: true
                    text: card.modelData.id === "tesseract" && card.modelData.packages.indexOf("tesseract") < 0
                          ? qsTr("Install languages") : qsTr("Install")
                    busy: card.busy
                    enabled: prefs.installing.length === 0 || card.busy
                    tipText: card.modelData.packages.join(" ")
                    onClicked: prefs.install(card.modelData.id)
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Installed with the system's package manager; your password is asked.")
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.Wrap
        }
        LingmoUI.ActionButton {
            text: qsTr("Check again")
            iconName: "view-refresh"
            onClicked: prefs.reloadCatalog()
        }
    }
}
