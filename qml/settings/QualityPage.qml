/*
 * SPDX-FileCopyrightText: 2026 LingmoOS Team
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LingmoUI.CompatibleModule 3.0 as LingmoUI

// FileAction's own quality settings (~/.config/fileaction/settings.json, "Quality"):
// the engine reads them on every conversion
PageScroll {
    id: page
    title: qsTr("Quality")
    subtitle: qsTr("Used by every conversion from now on.")

    component PercentRow: SettingRow {
        id: pr
        property int value
        property int from: 10
        property int to: 100
        signal moved(int value)
        Slider {
            id: slider
            Layout.preferredWidth: 200
            from: pr.from; to: pr.to; stepSize: 5
            snapMode: Slider.SnapAlways
            value: pr.value
            onMoved: pr.moved(Math.round(value))
        }
        Label {
            Layout.preferredWidth: 44
            text: qsTr("%1 %").arg(Math.round(slider.value))
            color: LingmoUI.Theme.textColor
            font.pointSize: LingmoUI.Units.fontBody
        }
    }

    component Heading: Label {
        font.pointSize: LingmoUI.Units.fontSubheading
        font.weight: Font.DemiBold
        color: LingmoUI.Theme.textColor
    }

    LingmoUI.Banner {
        Layout.fillWidth: true
        type: LingmoUI.Banner.Error
        text: prefs.settingsError
        closable: false
    }

    LingmoUI.Card {
        id: imgCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: imgCard.padding
            anchors.rightMargin: imgCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Heading { text: qsTr("Pictures") }
            PercentRow {
                title: qsTr("JPG, WebP and AVIF quality")
                subtitle: qsTr("Converting, rotating, shrinking (Max 1920 px) and PDF pages as pictures.")
                from: 40
                value: prefs.jpegQuality
                onMoved: (v) => prefs.jpegQuality = v
            }
            PercentRow {
                title: qsTr("Compress")
                subtitle: qsTr("The Compress slice: lower is smaller.")
                from: 30; to: 95
                value: prefs.compressQuality
                onMoved: (v) => prefs.compressQuality = v
            }
            SettingRow {
                title: qsTr("Remove the location from photos")
                subtitle: qsTr("The GPS position is taken out of converted and edited photos. The “No metadata” slice removes everything.")
                Switch {
                    checked: prefs.stripLocation
                    onToggled: prefs.stripLocation = checked
                }
            }
            SettingRow {
                title: qsTr("PDF pages as pictures")
                subtitle: qsTr("Resolution of PDF to JPG and PNG.")
                ComboBox {
                    id: dpi
                    Layout.preferredWidth: 160
                    readonly property var values: [150, 200, 300, 600]
                    model: values.map(v => qsTr("%1 DPI").arg(v))
                    currentIndex: Math.max(0, values.indexOf(prefs.pdfDpi))
                    onActivated: (i) => prefs.pdfDpi = values[i]
                }
            }
        }
    }

    LingmoUI.Card {
        id: videoCard
        Layout.fillWidth: true
        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: videoCard.padding
            anchors.rightMargin: videoCard.padding
            anchors.topMargin: LingmoUI.Units.largeSpacing
            anchors.bottomMargin: LingmoUI.Units.largeSpacing
            spacing: LingmoUI.Units.smallSpacing

            Heading { text: qsTr("Video") }
            SettingRow {
                title: qsTr("Converting and editing")
                subtitle: qsTr("MP4, MOV, MKV, 720p, rotate, trim, speed up.")
                ComboBox {
                    Layout.preferredWidth: 200
                    // H.264 CRF: lower is better and bigger
                    readonly property var values: [18, 23, 28]
                    model: [qsTr("High (bigger files)"), qsTr("Balanced"), qsTr("Small")]
                    currentIndex: { const i = values.indexOf(prefs.videoCrf); return i < 0 ? 1 : i }
                    onActivated: (i) => prefs.videoCrf = values[i]
                }
            }
            SettingRow {
                title: qsTr("Compress")
                subtitle: qsTr("The Compress slice for videos.")
                ComboBox {
                    Layout.preferredWidth: 200
                    readonly property var values: [24, 28, 32]
                    model: [qsTr("Light"), qsTr("Balanced"), qsTr("Strong (smallest)")]
                    currentIndex: { const i = values.indexOf(prefs.compressVideoCrf); return i < 0 ? 1 : i }
                    onActivated: (i) => prefs.compressVideoCrf = values[i]
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTr("Stored in FileAction's own settings, so its other apps use them too.")
            color: LingmoUI.Theme.secondaryTextColor
            font.pointSize: LingmoUI.Units.fontCaption
            wrapMode: Text.Wrap
        }
        LingmoUI.ActionButton {
            text: qsTr("Reset to default")
            iconName: "edit-undo"
            onClicked: prefs.resetQuality()
        }
    }
}
