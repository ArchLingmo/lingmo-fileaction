/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// A line in a card: title, an optional explanation, the control on the right
RowLayout {
    id: row

    property string title
    property string subtitle
    default property alias control: controlArea.data

    Layout.fillWidth: true
    Layout.minimumHeight: LingmoUI.Units.controlHeight
    spacing: LingmoUI.Units.largeSpacing

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Label {
            Layout.fillWidth: true
            text: row.title
            color: LingmoUI.Theme.textColor
            font.pointSize: LingmoUI.Units.fontBody
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: row.subtitle
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.Wrap
        }
    }

    RowLayout {
        id: controlArea
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        spacing: LingmoUI.Units.smallSpacing
    }
}
