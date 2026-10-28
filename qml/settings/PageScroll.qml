/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// One settings page: the title in the window's top strip, then the cards, scrolling
Item {
    id: page

    property string title
    property string subtitle
    default property alias content: column.data

    LingmoUI.PageHeader {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        title: page.title
        subtitle: page.subtitle
    }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.topMargin: header.height
        contentHeight: column.implicitHeight + LingmoUI.Units.largeSpacing * 3
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        LingmoUI.WheelHandler { target: flick }

        ColumnLayout {
            id: column
            x: LingmoUI.Units.largeSpacing * 2
            y: LingmoUI.Units.largeSpacing
            width: flick.width - LingmoUI.Units.largeSpacing * 4
            spacing: LingmoUI.Units.largeSpacing
        }
    }
}
