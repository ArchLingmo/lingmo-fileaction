/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI
import ".."

// Each wheel per file type: hide slices, star favourites (they come first), change
// the order, with the real wheel drawn next to the list
PageScroll {
    id: page
    title: qsTr("Wheels")
    subtitle: qsTr("Favourites come first, in the order you starred them. Up to %1 slices fit on a wheel.").arg(prefs.maxSlices)

    readonly property var wheels: ["convert", "tools", "actions"]
    readonly property var wheelNames: [qsTr("Convert"), qsTr("Tools"), qsTr("Actions")]
    property int wheelIndex: 0
    readonly property string wheel: wheels[wheelIndex]
    property string category: "images"

    readonly property var categories: { prefs.revision; prefs.catalogLoading; return prefs.categories(wheel) }
    readonly property var slices: { prefs.revision; prefs.catalogLoading; return prefs.sliceList(wheel, category) }
    readonly property var preview: { prefs.revision; prefs.catalogLoading; return prefs.previewSlices(wheel, category) }

    // keep a category that exists on this wheel
    onCategoriesChanged: {
        for (let i = 0; i < categories.length; ++i) {
            if (categories[i].id === category)
                return
        }
        if (categories.length > 0)
            category = categories[0].id
    }

    RowLayout {
        Layout.fillWidth: true
        LingmoUI.SegmentedControl {
            model: page.wheels.map((w, i) => (prefs.keyLabels[w] ? prefs.keyLabels[w] + "  " : "") + page.wheelNames[i])
            currentIndex: page.wheelIndex
            onActivated: (index) => page.wheelIndex = index
        }
        Item { Layout.fillWidth: true }
        LingmoUI.ActionButton {
            text: qsTr("Reset to default")
            iconName: "edit-undo"
            onClicked: prefs.resetWheel(page.wheel)
        }
    }

    LingmoUI.Banner {
        Layout.fillWidth: true
        type: LingmoUI.Banner.Error
        text: page.wheel !== "actions" ? prefs.catalogError : ""
        closable: false
    }

    // File types, with how many slices each shows
    Flow {
        Layout.fillWidth: true
        visible: page.wheel !== "actions"
        spacing: LingmoUI.Units.smallSpacing
        Repeater {
            model: page.categories
            delegate: Rectangle {
                id: chip
                required property var modelData
                readonly property bool current: modelData.id === page.category
                width: chipRow.implicitWidth + LingmoUI.Units.largeSpacing * 1.5
                height: LingmoUI.Units.smallControlHeight
                radius: height / 2
                color: current ? LingmoUI.Theme.highlightColor
                     : chipMouse.containsMouse ? LingmoUI.Theme.alternateBackgroundColor : LingmoUI.Theme.secondBackgroundColor
                border.width: current ? 0 : 1
                border.color: LingmoUI.Theme.borderColor
                Behavior on color { ColorAnimation { duration: LingmoUI.Units.veryShortDuration } }
                RowLayout {
                    id: chipRow
                    anchors.centerIn: parent
                    spacing: LingmoUI.Units.smallSpacing
                    LingmoUI.IconItem {
                        Layout.preferredWidth: LingmoUI.Units.iconSizes.small
                        Layout.preferredHeight: LingmoUI.Units.iconSizes.small
                        source: chip.modelData.icon
                    }
                    Label {
                        text: chip.modelData.label
                        color: chip.current ? LingmoUI.Theme.highlightedTextColor : LingmoUI.Theme.textColor
                        font.pointSize: LingmoUI.Units.fontBody
                    }
                    LingmoUI.Badge {
                        text: chip.modelData.count
                        inverted: chip.current
                        subtle: true
                    }
                }
                MouseArea {
                    id: chipMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: page.category = chip.modelData.id
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: LingmoUI.Units.largeSpacing

        // The slices
        LingmoUI.Card {
            id: listCard
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: LingmoUI.Units.smallSpacing
                anchors.rightMargin: listCard.padding
                anchors.topMargin: LingmoUI.Units.smallSpacing
                anchors.bottomMargin: LingmoUI.Units.smallSpacing
                spacing: 0

                // Actions: how many "Open with" apps come first
                SettingRow {
                    visible: page.wheel === "actions"
                    Layout.leftMargin: LingmoUI.Units.smallSpacing + 28
                    title: qsTr("“Open with” apps")
                    subtitle: qsTr("The apps for the file's type, your default first.")
                    SpinBox {
                        from: 0; to: 4
                        value: prefs.openWithCount
                        onValueModified: prefs.openWithCount = value
                    }
                }

                LingmoUI.Spinner {
                    Layout.alignment: Qt.AlignHCenter
                    visible: prefs.catalogLoading && page.wheel !== "actions"
                    running: visible
                }

                Repeater {
                    model: page.slices
                    delegate: Rectangle {
                        id: row
                        required property var modelData
                        required property int index
                        readonly property bool favorite: modelData.favorite === true
                        readonly property bool shown: modelData.hidden !== true
                        Layout.fillWidth: true
                        implicitHeight: rowLayout.implicitHeight + LingmoUI.Units.smallSpacing
                        radius: LingmoUI.Theme.smallRadius
                        color: rowMouse.containsMouse ? LingmoUI.Theme.hoverColor : "transparent"
                        Behavior on color { ColorAnimation { duration: LingmoUI.Units.veryShortDuration } }
                        MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

                        RowLayout {
                            id: rowLayout
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: LingmoUI.Units.smallSpacing

                            LingmoUI.IconButton {
                                iconName: row.favorite ? "starred-symbolic" : "non-starred-symbolic"
                                iconColor: row.favorite ? LingmoUI.Theme.orangeColor : LingmoUI.Theme.secondaryTextColor
                                tipText: row.favorite ? qsTr("Remove from favourites") : qsTr("Favourite: always first")
                                enabled: row.shown
                                onClicked: prefs.setFavorite(row.modelData.id, !row.favorite)
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0
                                opacity: row.shown ? 1 : 0.5
                                Label {
                                    Layout.fillWidth: true
                                    text: row.modelData.label
                                    color: LingmoUI.Theme.textColor
                                    font.pointSize: LingmoUI.Units.fontBody
                                    font.weight: row.favorite ? Font.DemiBold : Font.Normal
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    visible: text.length > 0
                                    text: row.modelData.available === false
                                          ? qsTr("needs %1").arg((row.modelData.missing || []).join(", "))
                                          : (row.modelData.subtitle || "")
                                    color: row.modelData.available === false ? LingmoUI.Theme.warningColor : LingmoUI.Theme.secondaryTextColor
                                    font.pointSize: LingmoUI.Units.fontCaption
                                    elide: Text.ElideRight
                                }
                            }
                            LingmoUI.IconButton {
                                iconName: "go-up"
                                tipText: qsTr("Move up")
                                enabled: row.shown && row.index > 0 && page.slices[row.index - 1].favorite === row.favorite
                                opacity: rowMouse.containsMouse || hovered ? 1 : 0.35
                                onClicked: prefs.move(page.wheel, page.category, row.modelData.id, -1)
                            }
                            LingmoUI.IconButton {
                                iconName: "go-down"
                                tipText: qsTr("Move down")
                                enabled: row.shown && row.index + 1 < page.slices.length && page.slices[row.index + 1].favorite === row.favorite
                                opacity: rowMouse.containsMouse || hovered ? 1 : 0.35
                                onClicked: prefs.move(page.wheel, page.category, row.modelData.id, 1)
                            }
                            Switch {
                                checked: row.shown
                                onToggled: prefs.setHidden(row.modelData.id, !checked)
                                ToolTip.visible: hovered
                                ToolTip.text: checked ? qsTr("Shown on the wheel") : qsTr("Hidden")
                            }
                        }
                    }
                }
            }
        }

        // The wheel as it will look (the real wheel's drawing)
        LingmoUI.Card {
            id: previewCard
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 300
            Layout.preferredHeight: 330
            Item {
                anchors.fill: parent
                anchors.margins: LingmoUI.Units.smallSpacing
                WheelRing {
                    id: ring
                    width: implicitWidth
                    height: implicitHeight
                    anchors.centerIn: parent
                    scale: Math.min(parent.width / implicitWidth, (parent.height - previewLabel.height) / implicitHeight)
                    blur: false
                    solid: !prefs.wheelGlass
                    slices: page.preview
                    showNumbers: true
                    centerTitle: page.wheelNames[page.wheelIndex]
                    centerSubtitle: page.wheel === "actions" ? "" : (page.categories.find(c => c.id === page.category) || {}).label || ""
                    emptyText: qsTr("Every slice is hidden.")
                }
                Label {
                    id: previewLabel
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Preview")
                    color: LingmoUI.Theme.secondaryTextColor
                    font.pointSize: LingmoUI.Units.fontCaption
                }
            }
        }
    }
}
