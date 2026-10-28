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
    title: qsTr("About")

    LingmoUI.Card {
        id: card
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: card.padding
            anchors.rightMargin: card.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing * 2
            anchors.bottomMargin: LingmoUI.Units.largeSpacing * 2
            spacing: LingmoUI.Units.smallSpacing

            LingmoUI.IconItem {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: LingmoUI.Units.iconSizes.huge
                Layout.preferredHeight: LingmoUI.Units.iconSizes.huge
                source: "lingmo-fileaction"
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("File actions")
                font.pointSize: LingmoUI.Units.fontHeading
                font.weight: Font.DemiBold
                color: LingmoUI.Theme.textColor
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Version %1").arg(appVersion)
                      + (prefs.engineVersion.length > 0 ? "  ·  " + qsTr("engine %1").arg(prefs.engineVersion) : "")
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontBody
            }
            Label {
                Layout.fillWidth: true
                Layout.topMargin: LingmoUI.Units.largeSpacing
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Based on FileAction (MIT) by Metamorfolog, inspired by Tangerine.")
                wrapMode: Text.Wrap
                color: LingmoUI.Theme.textColor
                font.pointSize: LingmoUI.Units.fontBody
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("File actions is free software under the GPL 3.0 or later. The conversions are FileAction's code, under the MIT licence.")
                wrapMode: Text.Wrap
                color: LingmoUI.Theme.secondaryTextColor
                font.pointSize: LingmoUI.Units.fontCaption
            }
        }
    }

    LingmoUI.Card {
        id: links
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: links.padding
            anchors.rightMargin: links.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing
            Repeater {
                model: [
                    { title: qsTr("File actions (this app)"), url: "https://github.com/ArchLingmo/lingmo-fileaction" },
                    { title: qsTr("The conversion engine (FileAction fork)"), url: "https://github.com/ArchLingmo/lingmo-fileaction-engine" },
                    { title: qsTr("FileAction by Metamorfolog"), url: "https://github.com/Metamorfolog/file_action" }
                ]
                delegate: SettingRow {
                    required property var modelData
                    title: modelData.title
                    subtitle: modelData.url
                    LingmoUI.IconButton {
                        iconName: "internet-web-browser"
                        tipText: qsTr("Open")
                        onClicked: prefs.openUrl(modelData.url)
                    }
                }
            }
            SettingRow {
                title: qsTr("Licences")
                subtitle: "/usr/share/licenses/lingmo-fileaction"
                LingmoUI.IconButton {
                    iconName: "folder-open"
                    tipText: qsTr("Open")
                    onClicked: prefs.openUrl("file:///usr/share/licenses/lingmo-fileaction")
                }
            }
        }
    }
}
